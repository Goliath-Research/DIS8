/*
 * Portable C11 port of DISZ80.C (Z80 only, 19-10-88) by Lic. David Izada Rodriguez
 * (Instituto de Informatica, Universidad Central de Las Villas).
 * Spanish comments from the original are kept.
 *
 *   --compat   original control-flow walk, for tests against the DOS EXE
 *   --crlf     CR+LF output, as DOS text mode wrote it
 *
 * The default walk is the corrected one. See docs/known-issues.md.
 * / cl /Ox /Zp1 disz80.c
 */
#include "dis_port.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/*	cl /Ox /Zp1 disz80.c /link /exe		*/





/*									*/
/*	Información del fichero de símbolos .SYM como lista enlazada	*/
/*									*/

typedef struct nodo {
	word		dir;
	char		sym[7];
	struct nodo	*link;
} *LINK;

/*									*/
/*	Información del fichero de símbolos .SYM como arreglo ordenado	*/
/*									*/

typedef struct {
	word	dir;
	char	sym[7];
} SYM;

/*									*/
/*	Información necesaria por cada byte del programa analizado	*/
/*									*/

typedef struct {
	unsigned Code : 1;	/*	0 - Dato		*/
				/*	1 - Código		*/
	unsigned Data : 3;	/*	000 - Byte		*/
				/*	001 - Word		*/
				/*	010 - String 0		*/
				/*	011 - String $		*/
				/*	100 - DS		*/
				/*	101 - Comentado		*/
	unsigned Label : 1;	/*	0 - Sin etiqueta	*/
				/*      1 - Con etiqueta        */
	unsigned Break : 1;	/*      0 - Continuar auto...   */
				/*      1 - Parar automático    */
	unsigned Symbol : 1;	/*	0 - No tiene símbolo	*/
				/*	1 - Hay símbolo		*/
} type;

/*								*/
/*	Información necesaria por cada instrucción Z-80		*/
/*								*/

typedef struct {
	unsigned Size : 2;	/*	Tamaño en bytes		*/
	unsigned RST : 1;	/*	Instrucción RST		*/
	unsigned JPa : 1;	/*	JP absoluto o CALL	*/
	unsigned JPr : 1;	/*	JP relativo		*/
	unsigned RET : 1;	/*	RET o JP incondicional	*/
	unsigned Mult : 1;	/*	Instrucción múltiple	*/
	unsigned HL : 1;	/*	Referencia HL		*/
} info;

typedef struct {
	info	att;
	char	*s;	/*      w   - word              */
				/*	b   - byte		*/
				/*      e   - label             */
				/*	r   - relative		*/
				/*	i   - index IX,IY o HL	*/
				/*	h   - index XH,YH o H	*/
				/*	l   - index XL,YL o L	*/
} info0;

