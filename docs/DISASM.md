# Eight-bit program disassembler

Transcription of `original/DISASM.CHI` (ChiWriter). Font changes, rules, and page furniture are removed. The Spanish text keeps the spelling of the ChiWriter file, including its typos. The English section is a translation of that text.

## Español

              DESENSAMBLADOR DE PROGRAMAS DE 8 BITS
Autor :
    Lic. David Izada Rodríguez        Inst. Informática UCLV.
INDICE :
1 .- Resumen                                           3
2 .- Introducción                                      4
3 .- Desarrollo                                        5
4 .- Conclusiones                                      9
5.- Bibliografía                                      10
RESUMEN :
    Este programa facilita el desensamblaje  de  programas  de  8
bits escritos originalmente en  8080,  8085  ó  Z-80  mediante  un
método  de  búsqueda  automática  de  la  posible  estructura  del
programa. Permite al usuario además definir un fichero de símbolos
para obtener el programa fuente en un  estado  más  legible  (este
fichero de símbolos es compatible con el generado por el  LINKMT),
enlazador de CPM. El programa fue escrito  en  Microsoft  C  y  es
capaz de desensamblar las instrucciones  ocultas  del  Z-80.
    Para permitir el uso de emuladores (que generalmente utilizan
el formato .HEX) se entregan dos programas:  COMtoHEX  y  HEXtoCOM
que efectúan la conversión de formatos.
INTRODUCCION :
    Existen programas de este tipo para el trabajo con  lenguajes
ensambladores 16 bits, pero todavía se usan mucho los  microproce-
sadores de 8 bits en dispositivos electrónicos en aplicaciones in-
dustriales de control óptimo. En estos momentos no se cuentan  con
las herramientas necesarias para hacer que el  trabajo  con  estos
dispositivos se haga lo más cómoda posible.
    El presente trabajo es parte de un conjunto de soluciones que
se le están comenzando a dar a estos  problemas  y  uqe  se  están
desarrollando  con  la  óptica  de  poner  a  punto  los  sistemas
realizando su trabajo de puesta a punto casi completa en  máquinas
de 16 bits, aumentando mucho las comodidades de  este  proceso,  y
que sólo la parte de montaje necesite de otras herramientas.  Este
programa, por tanto, está elaborado en MS-C versión  5.10  permite
realizar todo el proceso  de  desensamblaje  a  programas  que  se
quieran mejorar y no se cuente con el listado fuente, para  usarlo
en una aplicación dada, o tal ves, sólo  se  necesite  obtener  el
fuente para saber como trabaja  determinado  sistema,  factor  muy
importante teniendo en cuenta que muchas veces se reciben  equipos
de los cuales no se tiene documentación alguna.
DESARROLLO :
    Los desensambladores son una parte necesaria del  trabajo  de
un programador, ya que son programas  que  obtiene  a  partir  del
código de máquina de algún microprocesador, un programa fuente  en
lenguaje ensamblador o nemotécnico. El lenguaje  ensamblador  está
muy cercano al lenguaje de máquina , en realidad muchas sentencias
del  lenguaje  ensamblador  no  son   más   que   representaciones
simbólicas de  sentencias  de  lenguaje  de  máquina.  Además  las
sentencias de ensamblador suelen tener un formato fijo,  que  hace
que sea, relativamente más fácil el analizarlas, se puede decir en
este caso, que sea posible el proceso  de  desensamblaje,  ya  que
hasta el momento no se conoce de ningún sistema que  logre  llevar
de código de máquina a algún  lenguaje  de  alto  nivel.  Esto  se
traduce como que en lenguaje  ensamblador  no  existen  sentencias
anidadas y estructuras por el estilo.
    Las técnicas que se emplean  en  un  desensamblador  son  las
mismas que las empleadas en la construcción de un compilador o más
específicamente en la de un ensamblador. Se debe realizar un  aná-
lisis del código objeto para descomponerlo en sus componentes  bá-
sicos para obtener las sentencias correctas del  lenguaje  fuente,
además se da la posibilidad de crear un  fichero  de  comando  que
permita indicarle al programa información necesaria  para  que  lo
que se genere quede lo más legible posible y  lista  para  su  uso
posterior.
    La estructura interna  del  programa  se  basa  en  construir
varias tablas que contengan toda  la  información  necesaria  para
desensamblar programas objetos de 8080, 8085 y Z-80. Estas  tablas
se separan, no de acurdo al lenguaje sino, de acuerdo al  tipo  de
instrucción, esto se comprende si se  analiza  que  la  diferencia
entre el 8080 y 8085 en cuanto a código es de dos instrucciones, y
que todos los códigos que existen en 8080 existen también en  Z-80
por  lo  cual  existe  una  tabla  que  contiene  casi  todas  las
instrucciones de  8080  que  tiene  la  información,  además,  del
nemotécnico del Z-80.
    Las tablas en forma general contienen información acerca  del
