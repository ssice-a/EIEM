"""Query the EIEM Endfield asset index without exporting anything.

The index is a flat record stream written by the unpacker's
EndfieldIndexStore: a magic header, then per-record payloads. It is large
(hundreds of MB) and uncompressed, so this walks it once and filters on the way
rather than building a full in-memory map.

Format (little endian):
    header : string magic "EIEM_END_FIELD_INDEX"
             int32  version (3)
             string vfsFingerprint
    record : byte   kind   (0 = end, 1 = asset, 2 = cab)
    asset  : string name, string container, string type, int64 pathID, string source
    cab    : string name, string source, int32 dependencyCount, string[] dependencies
    end    : int64 assetCount, int64 cabCount

`BinaryReader.ReadString` uses 7-bit-encoded length prefixed UTF-8, which is
what this reads.

Usage:
    python tools/query_endfield_index.py INDEX.eidx --name typhoea
    python tools/query_endfield_index.py INDEX.eidx --container uimodels --type Texture2D
    python tools/query_endfield_index.py INDEX.eidx --cab typhoea --cabs-only
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

MAGIC = "EIEM_END_FIELD_INDEX"
ASSET, CAB, END = 1, 2, 0


class Reader:
    def __init__(self, path: Path) -> None:
        self.handle = path.open("rb")
        self.size = path.stat().st_size

    def close(self) -> None:
        self.handle.close()

    def byte(self) -> int:
        chunk = self.handle.read(1)
        if not chunk:
            raise EOFError
        return chunk[0]

    def take(self, count: int) -> bytes:
        chunk = self.handle.read(count)
        if len(chunk) != count:
            raise EOFError
        return chunk

    def int32(self) -> int:
        return int.from_bytes(self.take(4), "little", signed=True)

    def int64(self) -> int:
        return int.from_bytes(self.take(8), "little", signed=True)

    def string(self) -> str:
        length = 0
        shift = 0
        while True:
            value = self.byte()
            length |= (value & 0x7F) << shift
            if not value & 0x80:
                break
            shift += 7
        return self.take(length).decode("utf-8", "replace")

    def tell(self) -> int:
        return self.handle.tell()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("index", help="the .eidx file")
    parser.add_argument("--name", help="regex on the asset name")
    parser.add_argument("--container", help="regex on the container path")
    parser.add_argument("--type", help="exact asset type, e.g. Texture2D or MonoBehaviour")
    parser.add_argument("--cab", help="regex on the cab/bundle name, searches cab records")
    parser.add_argument("--cabs-only", action="store_true", help="only print cab records")
    parser.add_argument("--limit", type=int, default=40, help="stop after this many hits")
    parser.add_argument(
        "--show-dependencies",
        action="store_true",
        help="for cab hits, list the bundle dependencies",
    )
    args = parser.parse_args()

    name_re = re.compile(args.name, re.I) if args.name else None
    container_re = re.compile(args.container, re.I) if args.container else None
    cab_re = re.compile(args.cab, re.I) if args.cab else None

    reader = Reader(Path(args.index))
    try:
        if reader.string() != MAGIC:
            print("error: not an EIEM Endfield index", file=sys.stderr)
            return 1
        version = reader.int32()
        if version != 3:
            print(f"error: unsupported index version {version}", file=sys.stderr)
            return 1
        fingerprint = reader.string()
        print(f"index version={version} fingerprint={fingerprint[:16]}...")

        assets = cabs = 0
        hits = 0
        while reader.tell() < reader.size:
            kind = reader.byte()
            if kind == END:
                total_assets, total_cabs = reader.int64(), reader.int64()
                print(f"assets={total_assets} cabs={total_cabs} (scanned {assets}/{cabs})")
                break
            if kind == ASSET:
                name = reader.string()
                container = reader.string()
                type_name = reader.string()
                path_id = reader.int64()
                source = reader.string()
                assets += 1
                if args.cabs_only:
                    continue
                if args.type and type_name != args.type:
                    continue
                if name_re and not name_re.search(name):
                    continue
                if container_re and not container_re.search(container):
                    continue
                hits += 1
                if hits <= args.limit:
                    print(f"  [{type_name}] {name}")
                    print(f"      container={container}")
                    print(f"      cab={source} pathID={path_id}")
                continue
            if kind == CAB:
                cab_name = reader.string()
                source = reader.string()
                count = reader.int32()
                dependencies = [reader.string() for _ in range(max(0, count))]
                cabs += 1
                if cab_re and not cab_re.search(cab_name):
                    continue
                hits += 1
                if hits <= args.limit:
                    print(f"  [cab] {cab_name} dependencies={len(dependencies)}")
                    if args.show_dependencies:
                        for dependency in dependencies:
                            print(f"      -> {dependency}")
                continue
            print(f"error: unknown record kind {kind} at {reader.tell()}", file=sys.stderr)
            return 2
        if hits > args.limit:
            print(f"... {hits - args.limit} more hit(s) suppressed")
        print(f"hits={hits}")
    finally:
        reader.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
