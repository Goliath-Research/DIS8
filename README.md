# DIS8

Portable C port of the 8080, 8085, Z80, and 6800 disassembler written by Lic. David Izada Rodríguez at the Instituto de Informática, Universidad Central de Las Villas.

The programs print their own dates: DISZ80 version 3.2 (19-10-88), DIS8 version 5.2 (28-12-90), DIS6800 version 1.0 (24-12-90). `original/DISASM.CHI` says the disassembler was written in Microsoft C 5.10, that it disassembles the hidden Z80 instructions, that COMtoHEX and HEXtoCOM convert the `.HEX` files used by emulators, and that the symbol file matches the one produced by LINKMT.

The files under [`original/`](original/) are the author's sources and binaries, kept byte for byte. Nothing in that directory is modified by this port.

## History

The code has two pieces: the disassembler and the simulator.

DIS8 is the disassembler, in a few incarnations, mainly for the 8080, 8085, and Z80. The Motorola 6800 version (DIS6800) was a special case.

PLUS is the small host program that loaded (downloaded) Z80 code onto the PLUS-80 card so the card's Z80 ran it. The card, made by Canopus Electronics for the NEC PC-9801 series, had its own Z80. The PC-9801's 8086 acted as host and handled I/O. [`PLUS.SYS`](original/PLUS.SYS) was the alternative used on the NEC PC-9801 when the PLUS-80 card was not present: a software fallback. `original/PLUS.SYS` is not Canopus's shipped binary. It is built from David Izada Rodríguez's disassembly of the original Canopus PLUS.SYS, with his fixes applied. That fixed version was later used for the simulator (the 1990 debugger work). [`original/PLUS.ASM`](original/PLUS.ASM) is the same 19208 bytes.

Using both tools, and with access to the PLUS-80 card, another team created a debugger similar to Turbo Debugger for Z80 programs on MS-DOS in 1990.

## Programs

| Port | Original | Role |
| --- | --- | --- |
| `dis8` | `DIS8.C` | 8080, 8085, and Z80 |
| `disz80` | `DISZ80.C` | Z80 only (no DOS executable is in the archive) |
| `dis6800` | `DIS6800.C` | 6800 |
| `comtohex` | `COMTOHEX.C` | `.COM` to the HEX records described in `CPM.DOC` |
| `hextocom` | `HEXTOCOM.C` | HEX data records concatenated into a `.COM` |
| `plus` | `PLUS.C` | Dry run of the PLUS-80 loader fragment. It does not touch hardware. |

```
dis8 [--compat] [--crlf] [8080|8085|Z80] file.com file.par file.mac [file.sym]
disz80 [--compat] [--crlf] file.com file.par file.mac [file.sym]
dis6800 [--compat] [--crlf] file.com file.par file.mac [file.sym]
```

`--compat` repeats the original control-flow walk, including the bugs in [docs/known-issues.md](docs/known-issues.md). The default walk is the corrected one. `--crlf` writes CR+LF, which is what DOS text mode wrote. The default is LF.

The `.PAR` command file is documented in [docs/par.md](docs/par.md).

## Build

C11, CMake 3.16 or newer.

```
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The build uses `-Wall -Wextra -Werror` with GCC and Clang, and `/W3` with MSVC.

## Tests

`tests/regression.py` compares `dis8` and `dis6800` with `--compat` to listings produced by `original/DIS8.EXE` and `original/DIS6800.EXE` under DOSBox-X. Those goldens live in `tests/golden/` with LF line endings. The same cases are also run without `--compat`, against a separate golden. The two outputs are not interchangeable. See [docs/known-issues.md](docs/known-issues.md) for the smallest input that shows each difference.

`original/TSRE0.COM` with a one-entry parameter file is identical in both modes. `original/EMZ.COM` with `original/L80.PAR` and with `original/MSX.PAR` is not: the corrected walk stops at bytes the parameter file already marked as data.

DISZ80 compat listings are checked against the 1988 source. There is no DOS executable to run.

`comtohex` / `hextocom` are checked by a round trip and by the checksum rule in `CPM.DOC`. The end record is the original `:0000000000`.

## Documentation

| File | Source |
| --- | --- |
| [docs/DIS8.md](docs/DIS8.md) | `DIS8.DOC` |
| [docs/DISZ80.md](docs/DISZ80.md) | `DISZ80.DOC` |
| [docs/CPM.md](docs/CPM.md) | `CPM.DOC` |
| [docs/DISASM.md](docs/DISASM.md) | `DISASM.CHI`, ChiWriter font codes removed |
| [docs/par.md](docs/par.md) | Parameter-file format |
| [docs/known-issues.md](docs/known-issues.md) | Control-flow bugs, with original and corrected listings |
| [docs/plus-sys.md](docs/plus-sys.md) | What the bytes of `original/PLUS.SYS` contain |

Each document converted from the original files has the Spanish text and an English translation. `docs/plus-sys.md` is an English note on the bytes in `original/PLUS.SYS`.

## Notice

`original/` is kept complete. Some of those files are not the author's work, or they contain another party's runtime.

- `DIS8.EXE` and `DIS6800.EXE` contain the string `MS Run-Time Library - Copyright (c) 1990, Microsoft Corp`, and the author's name.
- `RPM.COM` contains `(C) dci software 26-10-82` and `SImple MONitor`. It is a third-party CP/M program.
- `HARD.COM` has no copyright string in the binary. It may not be the author's.
- `EMZ.COM`, `EMZ.SYS`, and `EMZ.CPM` are the MEGASOFT 64K CP/M-80 emulator for the PC-9801 (strings include `MEGASOFT`, `EM/Z for PC-9801/F/E`, and `Copyright (C) MEGASOFT`).
- `PLUS.SYS` contains `Plus Vers 1.2`. The comments in `PLUS.C` are the author's. The port does not run that loader against hardware.
- The `P*.OBJ` files are Microsoft 8086 relocatable objects.

Sample `.ASM` files in `original/` are 8086-style sources. They are not listings produced by DIS8. The simulator-side sources are [`EMZ.ASM`](original/EMZ.ASM), [`EMZCPM.ASM`](original/EMZCPM.ASM), [`EMZPROC.ASM`](original/EMZPROC.ASM), and [`PLUS.ASM`](original/PLUS.ASM).

## License

MIT. Copyright (c) 1988-2026 David Izada Rodriguez. See [LICENSE](LICENSE).

The Microsoft runtime inside the original executables, and the third-party binaries listed above, keep their own copyrights. The MIT license covers this port and the author's sources.
