#!/usr/bin/env python3
"""Regression tests for the portable DIS8 tools.

Compat goldens for dis8 and dis6800 were produced by original/DIS8.EXE and
original/DIS6800.EXE under DOSBox-X. The default (fixed) goldens are the
corrected walk. Where the two modes print the same listing, both files are
still stored so a later edit cannot change one of them quietly.

DISZ80 has no executable in this archive. Its compat goldens are the 1988
control flow, reviewed against the source and against the DIS8.EXE listing
where the two programs implement the same instruction.
"""

import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FIX = ROOT / "tests" / "fixtures"
GOLD = ROOT / "tests" / "golden"
ORIG = ROOT / "original"


def norm(data):
    return data.replace(b"\r\n", b"\n").replace(b"\r", b"\n")


def run(cmd, cwd=None):
    result = subprocess.run(cmd, cwd=cwd, capture_output=True)
    if result.returncode != 0:
        sys.stderr.write(result.stdout.decode("utf-8", "replace"))
        sys.stderr.write(result.stderr.decode("utf-8", "replace"))
        raise SystemExit("command failed (%s): %s" % (result.returncode, " ".join(cmd)))
    return result


def golden(name, tool, mode):
    path = GOLD / ("%s.%s.%s.mac" % (name, tool, mode))
    if not path.is_file():
        raise SystemExit("missing golden %s" % path)
    return path.read_bytes()


def listing(tool, mode, args, dest, extra=()):
    flags = ["--compat"] if mode == "compat" else []
    run([tool, *flags, *[str(a) for a in args], str(dest), *[str(a) for a in extra]])
    return norm(dest.read_bytes())


def check_pair(tool, name, args, failures):
    for mode in ("compat", "fixed"):
        with tempfile.TemporaryDirectory() as tmp:
            dest = Path(tmp) / "out.mac"
            got = listing(tool, mode, args, dest)
        exp = golden(name, Path(tool).name if False else None, mode)
        # tool path may be absolute; goldens use the program name
        exp = golden(name, tool_key(tool), mode)
        if got != exp:
            failures.append("%s %s %s (%s bytes vs %s)" % (tool_key(tool), name, mode, len(got), len(exp)))


def tool_key(path):
    name = Path(path).name.lower()
    if name.endswith(".exe"):
        name = name[:-4]
    return name


def call_chain(count):
    image = bytearray()
    for i in range(count):
        target = (i + 1) * 3
        image += bytes([0xCD, target & 0xFF, (target >> 8) & 0xFF])
    image += bytes([0xC9])
    lines = ["\t.Z80", "", "\tORG\t0000H", ""]
    for i in range(count):
        lines.append("L%04X:\tCALL\tL%04X" % (i * 3, (i + 1) * 3))
    lines.append("L%04X:\tRET" % (count * 3))
    lines.extend(["", "\tEND", ""])
    return bytes(image), ("\n".join(lines)).encode()


def hex_checksum_ok(text):
    lines = [ln for ln in text.splitlines() if ln.startswith(":")]
    if not lines or lines[-1] != ":0000000000":
        return False
    for line in lines:
        digits = line[1:]
        if len(digits) < 10 or len(digits) % 2:
            return False
        if digits.lower() != digits.upper() and any(c.islower() for c in digits):
            return False
        total = 0
        for i in range(0, len(digits), 2):
            total = (total + int(digits[i:i + 2], 16)) & 0xFF
        if line != ":0000000000" and total != 0:
            return False
    return True


