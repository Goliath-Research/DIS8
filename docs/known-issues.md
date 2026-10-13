# Known issues in the 1988–1990 code walk

The recursive walk that separates code from data is preserved with `--compat`. The default walk is the corrected one. `tests/golden/*.dis8.compat.mac` and `tests/golden/*.dis6800.compat.mac` are the listings produced by `original/DIS8.EXE` and `original/DIS6800.EXE` under DOSBox-X (line endings normalized to LF). There is no `DISZ80.EXE` in this archive; the DISZ80 compat listings follow the 1988 source.

Do not replace a compat golden with the fixed listing. The two files are checked separately.

## Decisions that were already right

- An unconditional jump sets the stop flag (`RET` on 8080/Z80, `Stop` on 6800), so the walk does not fall through. `CALL`, `RST`, and conditional branches do fall through.
- `JP (HL)` / `PCHL` (`E9`) stops and does not invent a target. `E9 C9` with `ORG 0100` is `JP (HL)` plus `DB 0C9H` in both modes, and that listing matches `DIS8.EXE`.
- `RST n` targets `n * 8` (`opcode - C7`, then minus the origin) and falls through. With `ORG 0` the vector is inside the image. `C3 08 00` followed by `RST 0` and `RET` matches `DIS8.EXE` in both modes.
- A `JR` / `DJNZ` / 6800 relative displacement is `PC + 2 + signed 8-bit`. `18 FE C9` is `JR L0100` and the `RET` is data, in both modes.
- An absolute target is a 16-bit subtraction from the origin. A result that wraps inside the loaded window is still followed.
- A chain of 64 `CALL`s matches `DIS8.EXE` in both modes. The regression test rebuilds that listing from the addresses and also runs a chain of 5000 calls.

## 1. DIS8 measured an indexed instruction with the wrong byte

`LENIx` is called with `PC` already on the opcode. The 1990 source asked whether `Table[PROG[PC+1]]` contains `"(i"`. That byte is the displacement, not the opcode. The printer asks the same question while `PC` is still on the `DD`/`FD` prefix, so it sees the opcode and the printed length is right.

`DD 36 05 C3` is `LD (IX+05H),0C3H` (4 bytes). The displacement `05` is `DEC B`, whose text has no `"(i"`, so the walk used length 3. The immediate `C3` was then an unconditional `JP` and the final `RET` was never reached. The printer still consumed 4 bytes, so the listing shows `LD` and then `DJNZ`, and leaves `C9` as data.

Bytes `DD 36 05 C3 10 01 C9`. Parameters `O 0100` / `E 0100`. CPU Z80.

Original (`DIS8.EXE`, and `dis8 --compat`):

```
	.Z80

	ORG	0100H

L0100:	LD	(IX+05H),0C3H
	DJNZ	L0107
	DB	0C9H			;0106H

	END
```

Corrected (`dis8`):

```
	.Z80

	ORG	0100H

L0100:	LD	(IX+05H),0C3H
	DJNZ	L0107
	RET

	END
```

`DD 36 05 00 C9` does not show this in DIS8: the printer and a linear fall-through still reach the `RET`. It does show the next bug.

## 2. DISZ80 omitted the IX/IY displacement

In the 1988 source the extra byte is commented out in both `LENIx` and `DISASMIx` (`Size + 1` only). The printer and the walker agree with each other and both skip the displacement, so that byte is decoded again. DIS8's printer kept the byte; DISZ80's did not. No DOS binary of DISZ80 is in the tree.

Bytes `DD 36 05 00 C9`. Parameters `O 0100` / `E 0100`.

`disz80 --compat`:

```
	.Z80

	ORG	0100H

L0100:	LD	(IX+05H),00H
	NOP
	RET

	END
```

`disz80`:

```
	.Z80

	ORG	0100H

L0100:	LD	(IX+05H),00H
	RET

	END
```

The same bytes under `DIS8.EXE` are `LD` then `RET`, with no `NOP`. That is `ix2.dis8.compat.mac`.

## 3. `DDCB` / `FDCB` length used the wrong operator

The source says `(op >> 3 + 2) % 8`. `+` binds tighter than `>>`, so that is `(op >> 5) % 8`. The printer accepts the indexed CB form when `(op + 2) % 8 == 0`, that is when the register field is `(HL)`. The two tests agree only for opcodes below `20h`.

`DD CB 00 36` is `SLL (IX+00H)` (opcode `36h`). The walk's length was 1, so it never saw the `JP` after the instruction and fell through into the `RET`. The corrected length is 4. The `JP` is unconditional, and the `RET` is data. The printer was already aligned, which is why both listings show `SLL` and `JP`.

Bytes `DD CB 00 36 C3 00 01 C9`. Parameters `O 0100` / `E 0100`.

Original (`DIS8.EXE`; `disz80 --compat` prints the same):

```
	.Z80

	ORG	0100H

L0100:	SLL	(IX+00H)
	JP	L0100
	RET

	END
```

