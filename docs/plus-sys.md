# What the bytes of `original/PLUS.SYS` are

`original/PLUS.SYS` is not Canopus's shipped binary. It is built from David Izada Rodríguez's disassembly of the original Canopus PLUS.SYS, with his fixes applied.

The file is 19208 bytes (last byte `1A` at offset `4B07`). SHA-256 `868c2113bcecd8e2ca02cf5625307ad7d6f9632f2738c0906ba3ed4de903943a`. `cmp` finds those bytes identical to `original/PLUS.ASM` (same length, same SHA-256). Both begin with the text `CODE SEGMENT` and end with `END` followed by `1A`. `PLUS.ASM` matches `PLUS.SYS`. There is no separate assembled image of `PLUS.ASM` in the tree to compare with `PLUS.SYS`.

15162 bytes are ASCII `20`–`7E`, 4035 are tab, CR, or LF (958 CR+LF pairs), one byte is `1A`, and ten bytes are outside that set: `A2` at `0149`, `A8` at `01EC`, `A1` at `021F`, `A2` at `058D`, `A8` at `0A31`, `A1` at `0A56`, `A2` at `0A86`, `A0` at `1002`, `A2` at `15BF`, `A2` at `184B`. Each of those ten sits in a comment. Read as CP437, `A0` is á, `A1` is í, `A2` is ó, and `A8` is ¿. That reading is the CP437 name for the byte, not a second copy of the file.

The file is assembly source text. It is not an executable image. NDISASM 2.16.01 (`ndisasm -b 16`) and GNU objdump 2.42 (`objdump -b binary -m i386 -M i8086,intel -D`) were both run on the raw bytes. They decode the ASCII. They do not recover a loaded program.

## Device-driver header

An MS-DOS character-device header is 18 bytes at the start of a driver image: next-driver pointer (dword), attributes (word), strategy offset (word), interrupt offset (word), 8-byte name.

Read that way, offset `0000` is:

| Field | Bytes | Value |
| --- | --- | --- |
| Next pointer | `43 4F 44 45` | `45444F43` |
| Attributes | `09 53` | `5309` |
| Strategy | `45 47` | `4745` |
| Interrupt | `4D 45` | `454D` |
| Name | `4E 54 0D 0A 09 41 53 53` | `NT`, CR, LF, tab, `ASS` |

Those bytes are the text `CODE`, tab, `SEGMENT`, CR, LF, tab, `ASS`. Attribute bit 15 is clear (`5309` has no `8000`). The name is not 8 name bytes. This is not a device-driver header.

A scan of every 18-byte window with attribute bit 15 set, a strategy offset and an interrupt offset inside the file, and at least 6 printable bytes in the name field finds three windows:

| Offset | Following bytes |
| --- | --- |
| `021A` | `CALL`, tab, `Load` inside `CALL LoadAndRun` |
| `15BA` | `JMP`, tab, `SHOR` inside `JMP SHORT ReadAndSendN` |
| `1846` | `CALL`, tab, `Sen` inside `CALL Send` |

Each window is source text. None is a driver header.

## Entry points

There is no binary entry point. The first mnemonic in the text, at file offset `0021`, is `JMP Start`. The label `Start:` is at `0046`. Between them, at `0033`, the text is `DB 'Plus Vers 1.2'`.

`ndisasm -b 16` of the first bytes is the ASCII of `CODE SEGMENT`, not that jump:

```
00000000  43                inc bx
00000001  4F                dec di
00000002  44                inc sp
00000003  45                inc bp
00000004  095345            or [bp+di+0x45],dx
```

objdump prints the same first instructions (`inc bx`, `dec di`, `inc sp`, `inc bp`, `or WORD PTR [bp+di+0x45],dx`).

## Strings

Quoted strings stored with `DB`:

| Offset | Text |
| --- | --- |
| `0033` | `Plus Vers 1.2` |
| `032E` | `$   ` (label `Presentacion`) |
| `0346` | `BAD LOAD` inside `0Dh,0Ah,'BAD LOAD$'` |
| `0756` | `PARAMETER ERROR$` |
| `077C` | `NO SPACE$` |

The rest of the readable runs are mnemonics, labels, and comments (`SEGMENT` at `0005`, `ResetPLUS80` at `016C`, `LoadAndRun` at `0228`, and so on).

## I/O instructions

No byte in the file is an 8086 `IN` or `OUT` opcode. The opcodes `E4`, `E5`, `E6`, `E7`, `EC`, `ED`, `EE`, and `EF` occur zero times, so neither disassembler emits `IN` or `OUT`.

The source text writes these 8086 port instructions:

| Offset | Text |
| --- | --- |
| `0EAB` | `OUT 0F6h,AL` |
| `0EC6` | `OUT 0F6h,AL` |
| `0EE5` | `OUT 0F6h,AL` |
| `0F28` | `OUT 0F6h,AL` |
| `1F36` | `IN AL,0F4h` |
| `1F68` | `OUT 0F2h,AL` |
| `1F97` | `IN AL,0F4h` |
| `1FBE` | `IN AL,0F0h` |

