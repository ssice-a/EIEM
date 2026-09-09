"""Read-only metadata-v29 type/method lookup, with optional PE method RVAs.

Layout reference: Il2CppDumper MetadataClass.cs / Il2CppClass.cs (v29 fields).
Does not load a game DLL or infer runtime addresses. Module RVA must identify
the selected assembly's Il2CppCodeGenModule, not an arbitrary pointer table.
"""
import argparse
import json
import mmap
import struct
from pathlib import Path

from inspect_native_shapes import Image


def inspect(metadata, image, module_rva, class_name, namespace, type_stride=88):
    magic, version = struct.unpack_from('<II', metadata)
    if magic != 0xFAB11BAF or version != 29:
        raise ValueError('Only standard metadata v29 is supported by this diagnostic')

    def table(pair, stride):
        offset, size = struct.unpack_from('<II', metadata, 8+pair*8)
        if offset+size > len(metadata) or size % stride:
            raise ValueError('Unexpected table layout')
        return offset, size//stride

    string_offset, string_size = table(2, 1)
    types_offset, types_count = table(19, type_stride)
    methods_offset, methods_count = table(5, 32)
    parameters_offset, parameters_count = table(10, 12)
    fields_offset, fields_count = table(11, 12)
    images_offset, images_count = table(20, 40)

    def name(index):
        if not 0 <= index < string_size:
            raise ValueError('Invalid string index')
        end = metadata.find(b'\0', string_offset+index, string_offset+string_size)
        if end < 0:
            raise ValueError('Unterminated metadata string')
        return metadata[string_offset+index:end].decode('utf-8')

    matches = []
    for type_index in range(types_count):
        offset = types_offset+type_stride*type_index
        fields = struct.unpack_from('<16I', metadata, offset)
        # The inspected Endfield v29 file adds four bytes before the counts.
        # Explicit caller selection only; owner, token and module checks below
        # must still agree. The extra field is not assigned a guessed meaning.
        counts = struct.unpack_from('<8H', metadata, offset+type_stride-24)
        if name(fields[0]) != class_name or namespace is not None and name(fields[1]) != namespace:
            continue
        assembly = None
        for n in range(images_count):
            definition = struct.unpack_from('<10I', metadata, images_offset+40*n)
            if definition[2] <= type_index < definition[2]+definition[3]:
                assembly = name(definition[0])
                break
        pointer_count = pointer_rva = 0
        if image:
            module_name, pointer_count, pointer_va = struct.unpack_from('<QQQ', image.data, image.offset(module_rva))
            module_name_offset = image.offset(module_name-image.base)
            module_name_end = image.data.find(b'\0', module_name_offset, module_name_offset+256)
            actual = image.data[module_name_offset:module_name_end].decode('ascii')
            if actual != assembly:
                raise ValueError(f'Module {actual} does not match metadata assembly {assembly}')
            pointer_rva = pointer_va-image.base
        declared_fields = []
        if counts[2] and fields[8]+counts[2] > fields_count:
            raise ValueError('Invalid field range')
        for index in range(fields[8], fields[8]+counts[2]):
            definition = struct.unpack_from('<3I', metadata, fields_offset+12*index)
            if definition[2] >> 24 != 4:
                raise ValueError('Invalid field token')
            # Type indices describe metadata, not runtime offsets or serialized
            # field presence. Resolve types with a real registration/runtime dump.
            declared_fields.append(dict(name=name(definition[0]), type_index=definition[1],
                                        token=hex(definition[2])))
        methods = []
        if counts[0] and fields[9]+counts[0] > methods_count:
            raise ValueError('Invalid method range')
        for index in range(fields[9], fields[9]+counts[0]):
            values = struct.unpack_from('<6I4H', metadata, methods_offset+32*index)
            if values[1] != type_index or values[5] >> 24 != 6:
                raise ValueError('Invalid method owner/token')
            parameters = []
            if values[9] and values[3]+values[9] > parameters_count:
                raise ValueError('Invalid parameter range')
            for parameter_index in range(values[3], values[3]+values[9]):
                parameter = struct.unpack_from('<3I', metadata, parameters_offset+12*parameter_index)
                if parameter[1] >> 24 != 8:
                    raise ValueError('Invalid parameter token')
                parameters.append(dict(name=name(parameter[0]), token=hex(parameter[1]),
                                       type_index=parameter[2]))
            slot = (values[5] & 0xffffff)-1
            rva = None
            if image:
                if not 0 <= slot < pointer_count:
                    raise ValueError('Method token exceeds module pointer count')
                address = struct.unpack_from('<Q', image.data, image.offset(pointer_rva+8*slot))[0]
                rva = hex(address-image.base) if address else None
            methods.append(dict(name=name(values[0]), token=hex(values[5]), parameter_count=values[9],
                                parameters=parameters, flags=hex(values[6]), return_type_index=values[2], rva=rva))
        matches.append(dict(assembly=assembly, namespace=name(fields[1]), name=class_name,
                            type_index=type_index, fields=declared_fields, methods=methods))
    return matches


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('metadata', type=Path)
    parser.add_argument('type_name')
    parser.add_argument('--namespace')
    parser.add_argument('--image', type=Path)
    parser.add_argument('--module-rva', type=lambda x:int(x,0))
    parser.add_argument('--output', type=Path)
    parser.add_argument('--type-stride', type=int, choices=(88,92), default=88)
    args = parser.parse_args()
    if bool(args.image) != (args.module_rva is not None):
        parser.error('--image and --module-rva are required together')
    with args.metadata.open('rb') as f, mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as data:
        if args.image:
            with args.image.open('rb') as binary, mmap.mmap(binary.fileno(),0,access=mmap.ACCESS_READ) as raw:
                result = inspect(data,Image(args.image,raw),args.module_rva,args.type_name,args.namespace,args.type_stride)
        else:
            result = inspect(data,None,None,args.type_name,args.namespace,args.type_stride)
    text = json.dumps(result,ensure_ascii=False,indent=2)
    if args.output:
        with args.output.open('x',encoding='utf-8') as f:
            f.write(text+'\n')
        print(args.output)
    else:
        print(text)


if __name__ == '__main__':
    main()