info0 Table[0x100] = {
	{{1,0,0,0,0,0,0},"NOP"},
	{{3,0,0,0,0,0,0},"LD\tBC,w"},
	{{1,0,0,0,0,0,0},"LD\t(BC),A"},
	{{1,0,0,0,0,0,0},"INC\tBC"},
	{{1,0,0,0,0,0,0},"INC\tB"},
	{{1,0,0,0,0,0,0},"DEC\tB"},
	{{2,0,0,0,0,0,0},"LD\tB,b"},
	{{1,0,0,0,0,0,0},"RLCA"},
	{{1,0,0,0,0,0,0},"EX\tAF,AF'"},
	{{1,0,0,0,0,0,1},"ADD\ti,BC"},
	{{1,0,0,0,0,0,0},"LD\tA,(BC)"},
	{{1,0,0,0,0,0,0},"DEC\tBC"},
	{{1,0,0,0,0,0,0},"INC\tC"},
	{{1,0,0,0,0,0,0},"DEC\tC"},
	{{2,0,0,0,0,0,0},"LD\tC,b"},
	{{1,0,0,0,0,0,0},"RRCA"},
	{{2,0,0,1,0,0,0},"DJNZ\tr"},
	{{3,0,0,0,0,0,0},"LD\tDE,w"},
	{{1,0,0,0,0,0,0},"LD\t(DE),A"},
	{{1,0,0,0,0,0,0},"INC\tDE"},
	{{1,0,0,0,0,0,0},"INC\tD"},
	{{1,0,0,0,0,0,0},"DEC\tD"},
	{{2,0,0,0,0,0,0},"LD\tD,b"},
	{{1,0,0,0,0,0,0},"RLA"},
	{{2,0,0,1,1,0,0},"JR\tr"},
	{{1,0,0,0,0,0,1},"ADD\ti,DE"},
	{{1,0,0,0,0,0,0},"LD\tA,(DE)"},
	{{1,0,0,0,0,0,0},"DEC\tDE"},
	{{1,0,0,0,0,0,0},"INC\tE"},
	{{1,0,0,0,0,0,0},"DEC\tE"},
	{{2,0,0,0,0,0,0},"LD\tE,b"},
	{{1,0,0,0,0,0,0},"RRA"},
	{{2,0,0,1,0,0,0},"JR\tNZ,r"},
	{{3,0,0,0,0,0,1},"LD\ti,w"},
	{{3,0,0,0,0,0,1},"LD\t(e),i"},
	{{1,0,0,0,0,0,1},"INC\ti"},
	{{1,0,0,0,0,0,1},"INC\th"},
	{{1,0,0,0,0,0,1},"DEC\th"},
	{{2,0,0,0,0,0,1},"LD\th,b"},
	{{1,0,0,0,0,0,0},"DAA"},
	{{2,0,0,1,0,0,0},"JR\tZ,r"},
	{{1,0,0,0,0,0,1},"ADD\ti,i"},
	{{3,0,0,0,0,0,1},"LD\ti,(e)"},
	{{1,0,0,0,0,0,1},"DEC\ti"},
	{{1,0,0,0,0,0,1},"INC\tl"},
	{{1,0,0,0,0,0,1},"DEC\tl"},
	{{2,0,0,0,0,0,1},"LD\tl,b"},
	{{1,0,0,0,0,0,0},"CPL"},
	{{2,0,0,1,0,0,0},"JR\tNC,r"},
	{{3,0,0,0,0,0,0},"LD\tSP,w"},
	{{3,0,0,0,0,0,0},"LD\t(e),A"},
	{{1,0,0,0,0,0,0},"INC\tSP"},
	{{1,0,0,0,0,0,1},"INC\t(i)"},
	{{1,0,0,0,0,0,1},"DEC\t(i)"},
	{{2,0,0,0,0,0,1},"LD\t(i),b"},
	{{1,0,0,0,0,0,0},"SCF"},
	{{2,0,0,1,0,0,0},"JR\tC,r"},
	{{1,0,0,0,0,0,1},"ADD\ti,SP"},
	{{3,0,0,0,0,0,0},"LD\tA,(e)"},
	{{1,0,0,0,0,0,0},"DEC\tSP"},
	{{1,0,0,0,0,0,0},"INC\tA"},
	{{1,0,0,0,0,0,0},"DEC\tA"},
	{{2,0,0,0,0,0,0},"LD\tA,b"},
	{{1,0,0,0,0,0,0},"CCF"},
	{{1,0,0,0,0,0,0},"LD\tB,B"},
	{{1,0,0,0,0,0,0},"LD\tB,C"},
	{{1,0,0,0,0,0,0},"LD\tB,D"},
	{{1,0,0,0,0,0,0},"LD\tB,E"},
	{{1,0,0,0,0,0,1},"LD\tB,h"},
	{{1,0,0,0,0,0,1},"LD\tB,l"},
	{{1,0,0,0,0,0,1},"LD\tB,(i)"},
	{{1,0,0,0,0,0,0},"LD\tB,A"},
	{{1,0,0,0,0,0,0},"LD\tC,B"},
	{{1,0,0,0,0,0,0},"LD\tC,C"},
	{{1,0,0,0,0,0,0},"LD\tC,D"},
	{{1,0,0,0,0,0,0},"LD\tC,E"},
	{{1,0,0,0,0,0,1},"LD\tC,h"},
	{{1,0,0,0,0,0,1},"LD\tC,l"},
	{{1,0,0,0,0,0,1},"LD\tC,(i)"},
	{{1,0,0,0,0,0,0},"LD\tC,A"},
	{{1,0,0,0,0,0,0},"LD\tD,B"},
	{{1,0,0,0,0,0,0},"LD\tD,C"},
	{{1,0,0,0,0,0,0},"LD\tD,D"},
	{{1,0,0,0,0,0,0},"LD\tD,E"},
	{{1,0,0,0,0,0,1},"LD\tD,h"},
	{{1,0,0,0,0,0,1},"LD\tD,l"},
	{{1,0,0,0,0,0,1},"LD\tD,(i)"},
	{{1,0,0,0,0,0,0},"LD\tD,A"},
	{{1,0,0,0,0,0,0},"LD\tE,B"},
	{{1,0,0,0,0,0,0},"LD\tE,C"},
	{{1,0,0,0,0,0,0},"LD\tE,D"},
	{{1,0,0,0,0,0,0},"LD\tE,E"},
	{{1,0,0,0,0,0,1},"LD\tE,h"},
	{{1,0,0,0,0,0,1},"LD\tE,l"},
	{{1,0,0,0,0,0,1},"LD\tE,(i)"},
	{{1,0,0,0,0,0,0},"LD\tE,A"},
	{{1,0,0,0,0,0,1},"LD\th,B"},
	{{1,0,0,0,0,0,1},"LD\th,C"},
	{{1,0,0,0,0,0,1},"LD\th,D"},
	{{1,0,0,0,0,0,1},"LD\th,E"},
	{{1,0,0,0,0,0,0},"LD\th,h"},
	{{1,0,0,0,0,0,1},"LD\th,l"},
	{{1,0,0,0,0,0,1},"LD\th,(i)"},
	{{1,0,0,0,0,0,1},"LD\th,A"},
	{{1,0,0,0,0,0,1},"LD\tl,B"},
	{{1,0,0,0,0,0,1},"LD\tl,C"},
	{{1,0,0,0,0,0,1},"LD\tl,D"},
	{{1,0,0,0,0,0,1},"LD\tl,E"},
	{{1,0,0,0,0,0,1},"LD\tl,h"},
	{{1,0,0,0,0,0,1},"LD\tl,l"},
	{{1,0,0,0,0,0,1},"LD\tL,(i)"},
	{{1,0,0,0,0,0,1},"LD\tl,A"},
	{{1,0,0,0,0,0,1},"LD\t(i),B"},
	{{1,0,0,0,0,0,1},"LD\t(i),C"},
	{{1,0,0,0,0,0,1},"LD\t(i),D"},
	{{1,0,0,0,0,0,1},"LD\t(i),E"},
	{{1,0,0,0,0,0,1},"LD\t(i),H"},
	{{1,0,0,0,0,0,1},"LD\t(i),L"},
	{{1,0,0,0,0,0,0},"HALT"},
	{{1,0,0,0,0,0,1},"LD\t(i),A"},
	{{1,0,0,0,0,0,0},"LD\tA,B"},
	{{1,0,0,0,0,0,0},"LD\tA,C"},
	{{1,0,0,0,0,0,0},"LD\tA,D"},
	{{1,0,0,0,0,0,0},"LD\tA,E"},
	{{1,0,0,0,0,0,1},"LD\tA,h"},
	{{1,0,0,0,0,0,1},"LD\tA,l"},
	{{1,0,0,0,0,0,1},"LD\tA,(i)"},
	{{1,0,0,0,0,0,0},"LD\tA,A"},
	{{1,0,0,0,0,0,0},"ADD\tA,B"},
	{{1,0,0,0,0,0,0},"ADD\tA,C"},
	{{1,0,0,0,0,0,0},"ADD\tA,D"},
	{{1,0,0,0,0,0,0},"ADD\tA,E"},
	{{1,0,0,0,0,0,1},"ADD\tA,h"},
	{{1,0,0,0,0,0,1},"ADD\tA,l"},
	{{1,0,0,0,0,0,1},"ADD\tA,(i)"},
	{{1,0,0,0,0,0,0},"ADD\tA,A"},
	{{1,0,0,0,0,0,0},"ADC\tA,B"},
	{{1,0,0,0,0,0,0},"ADC\tA,C"},
	{{1,0,0,0,0,0,0},"ADC\tA,D"},
	{{1,0,0,0,0,0,0},"ADC\tA,E"},
	{{1,0,0,0,0,0,1},"ADC\tA,h"},
	{{1,0,0,0,0,0,1},"ADC\tA,l"},
	{{1,0,0,0,0,0,1},"ADC\tA,(i)"},
	{{1,0,0,0,0,0,0},"ADC\tA,A"},
	{{1,0,0,0,0,0,0},"SUB\tB"},
	{{1,0,0,0,0,0,0},"SUB\tC"},
	{{1,0,0,0,0,0,0},"SUB\tD"},
	{{1,0,0,0,0,0,0},"SUB\tE"},
	{{1,0,0,0,0,0,1},"SUB\th"},
	{{1,0,0,0,0,0,1},"SUB\tl"},
	{{1,0,0,0,0,0,1},"SUB\t(i)"},
	{{1,0,0,0,0,0,0},"SUB\tA"},
	{{1,0,0,0,0,0,0},"SBC\tA,B"},
	{{1,0,0,0,0,0,0},"SBC\tA,C"},
	{{1,0,0,0,0,0,0},"SBC\tA,D"},
	{{1,0,0,0,0,0,0},"SBC\tA,E"},
	{{1,0,0,0,0,0,1},"SBC\tA,h"},
	{{1,0,0,0,0,0,1},"SBC\tA,l"},
	{{1,0,0,0,0,0,1},"SBC\tA,(i)"},
	{{1,0,0,0,0,0,0},"SBC\tA,A"},
	{{1,0,0,0,0,0,0},"AND\tB"},
	{{1,0,0,0,0,0,0},"AND\tC"},
	{{1,0,0,0,0,0,0},"AND\tD"},
	{{1,0,0,0,0,0,0},"AND\tE"},
	{{1,0,0,0,0,0,1},"AND\th"},
	{{1,0,0,0,0,0,1},"AND\tl"},
	{{1,0,0,0,0,0,1},"AND\t(i)"},
	{{1,0,0,0,0,0,0},"AND\tA"},
	{{1,0,0,0,0,0,0},"XOR\tB"},
	{{1,0,0,0,0,0,0},"XOR\tC"},
	{{1,0,0,0,0,0,0},"XOR\tD"},
	{{1,0,0,0,0,0,0},"XOR\tE"},
	{{1,0,0,0,0,0,1},"XOR\th"},
	{{1,0,0,0,0,0,1},"XOR\tl"},
	{{1,0,0,0,0,0,1},"XOR\t(i)"},
	{{1,0,0,0,0,0,0},"XOR\tA"},
	{{1,0,0,0,0,0,0},"OR\tB"},
	{{1,0,0,0,0,0,0},"OR\tC"},
	{{1,0,0,0,0,0,0},"OR\tD"},
	{{1,0,0,0,0,0,0},"OR\tE"},
	{{1,0,0,0,0,0,1},"OR\th"},
	{{1,0,0,0,0,0,1},"OR\tl"},
	{{1,0,0,0,0,0,1},"OR\t(i)"},
	{{1,0,0,0,0,0,0},"OR\tA"},
	{{1,0,0,0,0,0,0},"CP\tB"},
	{{1,0,0,0,0,0,0},"CP\tC"},
	{{1,0,0,0,0,0,0},"CP\tD"},
	{{1,0,0,0,0,0,0},"CP\tE"},
	{{1,0,0,0,0,0,1},"CP\th"},
	{{1,0,0,0,0,0,1},"CP\tl"},
	{{1,0,0,0,0,0,1},"CP\t(i)"},
	{{1,0,0,0,0,0,0},"CP\tA"},
	{{1,0,0,0,0,0,0},"RET\tNZ"},
	{{1,0,0,0,0,0,0},"POP\tBC"},
	{{3,0,1,0,0,0,0},"JP\tNZ,e"},
	{{3,0,1,0,1,0,0},"JP\te"},
	{{3,0,1,0,0,0,0},"CALL\tNZ,e"},
	{{1,0,0,0,0,0,0},"PUSH\tBC"},
	{{2,0,0,0,0,0,0},"ADD\tA,b"},
	{{1,1,1,0,0,0,0},"RST\t0"},
	{{1,0,0,0,0,0,0},"RET\tZ"},
	{{1,0,0,0,1,0,0},"RET"},
	{{3,0,1,0,0,0,0},"JP\tZ,e"},
	{{0,0,0,0,0,1,0},""},			/*	CB	*/
	{{3,0,1,0,0,0,0},"CALL\tZ,e"},
	{{3,0,1,0,0,0,0},"CALL\te"},
	{{2,0,0,0,0,0,0},"ADC\tA,b"},
	{{1,1,1,0,0,0,0},"RST\t8"},
	{{1,0,0,0,0,0,0},"RET\tNC"},
	{{1,0,0,0,0,0,0},"POP\tDE"},
	{{3,0,1,0,0,0,0},"JP\tNC,e"},
	{{2,0,0,0,0,0,0},"OUT\t(b),A"},
	{{3,0,1,0,0,0,0},"CALL\tNC,e"},
	{{1,0,0,0,0,0,0},"PUSH\tDE"},
	{{2,0,0,0,0,0,0},"SUB\tb"},
	{{1,1,1,0,0,0,0},"RST\t10H"},
	{{1,0,0,0,0,0,0},"RET\tC"},
	{{1,0,0,0,0,0,0},"EXX"},
	{{3,0,1,0,0,0,0},"JP\tC,e"},
	{{2,0,0,0,0,0,0},"IN\tA,(b)"},
	{{3,0,1,0,0,0,0},"CALL\tC,e"},
	{{0,0,0,0,0,1,0},""},			/*	DD - index IX	*/
	{{2,0,0,0,0,0,0},"SBC\tA,b"},
	{{1,1,1,0,0,0,0},"RST\t18H"},
	{{1,0,0,0,0,0,0},"RET\tPO"},
	{{1,0,0,0,0,0,1},"POP\ti"},
	{{3,0,1,0,0,0,0},"JP\tPO,e"},
	{{1,0,0,0,0,0,1},"EX\t(SP),i"},
	{{3,0,1,0,0,0,0},"CALL\tPO,e"},
	{{1,0,0,0,0,0,1},"PUSH\ti"},
	{{2,0,0,0,0,0,0},"AND\tb"},
	{{1,1,1,0,0,0,0},"RST\t20H"},
	{{1,0,0,0,0,0,0},"RET\tPE"},
	{{1,0,0,0,1,0,1},"JP\t(i)"},
	{{3,0,1,0,0,0,0},"JP\tPE,e"},
	{{1,0,0,0,0,0,1},"EX\tDE,i"},
	{{3,0,1,0,0,0,0},"CALL\tPE,e"},
	{{0,0,0,0,0,1,0},""},			/*	DD	*/
	{{2,0,0,0,0,0,0},"XOR\tb"},
	{{1,1,1,0,0,0,0},"RST\t28H"},
	{{1,0,0,0,0,0,0},"RET\tP"},
	{{1,0,0,0,0,0,0},"POP\tAF"},
	{{3,0,1,0,0,0,0},"JP\tP,e"},
	{{1,0,0,0,0,0,0},"DI"},
	{{3,0,1,0,0,0,0},"CALL\tP,e"},
	{{1,0,0,0,0,0,0},"PUSH\tAF"},
	{{2,0,0,0,0,0,0},"OR\tb"},
	{{1,1,1,0,0,0,0},"RST\t30H"},
	{{1,0,0,0,0,0,0},"RET\tM"},
	{{1,0,0,0,0,0,1},"LD\tSP,i"},
	{{3,0,1,0,0,0,0},"JP\tM,e"},
	{{1,0,0,0,0,0,0},"EI"},
	{{3,0,1,0,0,0,0},"CALL\tM,e"},
	{{0,0,0,0,0,1,0},""},			/*	FD - index IY	*/
	{{2,0,0,0,0,0,0},"CP\tb"},
	{{1,1,1,0,0,0,0},"RST\t38H"}};

