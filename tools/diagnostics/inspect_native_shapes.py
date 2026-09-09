"""Read-only PE inspection for the shape-weight investigation.

Uses installed pefile/capstone. RVAs belong only to the inspected file: this is
not a runtime address resolver, hook installer, or patcher. Xrefs are byte-scan
candidates, not a complete control-flow/call graph.
"""
import argparse
import bisect
import json
import mmap
import re
import struct
from pathlib import Path

import capstone
import pefile


class Image:
    def __init__(self, path, data):
        self.path, self.data = path, data
        self.pe = pefile.PE(str(path), fast_load=True)
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
        self.code = [s for s in self.pe.sections if s.Characteristics & 0x20000000]
        directory = self.pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
        self.functions = []
        if directory.VirtualAddress and directory.Size:
            offset = self.offset(directory.VirtualAddress)
            for p in range(offset, offset + directory.Size - 11, 12):
                start, end, _ = struct.unpack_from('<III', data, p)
                if start and end > start:
                    self.functions.append((start, end))
        self.functions.sort()
        self.starts = [f[0] for f in self.functions]

    def offset(self, rva):
        return self.pe.get_offset_from_rva(rva)

    def containing(self, rva):
        i = bisect.bisect_right(self.starts, rva) - 1
        if i >= 0 and rva < self.functions[i][1]:
            return self.functions[i]
        return None

    def instructions(self, rva, length):
        offset = self.offset(rva)
        return [dict(rva=hex(i.address), bytes=i.bytes.hex(), asm=i.mnemonic + ' ' + i.op_str)
                for i in self.dis.disasm(self.data[offset:offset + length], rva)]

    def function(self, rva, limit):
        bounds = self.containing(rva)
        start, end = bounds if bounds else (rva, rva + limit)
        return dict(request=hex(rva), unwind_bounds=bool(bounds), start=hex(start), end=hex(end),
                    truncated=end-start > limit,
                    instructions=self.instructions(start, min(end-start, limit)))

    def strings(self, needle):
        result, start = [], 0
        term = needle.encode('ascii')
        while True:
            pos = self.data.find(term, start)
            if pos < 0:
                return result
            end = self.data.find(b'\0', pos, pos + 512)
            if end < 0:
                end = pos + len(term)
            result.append(dict(rva=hex(self.pe.get_rva_from_offset(pos)),
                               text=self.data[pos:end].decode('ascii', 'replace')))
            start = pos + len(term)

    def references(self, targets):
        # RIP-relative LEA and immediate relative CALL/JMP candidates, verified
        # by decoding at the candidate address. No instruction-boundary claim.
        patterns = [(re.compile(rb'[\x48-\x4f]\x8d[\x05\x0d\x15\x1d\x25\x2d\x35\x3d]'), 7, 3, 'rip-lea'),
                    (re.compile(rb'[\xe8\xe9]'), 5, 1, 'relative-branch')]
        found = []
        for section in self.code:
            lo, hi = section.PointerToRawData, section.PointerToRawData + section.SizeOfRawData
            for pattern, size, displacement, kind in patterns:
                for match in pattern.finditer(self.data, lo, hi - size + 1):
                    off = match.start()
                    rva = section.VirtualAddress + off - lo
                    target = rva + size + struct.unpack_from('<i', self.data, off + displacement)[0]
                    if target not in targets:
                        continue
                    instruction = next(self.dis.disasm(self.data[off:off+size], rva), None)
                    if instruction is None or instruction.size != size:
                        continue
                    function = self.containing(rva)
                    found.append(dict(rva=hex(rva), target=hex(target), kind=kind,
                                      function=hex(function[0]) if function else None,
                                      asm=instruction.mnemonic + ' ' + instruction.op_str))
        return found

    def pointers(self, targets):
        found = []
        for target in sorted(targets):
            pattern = struct.pack('<Q', self.base + target)
            start = 0
            while True:
                pos = self.data.find(pattern, start)
                if pos < 0:
                    break
                start = pos + 8
                rva = self.pe.get_rva_from_offset(pos)
                values = []
                for p in range(max(0, pos - 16), min(len(self.data) - 7, pos + 24), 8):
                    value = struct.unpack_from('<Q', self.data, p)[0]
                    values.append(dict(at=hex(self.pe.get_rva_from_offset(p)), value=hex(value),
                                       as_rva=hex(value-self.base) if self.base <= value < self.base+self.pe.OPTIONAL_HEADER.SizeOfImage else None))
                found.append(dict(rva=hex(rva), target=hex(target), nearby=values))
        return found

    def ascii_pointer(self, rva):
        try:
            target = struct.unpack_from('<Q', self.data, self.offset(rva))[0] - self.base
            offset = self.offset(target)
            end = self.data.find(b'\0', offset, offset+512)
            value = self.data[offset:end] if end >= offset else b''
            if value and all(32 <= b < 127 for b in value):
                return value.decode('ascii')
        except (pefile.PEFormatError, struct.error):
            pass
        return None

    def string_table(self, rva):
        lo = hi = rva
        while lo >= 8 and rva-lo < 8*32768 and self.ascii_pointer(lo-8):
            lo -= 8
        while hi-rva < 8*32768 and self.ascii_pointer(hi+8):
            hi += 8
        return dict(request=hex(rva), start=hex(lo), end=hex(hi+8), index=(rva-lo)//8,
                    count=(hi-lo)//8+1, entry=self.ascii_pointer(rva))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    parser.add_argument('--needle', action='append', default=[])
    parser.add_argument('--rva', action='append', type=lambda x: int(x, 0), default=[])
    parser.add_argument('--xref', action='append', type=lambda x: int(x, 0), default=[])
    parser.add_argument('--limit', type=int, default=768)
    parser.add_argument('--span', action='append', default=[], help='exact RVA:byte_count disassembly window')
    parser.add_argument('--pointers', action='store_true')
    parser.add_argument('--table', action='append', type=lambda x: int(x, 0), default=[])
    parser.add_argument('--output', type=Path)
    parser.add_argument('--text', action='store_true', help='compact console disassembly')
    args = parser.parse_args()
    with args.image.open('rb') as file, mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ) as data:
        image = Image(args.image, data)
        strings = [item for term in args.needle for item in image.strings(term)]
        tables = [image.string_table(rva) for rva in args.table]
        targets = {int(s['rva'], 16) for s in strings} | set(args.xref) | {int(t['start'],16) for t in tables}
        report = dict(image=str(args.image), image_base=hex(image.base), size=len(data),
                      timestamp=image.pe.FILE_HEADER.TimeDateStamp,
                      strings=strings, references=image.references(targets) if targets else [],
                      functions=[image.function(rva, args.limit) for rva in args.rva],
                      pointers=image.pointers(targets) if args.pointers else [],
                      tables=tables,
                      spans=[image.instructions(*(int(part, 0) for part in span.split(':'))) for span in args.span])
        text = json.dumps(report, indent=2, ensure_ascii=False)
    if args.output:
        # Exclusive report creation; never overwrite a binary or prior capture.
        with args.output.open('x', encoding='utf-8') as file:
            file.write(text + '\n')
        print(args.output)
    else:
        if args.text:
            print(report['image'])
            for item in report['strings'] + report['references'] + report['tables'] + report['pointers']:
                print(item)
            for function in report['functions']:
                print('UNWIND_RANGE' if function['unwind_bounds'] else 'RAW_RANGE',
                      function['start'], function['end'], 'truncated=', function['truncated'])
                for instruction in function['instructions']:
                    print(instruction['rva'], instruction['asm'])
            for span in report['spans']:
                print('SPAN')
                for instruction in span:
                    print(instruction['rva'], instruction['asm'])
        else:
            print(text)


if __name__ == '__main__':
    main()
