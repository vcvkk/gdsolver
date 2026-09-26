"""Translate raw Windows 2.2081 offsets into iOS offsets through the compiler's record layouts.

usage: map_offsets.py <layouts-Win64.txt> <layouts-iOS.txt> <offsets.txt>

offsets.txt has one `Class 0xOFFSET label` per line (# starts a comment). For each one the
Windows layout names the member that holds the offset -- the innermost direct member of the
class or of one of its bases -- and the iOS layout gives that member's offset there. The byte
inside the member is kept, which is only sound when the member's own internals are laid out
the same way on both; the report says where that is not a given (containers, strings) and where
the offset falls in padding or past a member, i.e. has no member at all.
"""
import re
import sys
from dataclasses import dataclass

LINE = re.compile(r"^\s*(\d+)(?::(\d+)-(\d+))? \|( +)(.*?)\s*$")


@dataclass
class Field:
    off: int
    depth: int
    text: str       # "double m_x", "class GameObject (primary base)", "(... vtable pointer)"
    path: tuple     # names from the record down to this entry
    is_base: bool


def parse(path):
    """{class name: [Field]} for every top-level record dump in the file."""
    records = {}
    cur = None
    stack = []
    for raw in open(path, encoding="utf-8", errors="replace"):
        if raw.startswith("*** Dumping AST Record Layout"):
            cur = None
            continue
        m = LINE.match(raw)
        if not m:
            continue
        off = int(m.group(1))
        depth = (len(m.group(4)) - 1) // 2
        text = m.group(5)
        if depth == 0:
            name = re.sub(r"^(class|struct) ", "", text)
            cur = records.setdefault(name, [])
            stack = [name]
            continue
        if cur is None:
            continue
        is_base = bool(re.search(r"\((primary |virtual )?base\)$", text))
        if is_base:
            name = re.sub(r"^(class|struct) ", "", re.sub(r" \((primary |virtual )?base\)$", "", text))
        elif text.startswith("("):
            name = text
        else:
            name = text.split()[-1]
        del stack[depth:]
        stack.append(name)
        cur.append(Field(off, depth, text, tuple(stack), is_base))
    return records


def direct_members(fields):
    """Members that belong to the class or to one of its bases, not to a member's insides."""
    out = []
    for i, f in enumerate(fields):
        if f.is_base:
            continue
        # every ancestor on the path must be a base (or the record itself)
        owners = [g for g in fields[:i] if g.depth < f.depth and g.path == f.path[: g.depth + 1]]
        if all(g.is_base for g in owners):
            out.append(f)
    return out


def member_type(text):
    return text.rsplit(" ", 1)[0] if " " in text else text


def locate(fields, off, size_hint):
    """(member, byte inside it, member size) for the direct member holding `off`."""
    mem = sorted(direct_members(fields), key=lambda f: f.off)
    best = None
    for i, f in enumerate(mem):
        if f.off <= off:
            nxt = next((g.off for g in mem[i + 1:] if g.off > f.off), size_hint)
            best = (f, off - f.off, nxt - f.off)
    return best


def record_size(path, name):
    txt = open(path, encoding="utf-8", errors="replace").read()
    m = re.search(r"\| (?:class|struct) " + re.escape(name) + r"\n(.*?)\[sizeof=(\d+)", txt, re.S)
    return int(m.group(2)) if m else 1 << 30


def main():
    win_path, ios_path, spec = sys.argv[1:4]
    win, ios = parse(win_path), parse(ios_path)
    rows = []
    for line in open(spec, encoding="utf-8"):
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        cls, hexoff, *label = line.split()
        off = int(hexoff, 16)
        loc = locate(win[cls], off, record_size(win_path, cls))
        if not loc:
            rows.append((cls, hexoff, "?", "?", "no member", " ".join(label)))
            continue
        f, inner, size = loc
        target = next((g for g in direct_members(ios[cls]) if g.path == f.path), None)
        note = []
        if inner >= size:
            note.append("PADDING")
        t = member_type(f.text)
        if inner and re.search(r"std::|gd::|unordered|map|vector|string|set<", t):
            note.append("INSIDE-CONTAINER")
        if target is None:
            rows.append((cls, hexoff, f.path[-1], "?", "MISSING-ON-IOS " + " ".join(note),
                         " ".join(label)))
            continue
        ios_off = target.off + inner
        rows.append((cls, hexoff, f"{f.path[-1]}+{inner}" if inner else f.path[-1],
                     hex(ios_off), " ".join(note) or "ok", " ".join(label)))
    w = [max(len(str(r[i])) for r in rows) for i in range(5)]
    for r in rows:
        print("  ".join(str(r[i]).ljust(w[i]) for i in range(5)) + "  " + r[5])


if __name__ == "__main__":
    main()