info0	TableED[0x7C] = {		/*	Comienza en 0x40	*/
	{{1,0,0,0,0,0,0},"IN\tB,(C)"},
	{{1,0,0,0,0,0,0},"OUT\t(C),B"},
	{{1,0,0,0,0,0,0},"SBC\tHL,BC"},
	{{3,0,0,0,0,0,0},"LD\t(e),BC"},
	{{1,0,0,0,0,0,0},"NEG"},
	{{1,0,0,0,1,0,0},"RETN"},
	{{1,0,0,0,0,0,0},"IM\t0"},
	{{1,0,0,0,0,0,0},"LD\tI,A"},
	{{1,0,0,0,0,0,0},"INC\tC,(C)"},
	{{1,0,0,0,0,0,0},"OUT\t(C),C"},
	{{1,0,0,0,0,0,0},"ADC\tHL,BC"},
	{{3,0,0,0,0,0,0},"LD\tBC,(e)"},
	{{0,0,0,0,0,0,0},""},
	{{1,0,0,0,1,0,0},"RETI"},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{1,0,0,0,0,0,0},"IN\tD,(C)"},
	{{1,0,0,0,0,0,0},"OUT\t(C),D"},
	{{1,0,0,0,0,0,0},"SBC\tHL,DE"},
	{{3,0,0,0,0,0,0},"LD\t(e),DE"},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{1,0,0,0,0,0,0},"IM\t1"},
	{{1,0,0,0,0,0,0},"LD\tA,I"},
	{{1,0,0,0,0,0,0},"IN\tE,(C)"},
	{{1,0,0,0,0,0,0},"OUT\t(C),E"},
	{{1,0,0,0,0,0,0},"ADC\tHL,DE"},
	{{3,0,0,0,0,0,0},"LD\tDE,(e)"},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{1,0,0,0,0,0,0},"IM\t2"},
	{{0,0,0,0,0,0,0},""},
	{{1,0,0,0,0,0,0},"IN\tH,(C)"},
	{{1,0,0,0,0,0,0},"OUT\t(C),H"},
	{{1,0,0,0,0,0,0},"SBC\tHL,HL"},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{1,0,0,0,0,0,0},"RRD"},
	{{1,0,0,0,0,0,0},"IN\tL,(C)"},
	{{1,0,0,0,0,0,0},"OUT\t(C),L"},
	{{1,0,0,0,0,0,0},"ADC\tHL,HL"},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{1,0,0,0,0,0,0},"RLD"},
	{{1,0,0,0,0,0,0},"IN\tF,(C)"},
	{{0,0,0,0,0,0,0},""},
	{{1,0,0,0,0,0,0},"SBC\tHL,SP"},
	{{3,0,0,0,0,0,0},"LD\t(e),SP"},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{1,0,0,0,0,0,0},"IN\tA,(C)"},
	{{1,0,0,0,0,0,0},"OUT\t(C),A"},
	{{1,0,0,0,0,0,0},"ADC\tHL,SP"},
	{{3,0,0,0,0,0,0},"LD\tSP,(e)"},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{1,0,0,0,0,0,0},"LDI"},
	{{1,0,0,0,0,0,0},"CPI"},
	{{1,0,0,0,0,0,0},"INI"},
	{{1,0,0,0,0,0,0},"OUTI"},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{1,0,0,0,0,0,0},"LDD"},
	{{1,0,0,0,0,0,0},"CPD"},
	{{1,0,0,0,0,0,0},"IND"},
	{{1,0,0,0,0,0,0},"OUTD"},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{1,0,0,0,0,0,0},"LDIR"},
	{{1,0,0,0,0,0,0},"CPIR"},
	{{1,0,0,0,0,0,0},"INIR"},
	{{1,0,0,0,0,0,0},"OTIR"},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{0,0,0,0,0,0,0},""},
	{{1,0,0,0,0,0,0},"LDDR"},
	{{1,0,0,0,0,0,0},"CPDR"},
	{{1,0,0,0,0,0,0},"INDR"},
	{{1,0,0,0,0,0,0},"OTDR"}};