tamaño en bytes de cada instrucción, si es una intrucción de salto
o llamada a subrutina, si es un retorno o salto incondicional,  si
es una instrucción múltiple, etc.
    Se necesita, de acuerdo a la estructura de estos  microproce-
sadores, una tabla que contenga información de  las  instrucciones
que sólo existen en Z-80, una para las instrucciones  en  general,
una para la información del tratamiento de  registros  de  índices
que se sale del tratamiento normal de la mayoría de  las  instruc-
ciones y una para almacenar instrucciones que por su sencillés  no
necesitan toda la información de las demás.
    Se almacena una tabla de referencias para  cada  instrucción,
que no depende de su estructura sino de  la  información  dada  al
programa para realizar  el  desensamblaje.  Esta  contiene  si  la
instrucción es código o dato, si  es  dato  de  tipo  byte,  word,
string; si se comenta o no, si lleva etiqueta o  no  y  si  se  le
definió símbolos.
    El  proceso  de  desensamblaje  se  basa  de  ir  recorriendo
recursivamente todo el fichero para formar la tabla de información
que servirá luego para  ir  desensamblando  cada  instrucción  del
código objeto generando la información necesaria  para  cada  una,
esto se hace con un sistema  de  procedimientos  y  funciones  que
permiten obtener el tamaño de cada instrucción, la información  de
sus  operandos,  la  generación  de  la  instrucción  en   fuente,
etiquetas de acuerdo a la dirección, comentarios, etc.
    El programa es usando invocando, desde el Sistema  Operativo,
un comando que tiene la siguiente sintaxis:
    DIS8 [8080|8085|Z80] <.COM> <.PAR> <.MAC> [<.SYM>]
    El fichero de parámetros  está  formado  por  líneas  con  la
siguiente estructura:
<Comando> <Parámetros del Comando>
    Los posibles comandos son :
Offset <hex>       ; Dirección de carga del programa
Entry <hex>        ; Posible punto de  entrada  del  programa  en
                      <hex>
Break <hex>        ; Interrumpir  la  exploración  automática  en
                      <hex>
Label <hex>        ; Generar una etiqueta en <hex>
String0 <hex>      ; String terminada en 00h comienza en <hex>
String$ <hex>      ; String terminada en '$' comienza en <hex>
Word <hex1> <hex2> ; Words desde <hex1> hasta <hex2>
DS <hex1> <hex2>   ; Generar DS para el rango <hex1> hasta <hex2>
C <hex1> <hex2>    ; Poner el código como comentario
Upper <hex>        ; Define el límite superior del código
    Se escribe sólo las las partes en negrita, por ejemplo:
O  0100
E  0100
S0 0179
S0 018B
S0 019A
S0 01AA
S0 01B9
E  0290
E  0355
S0 0386
S0 03C1
S0 069D
S0 06AA
W  06B0 06B3
D  7BF7 7FFF
C  6C00 7C00
Este fichero de parámetros  le  da  la  información  necesaria  al
programa para dirigir el proceso de desensamblaje.
    El fichero de símbolos es opcional y tiene el mismo  formatos
que los ficheros de símbolos generados por el LINKMT. Está formado
por líneas con el siguiente formato:
<dirección hex> <nombre de la variable de hasta 6 caracteres>
y le da información al  programa  de  en  que  direcciónes  van  a
existeir símbolos para que se pongan como etiquetas y hacer que el
programa quede en un estado lo más legible posible.
CONCLUSIONES :
    Al quedar  terminados  todos  los  sistemas  de  ayuda  a  la
programación en 8  bits,  los  técnicos  que  trabajan  con  estos
dispositivos verán ampliamente mejorado su  trabajo  de  puesta  a
punto ampliando de esta forma la explotación que se  hace  de  los
microprocesadores de 8 bits.
    Con la estructura con la que fue  diseñado  el  programa,  se
pueden obtener desensambladores  de  otros  microprocesadores  con
sólo cambiar las tablas de símbolos, esto es muy importante ya que
se conoce que se esta trabajando con otros  dispositivos  como  el
TMS-320 en trabajos de análisis de voz y tratamiento de señales de
satélites, de contar con la información necesaria el trabajo sería
ampliado en tal sentido.
BIBLIOGRAFIA :
[ 1] Disk Operating System. Technical Reference.
    Microsoft Corporation.
[ 2] Microsoft Pascal 3.31. Reference Manual.
    Microsoft Corporation.
[ 3] Microsoft Pascal 3.31. User's Guide.
    Microsoft Corporation.
[ 4] A Book on C,
    An Introduction to programming in C.
[ 5] Personal Computer XT System. Technical Reference.
    IBM Inc.