def main():
    if len(sys.argv) != 7:
        raise SystemExit("usage: regression.py dis8 disz80 dis6800 comtohex hextocom plus")
    dis8, disz80, dis6800, comtohex, hextocom, plus = sys.argv[1:7]
    failures = []

    dis8_cases = [
        ("ix", [dis8, "Z80", FIX / "ix.com", FIX / "ix.par"]),
        ("ix2", [dis8, "Z80", FIX / "ix2.com", FIX / "ix2.par"]),
        ("cb2", [dis8, "Z80", FIX / "cb2.com", FIX / "cb2.par"]),
        ("rim", [dis8, "8085", FIX / "rim.com", FIX / "rim.par"]),
        ("r80", [dis8, "8080", FIX / "r80.com", FIX / "r80.par"]),
        ("dat", [dis8, "8080", FIX / "dat.com", FIX / "dat.par"]),
        ("brk", [dis8, "Z80", FIX / "brk.com", FIX / "brk.par"]),
        ("eof", [dis8, "Z80", FIX / "eof.com", FIX / "eof.par"]),
        ("ulim", [dis8, "Z80", FIX / "ulim.com", FIX / "ulim.par"]),
        ("jphl", [dis8, "Z80", FIX / "jphl.com", FIX / "jphl.par"]),
        ("rst", [dis8, "Z80", FIX / "rst.com", FIX / "rst.par"]),
        ("jr", [dis8, "Z80", FIX / "jr.com", FIX / "jr.par"]),
        ("call", [dis8, "Z80", FIX / "call.com", FIX / "call.par"]),
        ("ed", [dis8, "Z80", FIX / "ed.com", FIX / "ed.par"]),
        ("sym", [dis8, "Z80", FIX / "sym.com", FIX / "sym.par"], [FIX / "sym.sym"]),
        ("str", [dis8, "Z80", FIX / "str.com", FIX / "str.par"]),
        ("deep", [dis8, "Z80", FIX / "deep.com", FIX / "deep.par"]),
        ("ts", [dis8, "Z80", ORIG / "TSRE0.COM", FIX / "ts.par"]),
        ("l80", [dis8, "Z80", ORIG / "EMZ.COM", ORIG / "L80.PAR"]),
        ("msx", [dis8, "Z80", ORIG / "EMZ.COM", ORIG / "MSX.PAR"]),
    ]
    disz80_cases = [
        ("ix", [disz80, FIX / "ix.com", FIX / "ix.par"]),
        ("ix2", [disz80, FIX / "ix2.com", FIX / "ix2.par"]),
        ("cb2", [disz80, FIX / "cb2.com", FIX / "cb2.par"]),
        ("dat", [disz80, FIX / "dat.com", FIX / "dat.par"]),
        ("brk", [disz80, FIX / "brk.com", FIX / "brk.par"]),
        ("ed", [disz80, FIX / "ed.com", FIX / "ed.par"]),
        ("jphl", [disz80, FIX / "jphl.com", FIX / "jphl.par"]),
        ("jr", [disz80, FIX / "jr.com", FIX / "jr.par"]),
        ("call", [disz80, FIX / "call.com", FIX / "call.par"]),
        ("str", [disz80, FIX / "str.com", FIX / "str.par"]),
        ("deep", [disz80, FIX / "deep.com", FIX / "deep.par"]),
    ]
    dis6800_cases = [
        ("bra", [dis6800, FIX / "bra.com", FIX / "bra.par"]),
        ("jmpx", [dis6800, FIX / "jmpx.com", FIX / "jmpx.par"]),
        ("jsrx", [dis6800, FIX / "jsrx.com", FIX / "jsrx.par"]),
    ]

    def check(name, argv, extra=()):
        tool = argv[0]
        args = argv[1:]
        for mode in ("compat", "fixed"):
            with tempfile.TemporaryDirectory() as tmp:
                dest = Path(tmp) / "out.mac"
                got = listing(tool, mode, args, dest, extra)
            exp = golden(name, tool_key(tool), mode)
            if got != exp:
                failures.append("%s %s %s" % (tool_key(tool), name, mode))

    for item in dis8_cases + disz80_cases + dis6800_cases:
        if len(item) == 3:
            check(item[0], item[1], item[2])
        else:
            check(item[0], item[1])

    # --crlf matches the DOS text-mode bytes of a small DIS8 listing.
    with tempfile.TemporaryDirectory() as tmp:
        dest = Path(tmp) / "out.mac"
        run([
            dis8, "--compat", "--crlf", "Z80",
            str(FIX / "ix.com"), str(FIX / "ix.par"), str(dest),
        ])
        got = dest.read_bytes()
        exp = golden("ix", "dis8", "compat").replace(b"\n", b"\r\n")
        if got != exp or b"\n" in got.replace(b"\r\n", b""):
            failures.append("dis8 ix --crlf")

    # 64-call DOS golden matches the closed-form listing. A longer chain
    # uses the same shape on the heap walk (and on the recursive compat walk).
    image, expect = call_chain(64)
    if expect != golden("deep", "dis8", "compat"):
        failures.append("deep oracle drifted from the DOS golden")
    image, expect = call_chain(5000)
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        (tmp / "c.com").write_bytes(image)
        (tmp / "c.par").write_text("O 0000\nE 0000\n", encoding="ascii")
        for mode in ("fixed", "compat"):
            got = listing(dis8, mode, ["Z80", str(tmp / "c.com"), str(tmp / "c.par")], tmp / "out.mac")
            if got != expect:
                failures.append("dis8 chain5000 %s" % mode)

    # COM <-> Intel HEX. The end record is the original type-00 line.
    payload = bytes([0xC3, 0x03, 0xFE, 0x3E, 0x20])
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        (tmp / "a.com").write_bytes(payload)
        run([comtohex, str(tmp / "a.com"), str(tmp / "a.hex"), "256"])
        text = (tmp / "a.hex").read_text(encoding="ascii")
        if not hex_checksum_ok(text):
            failures.append("comtohex checksum or end record")
        if ":05010000C303FE3E20" not in text.replace("\r\n", "\n"):
            failures.append("comtohex record bytes")
        run([hextocom, str(tmp / "a.hex"), str(tmp / "b.com")])
        if (tmp / "b.com").read_bytes() != payload:
            failures.append("hextocom round trip")
        # A gap in the address field is not filled; records are concatenated.
        (tmp / "gap.hex").write_text(":020000000102FB\n:01001000AA45\n:0000000000\n", encoding="ascii")
        run([hextocom, str(tmp / "gap.hex"), str(tmp / "gap.com")])
        if (tmp / "gap.com").read_bytes() != bytes([0x01, 0x02, 0xAA]):
            failures.append("hextocom ignores addresses")

    plus_out = run([plus]).stdout.decode("ascii")
    if "outp(F6) <- B4" not in plus_out or "returns false" not in plus_out:
        failures.append("plus dry run")
    if "C3 03 FE 3E 20 D3 00 31 00 00 ED 56 3E C9 32 38 00 FB 76 32 00 FE F3 76" not in plus_out:
        failures.append("plus prog1 image")

    if failures:
        raise SystemExit("failures:\n" + "\n".join(failures))
    print("ok %d listings" % (len(dis8_cases) + len(disz80_cases) + len(dis6800_cases)))


if __name__ == "__main__":
    main()