char *Reg[10] = {"B","C","D","E","H","L","(HL)","A","IX","IY"};
char *Cod[11] = {"RLC","RRC","RL","RR","SLA","SRA","SLL","SRL","BIT","RES","SET"};

byte    PROG[IMGX];
word    PC,LIMIT,OFFSET = 0x100,symbols = 0;
type	TYPE[IMGX];
LINK	ptr = 0,p,q;
SYM	*Sym = 0;
FILE    *m;

word LENED(word PC)
{
	byte	cod;

	cod = PROG[PC];
	/* cod > 0xBB se salía de TableED[0x7C]. Esa lectura no se reproduce. */
	if ((cod < 0x40) || (cod > 0xBB) || (TableED[cod - 0x40].s[0] == '\0'))
	  return(1);
	else
	  return(TableED[cod - 0x40].att.Size+1);
}

word LENIxCB(word PC)
{
	byte op;

	op = PROG[PC + 1];
	/* Compat: the source says (op >> 3 + 2) % 8, and '+' binds tighter
	   than '>>', so this is (op >> 5) % 8. The printer instead accepts
	   the IX/IY CB form when (op + 2) % 8 == 0, i.e. (op & 7) == 6. */
	if (dis_compat) {
	  if ((op >> (3 + 2)) % 8)
	    return 1;
	  return 4;
	}
	if ((op + 2) % 8)
	  return 1;
	return 4;
}


