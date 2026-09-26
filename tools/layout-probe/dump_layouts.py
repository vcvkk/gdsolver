"""Re-run probe.cpp's compile command with -fdump-record-layouts and keep the classes we need.

usage: dump_layouts.py <build dir> <output file>

The command comes from compile_commands.json, so the flags (target triple, defines, include
paths, the bindings) are exactly the ones the mod is built with on this platform. The compiler
launcher and the precompiled header are dropped: a layout read out of a PCH is not re-printed.
"""
import json
import re
import shlex
import subprocess
import sys
from pathlib import Path

WANTED = {
    "PlayerObject", "GameObject", "EnhancedGameObject", "EffectGameObject", "EnterEffectObject",
    "GJBaseGameLayer", "PlayLayer", "GJEffectManager", "EnterEffectInstance", "GJGameLevel",
    "TeleportPortalObject", "RingObject",
}

build = Path(sys.argv[1])
out = Path(sys.argv[2])
entries = json.loads((build / "compile_commands.json").read_text())
entry = next(e for e in entries if e["file"].replace("\\", "/").endswith("probe.cpp"))
if entry.get("arguments"):
    args = entry["arguments"]
else:
    # Windows commands carry backslash paths and \"-escaped defines; clang takes forward
    # slashes, so keep the escaped quotes and turn every other backslash around.
    cmd = entry["command"].replace('\\"', "\0").replace("\\", "/").replace("\0", '\\"')
    args = shlex.split(cmd)

# Drop a compiler launcher (sccache/ccache) in front of the compiler.
while args and re.search(r"(s?ccache)(\.exe)?$", args[0], re.I):
    args = args[1:]

cleaned = []
skip = 0
for i, a in enumerate(args):
    if skip:
        skip -= 1
        continue
    if a in ("-o", "-MF", "-MT", "-MQ"):
        skip = 1
        continue
    if a in ("-c", "-MD", "-MMD") or a.startswith("/Fo") or a.startswith("-Fo"):
        continue
    # -Xclang -include-pch -Xclang <file> / -Xclang -include -Xclang <cmake_pch.hxx>
    if a == "-Xclang" and i + 3 < len(args) and args[i + 1] in ("-include-pch", "-include") \
            and args[i + 2] == "-Xclang" and "pch" in args[i + 3]:
        skip = 3
        continue
    if a.startswith("-include") and "pch" in a:
        continue
    cleaned.append(a)

cleaned += ["-fsyntax-only", "-Xclang", "-fdump-record-layouts"]
print("running:", " ".join(cleaned[:3]), "...", flush=True)
proc = subprocess.run(cleaned, cwd=entry["directory"], capture_output=True, text=True)
if proc.returncode != 0:
    sys.stderr.write(proc.stderr[-20000:])
    sys.exit(proc.returncode)

blocks = proc.stdout.split("*** Dumping AST Record Layout")
kept = []
seen = set()
for b in blocks:
    m = re.search(r"^\s*0 \| (?:class|struct) (\S+)", b, re.M)
    if not m:
        continue
    name = m.group(1)
    if name in WANTED and name not in seen:
        seen.add(name)
        kept.append("*** Dumping AST Record Layout" + b.rstrip() + "\n")

out.write_text("".join(kept))
print(f"kept {len(kept)} of {len(blocks) - 1} layouts: {sorted(seen)}")
missing = WANTED - seen
if missing:
    print("MISSING:", sorted(missing))
    sys.exit(1)