The four `OUT 0F6h,AL` are in `Init8255` (`0E8E`) and `ReInit8255` (`0F0A`). The immediates written just before them are `0B4h`, `0Fh`, `CS:Flag`, and `0Eh`. `Send` (`1F21`) masks the `0F4h` read with `2` and writes `AL` to `0F2h`. `Receive` (`1F89`) masks the `0F4h` read with `20h` and then reads `0F0h`.

Separate from those mnemonics, four `DB` lists hold bytes. Lengths below are the numeric bytes written on those `DB` lines (`DUP` and quoted `DB` lines are not included). Z80 `OUT (n),A` is `D3 n`. Z80 `IN A,(n)` is `DB n`. No `ED 40`–`ED 79` block `IN`/`OUT (C)` byte pair occurs in these lists.

`Prog1` at `103E`, 25 bytes. One port instruction: index `006` `D3 00`.

`Prog2` at `265D`, 151 bytes:

| Index | Bytes | Instruction |
| --- | --- | --- |
| `00B` | `D3 00` | `OUT (00h),A` |
| `086` | `D3 00` | `OUT (00h),A` |
| `08A` | `D3 00` | `OUT (00h),A` |
| `08E` | `DB 20` | `IN A,(20h)` |
| `094` | `D3 10` | `OUT (10h),A` |

`Prog3` at `2F5C`, 117 bytes:

| Index | Bytes | Instruction |
| --- | --- | --- |
| `005` | `D3 00` | `OUT (00h),A` |
| `052` | `D3 00` | `OUT (00h),A` |
| `058` | `D3 00` | `OUT (00h),A` |
| `06C` | `DB 20` | `IN A,(20h)` |
| `072` | `DB 10` | `IN A,(10h)` |

`Prog4` at `365B`, 885 bytes:

| Index | Bytes | Instruction |
| --- | --- | --- |
| `079` | `DB 20` | `IN A,(20h)` |
| `080` | `DB 10` | `IN A,(10h)` |
| `09F` | `DB 20` | `IN A,(20h)` |
| `0B1` | `D3 10` | `OUT (10h),A` |
| `0BD` | `D3 01` | `OUT (01h),A` |
| `0C3` | `D3 01` | `OUT (01h),A` |
| `32F` | `DB 20` | `IN A,(20h)` |
| `335` | `DB 10` | `IN A,(10h)` |
| `339` | `DB 20` | `IN A,(20h)` |
| `33F` | `D3 10` | `OUT (10h),A` |

The source's own count immediates match three of the lists: `CX,97h` is 151 (`Prog2`), `CX,75h` is 117 (`Prog3`), `CX,375h` is 885 (`Prog4`). `ResetPLUS80` has `MOV CX,18h` (24) and `MOV SI,OFFSET Prog1`, while the `Prog1` `DB` list is 25 bytes. The first of those 25 is `0Dh` at `103E`. The next line, at `1070`, begins `C3 03 FE`. The comment under that list begins `JP LFE03`, which is `C3 03 FE`. Which of the 25 bytes the `18h` count leaves out is not fixed by the text.

## Dispatch table

`Table` at `1FE1` is 51 lines of `DW OFFSET`, commented `00` through `32`. The file has 55 `DW` lines in all; the other four are `__DS`, `__SS`, `_8DS`, and `L08A8`. At `19B2` the text is `CMP CL,33h`. At `19F1` it is `CALL WORD PTR CS:[BX + Table]`. That is a 51-entry near-pointer table indexed by a value the text compares with `33h`. It is not 256 entries, and the entries are 8086 labels (`Terminate`, `StatusServices`, `DiskServices`, `CallBIOS`, and the others named on those lines), not Z80 opcodes.

Inside the `Prog4` bytes, index `102` starts 17 consecutive `C3` instructions (Z80 `JP nn`). The targets, in order, are `FB33`, `FB9A`, `FBC4`, `FBB2`, `FBBE`, `FBB8`, `FBBB`, `FBB5`, `FBB1`, `FBAC`, `FBB1`, `FBB1`, `FBB1`, `FBAF`, `FBAF`, `FBC1`, `FBB1`. `Prog4` contains 22 `C3` bytes in total. `Prog1`, `Prog2`, and `Prog3` contain one `C3` each. No `DB` list contains 256 `JP` instructions or 256 words. Whether those 17 `JP` instructions are used as a table is not settled by their being consecutive.

The `DB` lists do contain other Z80 opcodes (`ED` prefixes, `76` `HALT`, `ED B0` `LDIR`). The file also contains commented Z80 text (`.Z80`, `LD`, `OUT`, `HALT`) under `Prog1`, `Prog2`, and `Prog3`. That is Z80 machine code stored as hex constants, plus a comment listing of part of it. There is no 256-way Z80 or 8080 opcode dispatch table in the bytes, and the 8086 mnemonics do not include a loop that fetches an opcode and indexes a 256-entry table.