word LENIx(word PC)
{
	byte	cod;

	if (Table[cod = PROG[PC]].att.HL)
	  return((word)(Table[cod].att.Size + 1 + ((!dis_compat && strstr(Table[cod].s, "(i") != NULL) ? 1 : 0)));
	else
	  if (cod == 0xCB)
	    return(LENIxCB(PC+1));
	  else
	    return(1);
}

word LEN(word PC)
{
	byte	cod;

	if (Table[cod = PROG[PC]].att.Mult)
	  switch (cod) {
	    case 0xCB: return(2);
	    case 0xED: return(LENED(PC+1));
	    case 0xDD:
	    case 0xFD: return(LENIx(PC+1));
	    default: return(1);
	  }
	else
	  return(Table[cod].att.Size);
}

word PrintHex(word w, int label, int count)
{
	int     i,j;
	word    k = 0;
	char    s[8];

	for (i = count; i; ) {
	  j = w % 16;
	  w >>= 4;
	  s[--i] = (j < 10) ? ('0' + j) : ('A' + j - 10);
	}
	if (label)
	  k++,dis_putc('L',m);
	else {
	  if (s[0] > '9')
	    k++,dis_putc('0',m);
	}
	for (i = 0; i < count; i++)
	  k++,dis_putc(s[i],m);
	if (!label)
	  k++,dis_putc('H',m);
	return(k);
}

int compare(const void *a, const void *b)
{
	word da, db;
	memcpy(&da, a, sizeof da);
	memcpy(&db, b, sizeof db);
	if (da < db)
	  return -1;
	if (da > db)
	  return 1;
	return 0;
}

void PrintDIS(word *IP)
{
	dis_putc('+',m);
	PrintHex(PROG[++*IP++],FALSE,2);
}

void Print(char *s, word IP)
{
	int	Par = FALSE, Byt = FALSE;
	byte	b;
	word	label;
	SYM	*p;

	while (*s) {
	  switch (*s) {
	    case 'w': PrintHex(rd16(&PROG[++IP]),FALSE,4);
		      IP += 2;
		      break;
	    case 'e': label = rd16(&PROG[++IP]);
	    	      if ((Sym == 0) ||
		          ((p = bsearch(&label,Sym,symbols,sizeof(SYM),compare)) == NULL))
			PrintHex(label,TRUE,4);
		      else
		        dis_fputs(p->sym,m);
		      IP += 2;
		      break;
	    case 'b': PrintHex(b = (byte) PROG[++IP],FALSE,2);
		      Byt = TRUE;
		      IP += 1;
		      break;
	    case 'r': PrintHex(OFFSET + PC + 2 + rds8(PROG[++IP]),TRUE,4);
		      break;
	    case 'i': switch (PROG[PC]) {
			case 0xDD: dis_fprintf(m,"IX");
				   if (Par)
				     PrintDIS(&IP);
				   break;
			case 0xFD: dis_fprintf(m,"IY");
				   if (Par)
				     PrintDIS(&IP);
				   break;
			default  : dis_fprintf(m,"HL");
		      };
		      break;
	    case 'h': switch (PROG[PC]) {
			case 0xDD: dis_fprintf(m,"XH");
				   break;
			case 0xFD: dis_fprintf(m,"YH");
				   break;
			default  : dis_fprintf(m,"H");
		      };
		      break;
	    case 'l': switch (PROG[PC]) {
			case 0xDD: dis_fprintf(m,"XL");
				   break;
			case 0xFD: dis_fprintf(m,"YL");
				   break;
			default  : dis_fprintf(m,"L");
		      };
		      break;
	    default:
		      if (!Par)
		        Par = (*s == '(');
		      if (*s == ')')
		        Par = FALSE;
		      dis_putc(*s,m);
	  }
	  s++;
	}
	if (Byt)
	  if ((b > 31) && (b < 127))
	    dis_fprintf(m,"\t\t\t; '%c'",b);
	dis_putc('\n',m);
}

void Error()
{
	dis_fprintf(m,"??\t\t;");
	PrintHex(PROG[PC],FALSE,2);
	dis_putc('\n',m);
}

void Proc1CB(int n)
{
	dis_fprintf(m,"%s\t%s\n",Cod[n],Reg[PROG[PC] % 8]);
}

void Proc2CB(int n)
{
	dis_fprintf(m,"%s\t%c,%s\n",Cod[n],'0' + ((PROG[PC] >> 3) & 0x07),Reg[PROG[PC] % 8]);
}

void DISASMCB()
{
	byte	i,cod;

	i = (cod = PROG[++PC]) >> 3;
	if (i < 8)
	  Proc1CB(i);
	else
	  Proc2CB((cod >> 6) + 7);
	PC++;
}

void Proc1IxCB(int n)
{
	dis_fprintf(m,"%s\t(%s+",Cod[n],Reg[8+((PROG[PC-1] & 0x20) >> 5)]);
	PrintHex(PROG[PC+1],FALSE,2);
	dis_fprintf(m,")\n");
}

void Proc2IxCB(int n)
{
	dis_fprintf(m,"%s\t%c,(%s+",Cod[n],'0' + ((PROG[PC+2] >> 3) & 0x07),Reg[8+((PROG[PC-1] & 0x20) >> 5)]);
	PrintHex(PROG[PC+1],FALSE,2);
	dis_fprintf(m,")\n");
}

void DISASMIxCB()
{
	byte	i,cod;

	i = (cod = PROG[++PC + 2]) >> 3;
	if ((cod + 2) % 8)
	  Error();
	else {
	  if (i < 8)
	    Proc1IxCB(i);
	  else
	    Proc2IxCB((cod >> 6) + 7);
	  PC += 3;
	}
}

void DISASMIx()
{
	byte	cod;

	if (Table[cod = PROG[PC+1]].att.HL) {
	  Print(Table[cod].s,PC+1);
	  PC += Table[cod].att.Size + 1;
	  if (!dis_compat && strstr(Table[cod].s, "(i") != NULL)
	    PC++;	/* byte de desplazamiento IX/IY, ausente en 1988 */
	}
	else
	  if (PROG[PC+1] == 0xCB)
	    DISASMIxCB();
	  else {
	    Error();
	    PC++;
	  }
}

void DISASMED()
{
	byte	cod;

	cod = PROG[PC+1];
	if ((cod < 0x40) || (cod > 0xBB) || (TableED[cod - 0x40].s[0] == '\0')) {
	  Error();
	  PC++;
	}
	else {
	  cod -= 0x40;
	  Print(TableED[cod].s,PC+1);
	  PC += TableED[cod].att.Size + 1;
	}
}