Corrected (`dis8` and `disz80`):

```
	.Z80

	ORG	0100H

L0100:	SLL	(IX+00H)
	JP	L0100
	DB	0C9H			;0107H

	END
```

`--compat` still evaluates `(op >> (3 + 2)) % 8`, which is the original grouping. A second byte above `BBh` on an `ED` prefix used to index off the end of `TableED` in DISZ80. Both modes now reject that the way DIS8 already did. That read is not something the DOS program can be asked to print.

## 4. 8085 `RIM` and `SIM` were traced as `JR`

Opcodes `20h` and `30h` are `RIM` and `SIM` when the CPU is 8080 or 8085. `Size()` already returned 1. `JMP()` still saw the Z80 `JR NZ` / `JR NC` and consumed the next byte as a displacement. `20 00 C9` planted a label on the `RET`. The `.8085` / `.8080` lines around `RIM` in an 8080 listing are unchanged. Z80 mode still traces `20h` as `JR NZ`.

Bytes `20 00 C9`. Parameters `O 0100` / `E 0100`. CPU 8085.

Original (`DIS8.EXE`):

```
	.8085

	ORG	0100H

L0100:	RIM
	NOP
L0102:	RET

	END
```

Corrected:

```
	.8085

	ORG	0100H

L0100:	RIM
	NOP
	RET

	END
```

## 5. Three 6800 opcodes had the wrong branch flags

The 256-entry table stores:

| Opcode | Mnemonic | 1990 flags | What the instruction does |
| --- | --- | --- | --- |
| `20` | `BRA r` | absolute, not relative, does not stop | relative unconditional branch |
| `7E` | `JMP e` | absolute, does not stop | absolute jump, no fall-through |
| `AD` | `JSR b,X` | not a jump, stop | indexed call, target unknown, fall through |

`BRA` therefore read a big-endian word (the displacement byte plus the next opcode) and then fell through. `JMP` extended followed the right address and also fell through. `JSR` indexed stopped, so the bytes after the call were data.

Left as stored, because they already match the instruction: conditional branches `22`–`2F` (relative, fall through), `BSR` (`8D`), `RTS` (`39`) and `RTI` (`3B`), `JMP` indexed (`6E`, stop, no known target), `JSR` extended (`BD`, absolute, fall through).

`main` patches `20`, `7E`, and `AD` unless `--compat`, and it does that before the parameter file is read (`E` calls the walk immediately).

Bytes `20 FE 39`. Parameters `O 0100` / `E 0100`.

Original (`DIS6800.EXE`):

```
	ORG	0100H

L0100:	BRA	L0100
	RTS

	END
```

Corrected (`BRA` is a branch to itself, so `RTS` is not reached):

```
	ORG	0100H

L0100:	BRA	L0100
	DB	39H			;0102H '9'

	END
```

Bytes `7E 01 06 86 00 00 39` (`JMP $0106`, then `LDAA #$00`, a `00`, `RTS`):

Original:

```
	ORG	0100H

L0100:	JMP	L0106
	LDAA	00H
	DB	00H			;0105H
L0106:	RTS

	END
```

Corrected (the `JMP` does not fall through; `RTS` is still reached by the jump):

```
	ORG	0100H

L0100:	JMP	L0106
	DB	86H			;0103H
	DB	00H			;0104H
	DB	00H			;0105H
L0106:	RTS

	END
```

Bytes `AD 00 39`:

Original:

```
	ORG	0100H

L0100:	JSR	00H,X
	DB	39H			;0102H '9'

	END
```

Corrected:

```
	ORG	0100H

L0100:	JSR	00H,X
	RTS

	END
```

## 6. One address past the image, and `U`

The walk continued while `PC <= LIMIT`, and a label was planted while `p <= LIMIT`. `LIMIT` is the number of loaded bytes, so both tests accept the address one past the last byte. An instruction that did not fit was completed from the zero fill past the file. `U` replaced `LIMIT` with `address - origin`, which extends that zero fill. The DOS program could also index its 32768-byte type array off the end; `--compat` clamps that limit to `0xFF00` and does not reproduce the smash.

The corrected walk follows and labels only when the file offset is strictly less than `LIMIT`, and it does not mark an instruction that does not fit in `[0, LIMIT)`. `U` is clamped to the loaded size. `U` below the origin sets `LIMIT` to 0. A 16-bit wrap that lands inside the loaded window is still followed.

Bytes `00 C3`. Parameters `O 0100` / `E 0100`. The `JP` needs two operand bytes that were not loaded.

Original (`DIS8.EXE`, operands read as zeros, target `0000`):

```
	.Z80

	ORG	0100H

L0100:	NOP
	JP	L0000

	END
```

Corrected:

```
	.Z80

	ORG	0100H

L0100:	NOP
	DB	0C3H			;0101H

	END
```

Bytes `00 00`. Parameters `O 0100` / `U 0104` / `E 0100`.

Original (two extra zero `NOP`s):

