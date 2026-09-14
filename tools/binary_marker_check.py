"""Report whether given ASCII markers are present in a binary as raw bytes."""
import sys
from pathlib import Path

binary = Path(sys.argv[1])
blob = binary.read_bytes()
print(f"{binary}  {len(blob)} bytes")
for marker in sys.argv[2:]:
    needle = marker.encode("ascii")
    count = blob.count(needle)
    print(f"  {marker!r:34} occurrences={count}")