void DISASMCODE(word PC1, word PC2)
{
	byte    cod;
	word	label;
	SYM	*p;

	PC = PC1;
	while (PC <= PC2) {
	  if (TYPE[PC].Label) {
	    label = PC + OFFSET;
	    if (TYPE[PC].Symbol) {
	      p = bsearch(&label,Sym,symbols,sizeof(SYM),compare);
	      dis_fputs(p->sym,m);
	    }
	    else
	      PrintHex(label,TRUE,4);
	    dis_putc(':',m);
	  }
	  dis_putc('\t',m);
	  if (Table[cod = PROG[PC]].att.Mult)
	    switch (cod) {
	      case 0xCB: DISASMCB(); break;
	      case 0xED: DISASMED(); break;
	      case 0xDD:
	      case 0xFD: DISASMIx();
	    }
	  else {
	    Print(Table[cod].s,PC);
	    PC += Table[cod].att.Size;
	  }
	}
}

void PrintString(byte c)
{
	int	ASCII = FALSE;
	byte	b;

	dis_fprintf(m,"\tDB\t");
	do {
	  b = PROG[PC];
	  if ((b > 31) && (b < 127)) {
	    if (!ASCII) {
	      dis_putc('\'',m);
	      ASCII = TRUE;
	    }
	    dis_putc(b,m);
	    if (b == '\'')
	      dis_putc('\'',m);
	  }
	  else {
	    if (ASCII) {
	      dis_fprintf(m,"\',");
	      ASCII = FALSE;
	    }
	    PrintHex(b,FALSE,2);
	    if (b != c)
	      dis_putc(',',m);
	  }
	}
	while (PROG[PC++] != c);
	if (ASCII)
	  dis_putc('\'',m);
	dis_putc('\n',m);
}

word PrintDB(word IP,word len)
{
	word	i,c = 0;

	dis_fprintf(m,"\tDB\t");
	for (i = 1; i <= len; i++) {
	  c += PrintHex(PROG[IP++],FALSE,2);
	  if (i != len)
	    c++,dis_putc(',',m);
	}
	return(c);
}

void DISASMComment()
{
	word    i,n,len,IP,count = 0;

	IP = PC;
	do {
	  count++;
	  IP++;
	}
	while ((TYPE[IP].Data == 5) && (!TYPE[IP].Label));
	while ((len = LEN(PC)) <= count) {
	  n = PrintDB(PC,len);
	  for (i = 0; i <= ((23 - n) >> 3); i++)
	    dis_putc('\t',m);
	  dis_putc(';',m);
	  PrintHex(PC + OFFSET,FALSE,4);
	  dis_putc('\t',m);
	  DISASMCODE(PC,PC+len-1);
	  count -= len;
	}
	if (count) {
	  PrintDB(PC,count);
	  dis_putc('\n',m);
	  PC += count;
	}
}

void DISASMDATA(word PC1,word LIM)
{
	byte	cod;
	word	count,label;

	PC = PC1;
	while (PC <= LIM) {
	  if (TYPE[PC].Label) {
	    label = PC + OFFSET;
	    if (TYPE[PC].Symbol) {
	      p = bsearch(&label,Sym,symbols,sizeof(SYM),compare);
	      dis_fputs(p->sym,m);
	    }
	    else
	      PrintHex(label,TRUE,4);
	    dis_putc(':',m);
	  }
	  switch (TYPE[PC].Data) {
	    case 0: dis_fprintf(m,"\tDB\t");
		    PrintHex(cod = PROG[PC],FALSE,2);
		    if (!TYPE[PC].Label) {
		      dis_fprintf(m,"\t\t\t;");
		      PrintHex(PC + OFFSET,FALSE,4);
		      if ((cod > 31) && (cod < 127))
			dis_fprintf(m," '%c'",cod);
		    }
		    dis_putc('\n',m);
		    PC++;
		    break;
	    case 1: dis_fprintf(m,"\tDW\t");
		    PrintHex(rd16(&PROG[PC]),FALSE,4);
		    if (!TYPE[PC].Label) {
		      dis_fprintf(m,"\t\t\t;");
		      PrintHex(PC + OFFSET,FALSE,4);
		    }
		    PC += 2;
		    dis_putc('\n',m);
		    break;
	    case 2: PrintString(0);
		    break;
	    case 3: PrintString('$');
		    break;
	    case 4: dis_fprintf(m,"\tDS\t");
		    for (count = 1, PC++; ((TYPE[PC].Data == 4) && (!TYPE[PC].Label)); count++, PC++) ;
		    PrintHex(count,FALSE,4);
		    dis_putc('\n',m);
		    break;
	    case 5: DISASMComment();
	  }
	}
}

int JMPED(word PC)
{
	byte	cod;

	cod = PROG[PC];
	if ((cod < 0x40) || (cod > 0xBB) || (TableED[cod - 0x40].s[0] == '\0'))
	  return(FALSE);
	else {
	  cod -= 0x40;
	  return(TableED[cod].att.JPa || TableED[cod].att.JPr || TableED[cod].att.RST);
	}
}

int JMPIx(word PC)
{
	byte	cod;

	if (Table[cod = PROG[PC]].att.HL)
	  return(Table[cod].att.JPa || Table[cod].att.JPr || Table[cod].att.RST);
	else
	  return(FALSE);
}

int JMP(word PC)
{
	byte	cod;

	if (Table[cod = PROG[PC]].att.Mult)
	  switch (cod) {
	    case 0xCB: return(FALSE);
	    case 0xED: return(JMPED(PC+1));
	    case 0xDD:
	    case 0xFD: return(JMPIx(PC+1));
	    default: return(FALSE);
	  }
	else
	  return(Table[cod].att.JPa || Table[cod].att.JPr || Table[cod].att.RST);
}

int RETED(word PC)
{
	byte	cod;

	cod = PROG[PC];
	if ((cod < 0x40) || (cod > 0xBB) || (TableED[cod - 0x40].s[0] == '\0'))
	  return(FALSE);
	else
	  return(TableED[cod - 0x40].att.RET);
}

int RETIx(word PC)
{
	byte	cod;

	if (Table[cod= PROG[PC]].att.HL)
	  return(Table[cod].att.RET);
	else
	  return(FALSE);
}

int RET(word PC)
{
	byte	cod;

	if (Table[cod = PROG[PC]].att.Mult)
	  switch (cod) {
	    case 0xCB: return(FALSE);
	    case 0xED: return(RETED(PC+1));
	    case 0xDD:
	    case 0xFD: return(RETIx(PC+1));
	    default: return(FALSE);
	  }
	else
	  return(Table[cod].att.RET);
}