```
	.Z80

	ORG	0100H

L0100:	NOP
	NOP
	NOP
	NOP

	END
```

Corrected:

```
	.Z80

	ORG	0100H

L0100:	NOP
	NOP

	END
```

## 7. DIS8 and DISZ80 noticed `Break` one instruction late

`DIS6800` tests `Break` at the top of the loop, so the break address itself is not decoded. DIS8 and DISZ80 tested it only after advancing, so a jump or an entry that landed on the break address was still decoded. Linear fall-through onto a break stops before that byte in both walks. The corrected walk tests `Break` on entry for all three programs.

Bytes `C3 03 01 C9`. Parameters `O 0100` / `B 0103` / `E 0100`.

Original (`DIS8.EXE`):

```
	.Z80

	ORG	0100H

L0100:	JP	L0103
L0103:	RET

	END
```

Corrected (the `JP` still labels `0103`; the byte is not code):

```
	.Z80

	ORG	0100H

L0100:	JP	L0103
L0103:	DB	0C9H

	END
```

## 8. Bytes already marked as data were not a barrier

The walk stopped only when `Code` was already 1. An `E` processed after `D`, `S`, `W`, or `C` set `Code` on bytes those commands had classified. `S` marks only the first byte of the string. `W` sets `Data` on the first byte of each word and clears `Code` on both bytes. The corrected walk does not enter a byte with `Data != 0` and does not let an instruction span one.

Parameter order is still a script. A `D` that comes after `E` clears the code bits the walk just set. The barrier matters when the data marks are already in the table when the walk runs. The sample parameter files mostly put `D` and `C` after `E`, which is why several of them do not change.

Bytes `00 00 41 42 C9`. Parameters `O 0100` / `D 0102 0103` / `E 0100`. CPU 8080.

Original (`DIS8.EXE`):

```
	.8080

	ORG	0100H

L0100:	NOP
	NOP
	MOV	B,C
	MOV	B,D
	RET

	END
```

Corrected (`C9` was not itself marked as data, so it is a `DB`):

```
	.8080

	ORG	0100H

L0100:	NOP
	NOP
	DS	0002H
	DB	0C9H			;0104H

	END
```

`original/EMZ.COM` with `original/L80.PAR`: the only change is the `S0160B` string. `--compat` decoded the following zeros as `NOP`. The default leaves them as `DB`. `original/MSX.PAR` has the same kind of change after `W 39DE 3A73`: those bytes are not decoded as instructions, and the entry at `3A88` is still code. `original/TSRE0.COM` with `O 0100` / `E 0100` is identical in both modes.

## 9. `ED` addresses were looked up in the wrong slot

`TableED` is based at opcode `40h`. `DAT()` indexed it with the raw second byte, so `ED 43` (`LD (nn),BC`) consulted the empty slot at index `43h` instead of index `3`. The printer subtracts `40h`, so the address was printed, but the target byte was not labelled. An index past `7Bh` also walked off the `0x7C`-entry array. That read is not reproduced in either mode. The corrected `DAT()` subtracts `40h`.

Bytes `ED 43 06 01 00 00 00 C9`. Parameters `O 0100` / `E 0100`.

Original (`DIS8.EXE`):

```
	.Z80

	ORG	0100H

L0100:	LD	(L0106),BC
	NOP
	NOP
	NOP
	RET

	END
```

Corrected:

```
	.Z80

	ORG	0100H

L0100:	LD	(L0106),BC
	NOP
	NOP
L0106:	NOP
	RET

	END
```

## 10. The walk recursed on every branch

`Travel` calls itself for every taken branch. A long enough chain overflows the DOS stack. `--compat` keeps that recursion. The default walk stores pending addresses on a heap array of 65536 entries and explores the branch target before the fall-through, which is the same order. The 64-call golden matches `DIS8.EXE`. The regression test also runs 5000 calls in both modes; the listing is the same closed form (`Lxxxx: CALL Ly yyy` for each block, then `RET`).

## Other original defects, outside the walk

`PLUS.C` is not a hardware port. `SegOk` stored the logical not of a byte and treated the segment as present when the value did not change. The address `(unsigned long)seg << 16 + 0xFE00` is parsed as `seg << (16 + 0xFE00)`. `plus` uses `(segment << 16) + 0xFE00`, prints the 8255 sequence, and copies the 24-byte `Prog1` image from `original/P1.MAC`. It does not touch port `0xF6`. `ResetPLUS80` returned true only after the Z80 had overwritten `FE00` with `C9`. That image starts with `C3`, so a dry run returns false. The process exits 0 when the copied bytes match `Prog1`.

`COMTOHEX` always read `argv[3]`. The port treats the decimal load address as optional. The end record stays the original `:0000000000` (type `00`), not the usual type `01`. `HEXTOCOM` wrote a NUL one past its buffer and ignored record addresses, concatenating data bytes. The port allocates one extra byte and still ignores addresses, so a round trip does not preserve the load address or fill gaps.