[ 6] SuperCalc 4, User's Guide.
    Computer Associates.
[ 7] SuperCalc 4, Reference Manual.
    Computer Associates.
[ 8] Ducan, Ray; Programer's Quick Reference Series, IBM ROM BIOS
    Microsoft PRESS, A Division of Microsoft Corporation
[ 9] Freedman, Alan; Glosario de Computación.
    Computer Language Company Inc.
[10] Gries, David; Construcción de compiladores.
    Edición revolucionaria.
[11] Jensen, K.; Wirth, N. Pascal User Manual and Report.
    Springer Verlag.
[12] Rohl, J. S.; Barrett, H. J. Programming via Pascal.
    Cambridge Computer Science.
[13] Knuth, Donald E. Fundamental Algorithms.
    Amsterdam, Adison-Wesley Publising, 1975.
[14] Knuth, Donald E. Sorting and Searching.
    Amsterdam, Adison-Wesley Publising, 1975.

## English

### Eight-bit program disassembler

Author: Lic. David Izada Rodríguez, Instituto de Informática, Universidad Central de Las Villas (UCLV).

### Contents

1. Summary
2. Introduction
3. Development
4. Conclusions
5. Bibliography

### Summary

This program disassembles 8-bit programs originally written for the 8080, 8085, or Z80 by searching automatically for the likely structure of the program. The user can also supply a symbol file so the source comes out more readable. That symbol file is compatible with the one produced by LINKMT, the CP/M linker. The program was written in Microsoft C and can disassemble the hidden Z80 instructions.

Two further programs, COMtoHEX and HEXtoCOM, convert to and from the `.HEX` format used by emulators.

### Introduction

Disassemblers of this kind already existed for 16-bit assemblers, but 8-bit microprocessors were still widely used in electronic devices and in industrial control. The tools for that work were not comfortable to use.

This program is one piece of a set of tools aimed at bringing up those systems. Most of the bring-up is done on 16-bit machines; only the final installation needs other tools. It was written in Microsoft C 5.10. It disassembles a program when the source listing is missing, whether the goal is to reuse the program or to see how a system works. That matters when equipment arrives with no documentation.

### Development

A disassembler turns a microprocessor's machine code into assembly source. Assembly sits close to machine language: many statements are symbolic forms of machine instructions, and they have a fixed format, which is what makes disassembly possible. Nested statements of the kind used in high-level languages are not present. No system known to the author turned machine code into a high-level language.

The techniques are those of a compiler, and more specifically of an assembler: the object code is split into the basic pieces of a source statement. A command file tells the program what it needs so the generated source is readable and ready to use.

Internally the program builds tables with everything required to disassemble 8080, 8085, and Z80 object code. The tables are split by instruction shape, not by language. The 8080 and 8085 differ by two instructions, and every 8080 opcode also exists on the Z80, so one table holds most 8080 instructions together with the Z80 mnemonic.

In general a table entry records the size in bytes, whether the instruction is a jump or a call, whether it is a return or an unconditional jump, whether it is a multi-byte prefix, and so on.

Separate tables hold the Z80-only instructions, the ordinary instructions, the index-register forms (they are not handled like the majority of instructions), and the simple instructions that do not need every field.

Another table, filled from the information given to the program rather than from the opcode shape, records for each byte whether it is code or data; whether data is a byte, a word, or a string; whether it is commented; whether it has a label; and whether a symbol was defined.

Disassembly walks the file recursively and fills that table. Later each object instruction is printed from it. Procedures report the size of an instruction, its operands, the source text, labels from addresses, and comments.

The program is started from the operating system:

    DIS8 [8080|8085|Z80] <.COM> <.PAR> <.MAC> [<.SYM>]

The parameter file is lines of `<command> <parameters>`. Only the bold initial of each command is written. The commands are Offset, Entry, Break, Label, String0, String$, Word, DS, C, and Upper. An example is given in the Spanish text and in [par.md](par.md). The parameter file is what steers the disassembly.

The symbol file is optional and has the same layout as a LINKMT symbol file: a hex address and a name of up to six characters. Those names are used as labels.

### Conclusions

Once the 8-bit programming tools are finished, the people who work with these devices should have an easier bring-up, and the 8-bit processors can be used more fully.

The program is organized so that a disassembler for another microprocessor is a new set of tables. The text mentions ongoing work with the TMS-320 on voice analysis and satellite signals, which would need the corresponding instruction information.

### Bibliography

The ChiWriter bibliography is a reading list (DOS and PC technical references, Microsoft Pascal, C, SuperCalc, ROM BIOS, compiler construction, Pascal, and Knuth). Titles are left in the Spanish section as printed. One entry names the Spanish edition of Gries used by the author (`Edición revolucionaria`).