int DAT(word PC)
{
	byte	cod;

	cod = PROG[PC];
	if (JMP(PC) || RET(PC))
	  return(FALSE);
	else
	  if (Table[cod].att.Mult)
	    switch (cod) {
	      case 0xCB: return(FALSE);
	      case 0xED: {
		/* TableED empieza en el opcode 0x40. El fuente de 1988
		   indexaba con el segundo byte crudo, así que LD (nn),rr
		   no veía la 'e' y la dirección no recibía etiqueta.
		   Un índice >= 0x7C se salía del arreglo; esa lectura
		   no se reproduce. */
		byte ed = PROG[PC + 1];
		if (ed < 0x40 || ed > 0xBB)
		  return(FALSE);
		if (dis_compat)
		  return((ed < 0x7C) && (strchr(TableED[ed].s, 'e') != NULL));
		return(strchr(TableED[ed - 0x40].s, 'e') != NULL);
	      }
	      case 0xDD:
	      case 0xFD: return(strchr(Table[PROG[PC+1]].s,'e') != NULL);
	      default: return(FALSE);
	    }
	  else
	    return(strchr(Table[cod].s,'e') != NULL);
}

word CTargetED(word PC)
{
	byte	cod;

	cod = PROG[PC] - 0x40;
	if (TableED[cod].att.JPa)
	  return(rd16(&PROG[PC+1]) - OFFSET);
	else
	  return(PC + 2 + rds8(PROG[PC+1]));
}

word CTargetIx(word PC)
{
	byte	cod;

	if (Table[cod = PROG[PC]].att.JPa)
	  return(rd16(&PROG[PC+1]) - OFFSET);
	else
	  return(PC + 2 + rds8(PROG[PC+1]));
}

word CTarget(word PC)
{
	byte	cod;

	if (Table[cod = PROG[PC]].att.Mult)
	  switch (cod) {
	    case 0xED: return(CTargetED(PC+1));
	    case 0xDD:
	    case 0xFD: return(CTargetIx(PC+1));
	    default: return(0);
	  }
	else
	  if (Table[cod].att.RST)
	    return(cod - 0xC7 - OFFSET);
	  else if (Table[cod].att.JPa)
	    return(rd16(&PROG[PC+1]) - OFFSET);
	  else	/*	Table[cod].att.JPr	*/
	    return(PC + 2 + rds8(PROG[PC+1]));
}

word DTarget(word PC)
{
	return(rd16(&PROG[PC+1+Table[PROG[PC]].att.Mult]) - OFFSET);
}

void SetToCode(word PC,word LEN)
{
	word	n;

	n = PC + LEN;
	while (PC < n)
	  TYPE[PC++].Code = 1;
}

static void TravelCompat(word PC)
{
	while ((PC <= LIMIT) && !TYPE[PC].Code) {
	  SetToCode(PC, LEN(PC));
	  if (JMP(PC))
	    TravelCompat(CTarget(PC));
	  if (RET(PC))
	    return;
	  PC = (word)(PC + LEN(PC));
	  if (TYPE[PC].Break)
	    return;
	}
}

/* Corrected depth-first walk. Same order as the recursive original
   (follow the branch target, then the fall-through) but the pending
   addresses live on the heap, and a target is accepted only when it
   is strictly inside the loaded image. Bytes already given a data
   class by the .PAR file are a barrier. An opcode that does not fit
   before LIMIT is not treated as code. */
static void TravelFixed(word start)
{
	word *stack;
	int sp = 0;

	stack = (word *)xmalloc(sizeof(word) * 65536u);
	stack[sp++] = start;
	while (sp > 0) {
	  word PC = stack[--sp];

	  while (PC < LIMIT && !TYPE[PC].Code && !TYPE[PC].Break
		 && TYPE[PC].Data == 0) {
	    word len = LEN(PC);
	    word i;
	    int spans = 0;

	    if (len == 0 || (unsigned)PC + (unsigned)len > (unsigned)LIMIT)
	      break;
	    for (i = 0; i < len; i++) {
	      if (TYPE[(word)(PC + i)].Data != 0)
	        spans = 1;
	    }
	    if (spans)
	      break;
	    SetToCode(PC, len);
	    if (JMP(PC)) {
	      word tgt = CTarget(PC);
	      if (tgt < LIMIT) {
	        if (!RET(PC)) {
	          word nxt = (word)(PC + len);
	          if (nxt < LIMIT && sp < 65536)
	            stack[sp++] = nxt;
	        }
	        if (sp < 65536)
	          stack[sp++] = tgt;
	        break;
	      }
	    }
	    if (RET(PC))
	      break;
	    PC = (word)(PC + len);
	    if (PC >= LIMIT || TYPE[PC].Break)
	      break;
	  }
	}
	free(stack);
}

void Travel(word PC)
{
	if (dis_compat)
	  TravelCompat(PC);
	else
	  TravelFixed(PC);
}


void LoadProgram(char *s)
{
	char n[512];
	FILE *fp;
	size_t nread;

	if (strlen(s) > sizeof n - 5) {
	  printf("\aError al abrir el fichero %s\n", s);
	  exit(2);
	}
	strcpy(n, s);
	if (strchr(n, '.') == NULL)
	  strcat(n, ".COM");
	fp = fopen(n, "rb");
	if (fp == NULL) {
	  printf("\aError al abrir el fichero %s\n", n);
	  exit(2);
	}
	nread = fread(PROG, 1, 0xFF00, fp);
	if (ferror(fp)) {
	  printf("\aError al abrir el fichero %s\n", n);
	  fclose(fp);
	  exit(2);
	}
	fclose(fp);
	dis_loaded = (word)nread;
	LIMIT = dis_loaded;
}


static int pull_hex(FILE *f, word *o)
{
	unsigned u;
	if (fscanf(f, "%x", &u) != 1)
	  return 0;
	*o = (word)u;
	return 1;
}

static int pull_hex2(FILE *f, word *a, word *b)
{
	unsigned u, v;
	if (fscanf(f, "%x%x", &u, &v) != 2)
	  return 0;
	*a = (word)u;
	*b = (word)v;
	return 1;
}

