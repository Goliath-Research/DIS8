# Parameter file (.PAR)

DIS8, DISZ80, and DIS6800 read the same kind of parameter file. A command is the first character of a token. The rest of that token and the following numbers are the arguments. Spaces are optional: `O 0100` and `O0100` are the same command. Letters may be upper or lower case. A semicolon is not a comment; the examples in the 1990 manual are documentation, not syntax the program skips.

Hexadecimal fields are scanned with `%x`. A symbol name is at most six characters.

The commands are applied in order, as a script. `E` calls the code walk immediately, so a `D`, `S`, `W`, or `C` that appears earlier is already in the type table when the walk runs, and one that appears later overwrites the code bits the walk just set.

| Letter | Arguments | Effect |
| --- | --- | --- |
| `O` | address | Load address (origin). The default in the source is `0100h`. Printed as `ORG`. |
| `E` | address | Mark that byte as a label and walk every instruction reachable from it. |
| `B` | address | Stop the automatic walk at this address. See [known-issues.md](known-issues.md): DIS8 and DISZ80 used to decode the break byte when a jump landed on it. |
| `L` | address | Force a label. |
| `X` | file offset | DIS8 only. Slide the image down so the byte at this file offset becomes the first byte. The offset is not an address. The 1990 source does not also shrink the loaded length. DISZ80 and DIS6800 have no `X` command. |
| `S` | `0` or `$`, then address | String. `S0` ends at a `00` byte. `S$` ends at `$`. Only the first byte is marked as a string (and as a label). The printer reads the following bytes until the terminator. |
| `W` | start, end | Words. Each pair is cleared as code; the first byte of the pair is a word (`DW`). |
| `D` | start, end | Reserve the range (`DS`). |
| `C` | start, end | Print the range as bytes in a comment, not as instructions. |
| `U` | address | Upper limit. `--compat` sets the limit to `address - origin`, which can extend into the zero bytes past the file (clamped to `0xFF00`). The default clamps it to the loaded size, and treats an address below the origin as an empty image. |
| `H` | address, rest of the line | Comment printed when that address is reached. DIS8 and DIS6800 only. DISZ80 has no `H` command. |

Addresses on `E`, `B`, `L`, `S`, `W`, `D`, `C`, and `H` are CPU addresses. The file offset is `address - origin`. `X` is the exception: its number is a file offset.

The optional `.SYM` file is one symbol per line:

```
0103 FOO
```

The name replaces the generated `Lxxxx` label at that address. Names are truncated to six characters, matching LINKMT as described in `DIS8.DOC` and `DISASM.CHI`.

## What the walk treats as code

From each `E` (and from each branch the walk decides to follow) the program marks bytes as code until it hits a byte already marked code, a break, a return or unconditional jump, or an instruction it will not step into. Conditional jumps, calls, and `RST` are followed and also fall through. `JP (HL)` stops without a target. The bugs in that walk, and the `--compat` switch that reproduces them, are written out in [known-issues.md](known-issues.md).

## Listing

The `.MAC` file is M80-style text: a CPU directive (`.Z80`, `.8080`, or `.8085` on DIS8; DIS6800 prints only `ORG`), then `ORG`, then instructions and `DB` / `DW` / `DS` lines, then `END`. Generated labels are `L` plus four hex digits, with no `H`. Immediate hex constants end in `H`, and a leading `0` is inserted when the first digit would be `A`–`F`. The DOS programs wrote CR+LF because they used text mode. Pass `--crlf` for that. The default is LF.
