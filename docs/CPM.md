# M80, L80, and the HEX format

UTF-8 transcription of `original/CPM.DOC`. The Spanish text is the original. It is a short note on the CP/M assembler and linker that the disassembler's symbol files and HEX tools were written to sit next to.

## Español

### M80

```
M80 [<.REL>,<.PRN>=<.MAC>/R/L/Z/I]
```

- R — generar REL
- L — generar PRN
- Z — Z80
- I — INTEL 8080

Información en la tabla de símbolos:

- I — público o global
- U — indefinido
- `*` — externo
- `'` — valor reubicable de programa
- `"` — valor reubicable de dato
- `!` — valor reubicable de directiva COMMON

Errores:

| Código | Significado |
| --- | --- |
| A | Error de argumento |
| C | Anidado condicional en macro |
| D | Símbolo definido doble |
| E | Uso ilegal del EXTERNAL |
| M | Símbolo multidefinido |
| N | Error numérico |
| O | Código de operación erróneo o sintaxis cuestionable |
| P | Error de fase |
| Q | Sintaxis cuestionable |
| R | Error de reubicación |
| W | Símbolo indefinido |
| V | Error de valor |

### L80

```
L80 [<REL>/opciones,...]
```

- `/P:xxxx` — dirección inicial del programa (0103h)
- `/D:xxxx` — dirección inicial de los datos (delante del código)
- `/N` — identifica el nombre del programa ejecutable
- `/S` — búsqueda selectiva en bibliotecas
- `/E` — retornar al sistema operativo
- `/X` — generar `.HEX`

Formato `.HEX`. Líneas con la siguiente estructura:

```
:<LL><DDDD>00<dd><dd>...<dd><SS>
```

- LL — longitud en bytes
- DDDD — dirección donde se ubicarán los bytes
- dd — bytes de información
- SS — suma de control (debe sumar 0 toda la línea)

Y la última línea será:

```
:0000000000
```

## English

M80 is the CP/M macro assembler. The switches above select REL output, a PRN listing, Z80 mnemonics, or Intel 8080 mnemonics. The symbol-table letters and the error letters are the ones listed in the Spanish section.

L80 is the linker. `/P` and `/D` set the program and data origins, `/N` names the executable, `/S` searches libraries selectively, `/E` returns to the operating system, and `/X` writes a HEX file.

The HEX record in this note is Intel HEX with type `00` and an 8-bit checksum of the whole line (length, address, type, and data), which must be zero. The end record printed here is `:0000000000`. That is type `00` with a zero length, not the later type `01` end-of-file record. `comtohex` writes that end record. `hextocom` ignores addresses and concatenates the data bytes, which is what `HEXTOCOM.C` did, so a round trip does not keep the load address and does not fill a gap between records.

`comtohex` takes a decimal load address, as the original `atoi` call did:

```
comtohex [--crlf] <file.com> <file.hex> [decimal address]
hextocom [--crlf] <file.hex> [file.com]
```

The address is optional in the port. The 1990 program always read `argv[3]`.
