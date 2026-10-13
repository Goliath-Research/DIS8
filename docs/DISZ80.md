# DISZ80

UTF-8 transcription of `original/DISZ80.DOC`. The Spanish text is the original. There is no `DISZ80.EXE` in this archive; the port is `disz80`.

## Español

### Disensamblador Z80

Uso de DISZ80:

```
DISZ80 <.COM> <.PAR> <.MAC> [<.SYM>]
```

El fichero de parámetros está formado por líneas con la siguiente estructura:

```
<Comando><Parámetros del Comando>
```

Los posibles comandos son (se escribe sólo el primer carácter):

| Comando | Parámetros | Significado |
| --- | --- | --- |
| Offset | `<hex>` | Dirección de carga del programa |
| Entry | `<hex>` | Posible punto de entrada del programa en `<hex>` |
| Break | `<hex>` | Interrumpir la exploración automática en `<hex>` |
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

## English

```
disz80 [--compat] [--crlf] <file.com> <file.par> <file.mac> [file.sym]
```

This is the Z80-only disassembler of 19-10-88 (version 3.2). It has no eXtract command and no Help comment command. The other parameter commands match DIS8. See [par.md](par.md).

`--compat` repeats the 1988 walk, including the missing IX/IY displacement described in [known-issues.md](known-issues.md). The default walk is the corrected one. `--crlf` writes CR+LF.

The banner is the original one: Z-80 DISASSEMBLER Versión 3.2 19-10-88, Lic. David Izada Rodríguez, Instituto de Informática, Universidad Central de Las Villas.