void DefineTypes(char *s)              /* Primera división en código y datos */
{
	FILE    *f;
	char    c;
	word    w1,w2;
	char    n[512];

	strcpy(n,s);
	if (strchr(n,'.') == NULL)
	  strcat(n,".PAR");
	if ((f = fopen(n,"rb")) == NULL) {
	  printf("\n\aError al abrir el fichero %s\n",n);
	  exit(3);
	}
	while (fscanf(f,"%c",&c) != EOF)
	  switch (toupper(c)) {
	    case 'O': pull_hex(f, &OFFSET);
		      break;
	    case 'U': pull_hex(f, &w1);
		      	  if (dis_compat) {
	    LIMIT = (word)(w1 - OFFSET);
	    if (LIMIT > 0xFF00)
	      LIMIT = 0xFF00;
	  } else if (w1 < OFFSET)
	    LIMIT = 0;
	  else {
	    unsigned span = (unsigned)w1 - (unsigned)OFFSET;
	    LIMIT = (span > dis_loaded) ? dis_loaded : (word)span;
	  }
		      break;
	    case 'B': pull_hex(f, &w1);
		      TYPE[w1 - OFFSET].Break = 1;
		      break;
	    case 'C': pull_hex2(f, &w1, &w2);
		      w1 -= OFFSET;
		      w2 -= OFFSET;
		      while (w1 <= w2) {
			TYPE[w1].Code = 0;
			TYPE[w1++].Data = 5;
		      }
		      break;
	    case 'D': pull_hex2(f, &w1, &w2);
		      w1 -= OFFSET;
		      w2 -= OFFSET;
		      while (w1 <= w2) {
			TYPE[w1].Code = 0;
			TYPE[w1++].Data = 4;
		      }
		      break;
	    case 'E': pull_hex(f, &w1);
		      w1 -= OFFSET;
		      TYPE[w1].Label = 1;
		      Travel(w1);
		      break;
	    case 'L': pull_hex(f, &w1);
		      TYPE[w1 - OFFSET].Label = 1;
		      break;
	    case 'S': { unsigned _u; if (fscanf(f, "%c%x", &c, &_u) == 2) w1 = (word)_u; }
		      w1 -= OFFSET;
	      	      TYPE[w1].Code  = 0;
		      TYPE[w1].Label = 1;
		      switch (c) {
			case '0': TYPE[w1].Data = 2;
				  break;
			case '$': TYPE[w1].Data = 3;
		      }
		      break;
	    case 'W': pull_hex2(f, &w1, &w2);
		      w1 -= OFFSET;
		      w2 -= OFFSET;
		      while (w1 <= w2) {
			TYPE[w1].Code = 0;
			TYPE[w1++].Data = 1;
			TYPE[w1++].Code = 0;
		      }
	  }
	fclose(f);
}

/* Etiqueta solo si el destino cae en la imagen cargada.
   Compat keeps the original `<= LIMIT` test (one past the end). */
static int label_if(word p)
{
	if (dis_compat) {
	  if (p <= LIMIT) {
	    TYPE[p].Label = 1;
	    return 1;
	  }
	} else if (p < LIMIT) {
	  TYPE[p].Label = 1;
	  return 1;
	}
	return 0;
}

void CheckTypes()		/* Precisar referencias para etiquetas	*/
{
	word	PC = 0;

	while (PC < LIMIT)
	  if (TYPE[PC].Code) {
	    if (JMP(PC)) {
	      label_if(CTarget(PC));
	    }
	    else
	      if (DAT(PC))
		label_if(DTarget(PC));
	    PC += LEN(PC);
	  }
	  else
	    PC++;
}

void insert(void)
{
	if (ptr) {
	  p->link = (LINK) xmalloc(sizeof(*ptr));
	  p = p->link;
	}
	else {
	  ptr = (LINK) xmalloc(sizeof(*ptr));
	  p = ptr;
	}
	p->link = 0;
}

void LoadSymbols(char *s)
{
	word    dir,i;
	unsigned dir_u;
	char	sym[7];
	char    n[512];
	FILE    *f;

	strcpy(n,s);
	if (strchr(n,'.') == NULL)
	  strcat(n,".SYM");
	if ((f = fopen(n,"rb")) == NULL) {
	  printf("\a\nError al abrir el fichero %s\n",n);
	  exit(4);
	}
	while (fscanf(f,"%x %6s",&dir_u,sym) == 2 && ((dir = (word)dir_u), 1)) {
	  insert();
	  ++symbols;
	  p->dir = dir;
	  strcpy(p->sym,sym);
	  TYPE[dir - OFFSET].Symbol = 1;
	}
	fclose(f);
	Sym = (SYM *) xmalloc(symbols * sizeof(SYM));
	for (i = 0; i < symbols; i++) {
	  p = ptr;
	  Sym[i].dir = p->dir;
	  strcpy(Sym[i].sym,p->sym);
	  ptr = p->link;
	  free(p);
	}
	qsort(Sym,symbols,sizeof(SYM),compare);
}

void ListProgram(char *s)
{
	word    PC = 0,PC1;
	char    n[512];

	strcpy(n,s);
	if (strchr(n,'.') == NULL)
	  strcat(n,".MAC");
	if ((m = fopen(n,"wb")) == NULL) {
	  printf("\a\nError al crear el fichero %s\n",n);
	  exit(5);
	}
	dis_fprintf(m,"\t.Z80\n\n\tORG\t");
	PrintHex(OFFSET,FALSE,4);
	dis_fprintf(m,"\n\n");
	while (PC < LIMIT) {
	  for (PC1 = PC; ((PC1 < LIMIT) && (TYPE[PC1].Code == TYPE[PC].Code)); PC1++) ;
	  if (TYPE[--PC1].Code)
	    DISASMCODE(PC,PC1);
	  else
	    DISASMDATA(PC,PC1);
	  PC = PC1 + 1;
	}
	dis_fprintf(m,"\n\tEND\n");
	fclose(m);
}

int main(int argc, char *argv[])
{
	dis_parse_flags(&argc, argv);
	printf("\nZ-80 DISASSEMBLER Versión 3.2 19-10-88\n");
	printf("  Universidad Central de Las Villas\n");
	printf("      Instituto de Informática\n");
	printf("     Lic. David Izada Rodríguez\n\n");
	if ((argc < 4) || (argc > 5)) {
	  printf("\aUso: DISZ80 [--compat] [--crlf] <.COM> <.PAR> <.MAC> [<.SYM>]\n");
	  exit(1);
	}
	LoadProgram(argv[1]);
	DefineTypes(argv[2]);
	CheckTypes();
	if (argc == 5)
	  LoadSymbols(argv[4]);
	ListProgram(argv[3]);
	return 0;
}
