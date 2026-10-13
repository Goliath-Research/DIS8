# DIS8 and DIS6800

UTF-8 transcription of `original/DIS8.DOC`. The Spanish text is the original. The English section translates it. The parameter-file details that the 1990 source actually implements are in [par.md](par.md).

## Español

### Disensamblador 8080, 8085 y Z80

Uso de DIS8:

```
DIS8 [8080|8085|Z80] <.COM> <.PAR> <.MAC> [<.SYM>]
```

El fichero de parámetros está formado por líneas con la siguiente estructura:

```
<Comando><Parámetros del Comando>
```

Los posibles comandos son (se escribe sólo el primer carácter):

| Comando | Parámetros | Significado |
| --- | --- | --- |
| eXtract | `<hex>` | Extraer el programa desde `<hex>` en el fichero |
| Offset | `<hex>` | Dirección de carga del programa |
| Entry | `<hex>` | Posible punto de entrada del programa en `<hex>` |
| Break | `<hex>` | Interrumpir la exploración automática en `<hex>` |
| Help | `<hex> <texto>` | Comentario para el fuente |
| Label | `<hex>` | Generar una etiqueta en `<hex>` |
| String0 | `<hex>` | String terminada en 00h comienza en `<hex>` |
| String$ | `<hex>` | String terminada en `$` comienza en `<hex>` |
| Word | `<hex1> <hex2>` | Words desde `<hex1>` hasta `<hex2>` |
| DS | `<hex1> <hex2>` | Generar DS para el rango `<hex1>` hasta `<hex2>` |
| C | `<hex1> <hex2>` | Poner el código como comentario |
| Upper | `<hex>` | Define el límite superior del código |

Por ejemplo:

```
O 0100
E 0100
S0 0179
S0 018B
S0 019A
S0 01AA
S0 01B9
E 0290
E 0355
S0 0386
S0 03C1
S0 069D
S0 06AA
W 06B0 06B3
D 7BF7 7FFF
C 6C00 7C00
```

El fichero de símbolos opcional puede ser generado por el LINKMT o con un editor. Está formado por líneas con el siguiente formato:

```
<dirección hex>    <nombre de la variable de hasta 6 caracteres>
```

### Disensamblador 6800

```
DIS6800 <.COM> <.PAR> <.MAC> [<.SYM>]
```

El fichero de parámetros tiene la misma estructura que el anterior.

## English

### 8080, 8085 and Z80 disassembler

```
dis8 [--compat] [--crlf] [8080|8085|Z80] <file.com> <file.par> <file.mac> [file.sym]
```

The parameter file is one command per line. Only the first character of the command name is read. The commands are eXtract, Offset, Entry, Break, Help, Label, String0, String$, Word, DS, C, and Upper. The example in the Spanish section is the one printed in `DIS8.DOC`. `original/L80.PAR` is the same list written without spaces (`O0100`, `S00179`).

The optional symbol file can come from LINKMT or from an editor. Each line is a hex address and a name of up to six characters.

### 6800 disassembler

```
dis6800 [--compat] [--crlf] <file.com> <file.par> <file.mac> [file.sym]
```

The parameter file has the same structure. DIS6800 has no eXtract command.

`--compat` repeats the 1990 control-flow walk. `--crlf` writes CR+LF, which is what DOS text mode wrote. The default walk is the corrected one described in [known-issues.md](known-issues.md), and the default line ending is LF.

The banner printed by the 1990 program, and by this port, is version 5.2 (28-12-90) for DIS8 and version 1.0 (24-12-90) for DIS6800, Lic. David Izada Rodríguez, Instituto de Informática, Universidad Central de Las Villas.
