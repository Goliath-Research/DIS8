/*	cl /Ox /Zp1 disz80.c /link /exe		*/

#include        <string.h>
#include        <memory.h>
#include        <fcntl.h>
#include        <io.h>
#include        <sys\types.h>
#include        <sys\stat.h>
#include        <stdlib.h>
#include        <stdio.h>
#include	<malloc.h>
#include	<search.h>

#define byte    unsigned char
#define	word	unsigned int

#define TRUE    1
#define FALSE   0

#define Nmax    0x8000

/*									*/
/*	Informaci¢n del fichero de s¡mbolos .SYM como lista enlazada	*/
/*									*/

typedef struct nodo {
	word		dir;
	char		sym[7];
	struct nodo	near *link;
} near *LINK;

/*									*/
/*	Informaci¢n del fichero de s¡mbolos .SYM como arreglo ordenado	*/
/*									*/

typedef struct {
	word	dir;
	char	sym[7];
} SYM;

/*									*/
/*	Informaci¢n necesaria por cada byte del programa analizado	*/
/*									*/

typedef struct {
	word	Code   : 1;	/*	0 - Dato		*/
				/*	1 - C¢digo		*/
	word	Data   : 3;	/*	000 - Byte		*/
				/*	001 - Word		*/
				/*	010 - String 0		*/
				/*	011 - String $		*/
				/*	100 - DS		*/
				/*	101 - Comentado		*/
	word	Label  : 1;	/*	0 - Sin etiqueta	*/
				/*      1 - Con etiqueta        */
	word    Break  : 1;	/*      0 - Continuar auto...   */
				/*      1 - Parar autom tico    */
	word	Symbol : 1;	/*	0 - No tiene s¡mbolo	*/
				/*	1 - Hay s¡mbolo		*/
} type;

/*								*/
/*	Informaci¢n necesaria por cada instrucci¢n Z-80		*/
/*								*/

typedef struct {
	word	Size : 2;	/*	Tama¤o en bytes		*/
	word	RST  : 1;	/*	Instrucci¢n RST		*/
	word	JPa  : 1;	/*	JP absoluto o CALL	*/
	word	JPr  : 1;	/*	JP relativo		*/
	word	RET  : 1;	/*	RET o JP incondicional	*/
	word	Mult : 1;	/*	Instrucci¢n m£ltiple	*/
	word	HL   : 1;	/*	Referencia HL		*/
} info;

typedef struct {
	info	att;
	char	near *s;	/*      w   - word              */
				/*	b   - byte		*/
				/*      e   - label             */
				/*	r   - relative		*/
				/*	i   - index IX,IY o HL	*/
				/*	h   - index XH,YH o H	*/
				/*	l   - index XL,YL o L	*/
} info0;

info0 Table[0x100] = {
	1,0,0,0,0,0,0,"NOP",
	3,0,0,0,0,0,0,"LD\tBC,w",
	1,0,0,0,0,0,0,"LD\t(BC),A",
	1,0,0,0,0,0,0,"INC\tBC",
	1,0,0,0,0,0,0,"INC\tB",
	1,0,0,0,0,0,0,"DEC\tB",
	2,0,0,0,0,0,0,"LD\tB,b",
	1,0,0,0,0,0,0,"RLCA",
	1,0,0,0,0,0,0,"EX\tAF,AF'",
	1,0,0,0,0,0,1,"ADD\ti,BC",
	1,0,0,0,0,0,0,"LD\tA,(BC)",
	1,0,0,0,0,0,0,"DEC\tBC",
	1,0,0,0,0,0,0,"INC\tC",
	1,0,0,0,0,0,0,"DEC\tC",
	2,0,0,0,0,0,0,"LD\tC,b",
	1,0,0,0,0,0,0,"RRCA",
	2,0,0,1,0,0,0,"DJNZ\tr",
	3,0,0,0,0,0,0,"LD\tDE,w",
	1,0,0,0,0,0,0,"LD\t(DE),A",
	1,0,0,0,0,0,0,"INC\tDE",
	1,0,0,0,0,0,0,"INC\tD",
	1,0,0,0,0,0,0,"DEC\tD",
	2,0,0,0,0,0,0,"LD\tD,b",
	1,0,0,0,0,0,0,"RLA",
	2,0,0,1,1,0,0,"JR\tr",
	1,0,0,0,0,0,1,"ADD\ti,DE",
	1,0,0,0,0,0,0,"LD\tA,(DE)",
	1,0,0,0,0,0,0,"DEC\tDE",
	1,0,0,0,0,0,0,"INC\tE",
	1,0,0,0,0,0,0,"DEC\tE",
	2,0,0,0,0,0,0,"LD\tE,b",
	1,0,0,0,0,0,0,"RRA",
	2,0,0,1,0,0,0,"JR\tNZ,r",
	3,0,0,0,0,0,1,"LD\ti,w",
	3,0,0,0,0,0,1,"LD\t(e),i",
	1,0,0,0,0,0,1,"INC\ti",
	1,0,0,0,0,0,1,"INC\th",
	1,0,0,0,0,0,1,"DEC\th",
	2,0,0,0,0,0,1,"LD\th,b",
	1,0,0,0,0,0,0,"DAA",
	2,0,0,1,0,0,0,"JR\tZ,r",
	1,0,0,0,0,0,1,"ADD\ti,i",
	3,0,0,0,0,0,1,"LD\ti,(e)",
	1,0,0,0,0,0,1,"DEC\ti",
	1,0,0,0,0,0,1,"INC\tl",
	1,0,0,0,0,0,1,"DEC\tl",
	2,0,0,0,0,0,1,"LD\tl,b",
	1,0,0,0,0,0,0,"CPL",
	2,0,0,1,0,0,0,"JR\tNC,r",
	3,0,0,0,0,0,0,"LD\tSP,w",
	3,0,0,0,0,0,0,"LD\t(e),A",
	1,0,0,0,0,0,0,"INC\tSP",
	1,0,0,0,0,0,1,"INC\t(i)",
	1,0,0,0,0,0,1,"DEC\t(i)",
	2,0,0,0,0,0,1,"LD\t(i),b",
	1,0,0,0,0,0,0,"SCF",
	2,0,0,1,0,0,0,"JR\tC,r",
	1,0,0,0,0,0,1,"ADD\ti,SP",
	3,0,0,0,0,0,0,"LD\tA,(e)",
	1,0,0,0,0,0,0,"DEC\tSP",
	1,0,0,0,0,0,0,"INC\tA",
	1,0,0,0,0,0,0,"DEC\tA",
	2,0,0,0,0,0,0,"LD\tA,b",
	1,0,0,0,0,0,0,"CCF",
	1,0,0,0,0,0,0,"LD\tB,B",
	1,0,0,0,0,0,0,"LD\tB,C",
	1,0,0,0,0,0,0,"LD\tB,D",
	1,0,0,0,0,0,0,"LD\tB,E",
	1,0,0,0,0,0,1,"LD\tB,h",
	1,0,0,0,0,0,1,"LD\tB,l",
	1,0,0,0,0,0,1,"LD\tB,(i)",
	1,0,0,0,0,0,0,"LD\tB,A",
	1,0,0,0,0,0,0,"LD\tC,B",
	1,0,0,0,0,0,0,"LD\tC,C",
	1,0,0,0,0,0,0,"LD\tC,D",
	1,0,0,0,0,0,0,"LD\tC,E",
	1,0,0,0,0,0,1,"LD\tC,h",
	1,0,0,0,0,0,1,"LD\tC,l",
	1,0,0,0,0,0,1,"LD\tC,(i)",
	1,0,0,0,0,0,0,"LD\tC,A",
	1,0,0,0,0,0,0,"LD\tD,B",
	1,0,0,0,0,0,0,"LD\tD,C",
	1,0,0,0,0,0,0,"LD\tD,D",
	1,0,0,0,0,0,0,"LD\tD,E",
	1,0,0,0,0,0,1,"LD\tD,h",
	1,0,0,0,0,0,1,"LD\tD,l",
	1,0,0,0,0,0,1,"LD\tD,(i)",
	1,0,0,0,0,0,0,"LD\tD,A",
	1,0,0,0,0,0,0,"LD\tE,B",
	1,0,0,0,0,0,0,"LD\tE,C",
	1,0,0,0,0,0,0,"LD\tE,D",
	1,0,0,0,0,0,0,"LD\tE,E",
	1,0,0,0,0,0,1,"LD\tE,h",
	1,0,0,0,0,0,1,"LD\tE,l",
	1,0,0,0,0,0,1,"LD\tE,(i)",
	1,0,0,0,0,0,0,"LD\tE,A",
	1,0,0,0,0,0,1,"LD\th,B",
	1,0,0,0,0,0,1,"LD\th,C",
	1,0,0,0,0,0,1,"LD\th,D",
	1,0,0,0,0,0,1,"LD\th,E",
	1,0,0,0,0,0,0,"LD\th,h",
	1,0,0,0,0,0,1,"LD\th,l",
	1,0,0,0,0,0,1,"LD\th,(i)",
	1,0,0,0,0,0,1,"LD\th,A",
	1,0,0,0,0,0,1,"LD\tl,B",
	1,0,0,0,0,0,1,"LD\tl,C",
	1,0,0,0,0,0,1,"LD\tl,D",
	1,0,0,0,0,0,1,"LD\tl,E",
	1,0,0,0,0,0,1,"LD\tl,h",
	1,0,0,0,0,0,1,"LD\tl,l",
	1,0,0,0,0,0,1,"LD\tL,(i)",
	1,0,0,0,0,0,1,"LD\tl,A",
	1,0,0,0,0,0,1,"LD\t(i),B",
	1,0,0,0,0,0,1,"LD\t(i),C",
	1,0,0,0,0,0,1,"LD\t(i),D",
	1,0,0,0,0,0,1,"LD\t(i),E",
	1,0,0,0,0,0,1,"LD\t(i),H",
	1,0,0,0,0,0,1,"LD\t(i),L",
	1,0,0,0,0,0,0,"HALT",
	1,0,0,0,0,0,1,"LD\t(i),A",
	1,0,0,0,0,0,0,"LD\tA,B",
	1,0,0,0,0,0,0,"LD\tA,C",
	1,0,0,0,0,0,0,"LD\tA,D",
	1,0,0,0,0,0,0,"LD\tA,E",
	1,0,0,0,0,0,1,"LD\tA,h",
	1,0,0,0,0,0,1,"LD\tA,l",
	1,0,0,0,0,0,1,"LD\tA,(i)",
	1,0,0,0,0,0,0,"LD\tA,A",
	1,0,0,0,0,0,0,"ADD\tA,B",
	1,0,0,0,0,0,0,"ADD\tA,C",
	1,0,0,0,0,0,0,"ADD\tA,D",
	1,0,0,0,0,0,0,"ADD\tA,E",
	1,0,0,0,0,0,1,"ADD\tA,h",
	1,0,0,0,0,0,1,"ADD\tA,l",
	1,0,0,0,0,0,1,"ADD\tA,(i)",
	1,0,0,0,0,0,0,"ADD\tA,A",
	1,0,0,0,0,0,0,"ADC\tA,B",
	1,0,0,0,0,0,0,"ADC\tA,C",
	1,0,0,0,0,0,0,"ADC\tA,D",
	1,0,0,0,0,0,0,"ADC\tA,E",
	1,0,0,0,0,0,1,"ADC\tA,h",
	1,0,0,0,0,0,1,"ADC\tA,l",
	1,0,0,0,0,0,1,"ADC\tA,(i)",
	1,0,0,0,0,0,0,"ADC\tA,A",
	1,0,0,0,0,0,0,"SUB\tB",
	1,0,0,0,0,0,0,"SUB\tC",
	1,0,0,0,0,0,0,"SUB\tD",
	1,0,0,0,0,0,0,"SUB\tE",
	1,0,0,0,0,0,1,"SUB\th",
	1,0,0,0,0,0,1,"SUB\tl",
	1,0,0,0,0,0,1,"SUB\t(i)",
	1,0,0,0,0,0,0,"SUB\tA",
	1,0,0,0,0,0,0,"SBC\tA,B",
	1,0,0,0,0,0,0,"SBC\tA,C",
	1,0,0,0,0,0,0,"SBC\tA,D",
	1,0,0,0,0,0,0,"SBC\tA,E",
	1,0,0,0,0,0,1,"SBC\tA,h",
	1,0,0,0,0,0,1,"SBC\tA,l",
	1,0,0,0,0,0,1,"SBC\tA,(i)",
	1,0,0,0,0,0,0,"SBC\tA,A",
	1,0,0,0,0,0,0,"AND\tB",
	1,0,0,0,0,0,0,"AND\tC",
	1,0,0,0,0,0,0,"AND\tD",
	1,0,0,0,0,0,0,"AND\tE",
	1,0,0,0,0,0,1,"AND\th",
	1,0,0,0,0,0,1,"AND\tl",
	1,0,0,0,0,0,1,"AND\t(i)",
	1,0,0,0,0,0,0,"AND\tA",
	1,0,0,0,0,0,0,"XOR\tB",
	1,0,0,0,0,0,0,"XOR\tC",
	1,0,0,0,0,0,0,"XOR\tD",
	1,0,0,0,0,0,0,"XOR\tE",
	1,0,0,0,0,0,1,"XOR\th",
	1,0,0,0,0,0,1,"XOR\tl",
	1,0,0,0,0,0,1,"XOR\t(i)",
	1,0,0,0,0,0,0,"XOR\tA",
	1,0,0,0,0,0,0,"OR\tB",
	1,0,0,0,0,0,0,"OR\tC",
	1,0,0,0,0,0,0,"OR\tD",
	1,0,0,0,0,0,0,"OR\tE",
	1,0,0,0,0,0,1,"OR\th",
	1,0,0,0,0,0,1,"OR\tl",
	1,0,0,0,0,0,1,"OR\t(i)",
	1,0,0,0,0,0,0,"OR\tA",
	1,0,0,0,0,0,0,"CP\tB",
	1,0,0,0,0,0,0,"CP\tC",
	1,0,0,0,0,0,0,"CP\tD",
	1,0,0,0,0,0,0,"CP\tE",
	1,0,0,0,0,0,1,"CP\th",
	1,0,0,0,0,0,1,"CP\tl",
	1,0,0,0,0,0,1,"CP\t(i)",
	1,0,0,0,0,0,0,"CP\tA",
	1,0,0,0,0,0,0,"RET\tNZ",
	1,0,0,0,0,0,0,"POP\tBC",
	3,0,1,0,0,0,0,"JP\tNZ,e",
	3,0,1,0,1,0,0,"JP\te",
	3,0,1,0,0,0,0,"CALL\tNZ,e",
	1,0,0,0,0,0,0,"PUSH\tBC",
	2,0,0,0,0,0,0,"ADD\tA,b",
	1,1,1,0,0,0,0,"RST\t0",
	1,0,0,0,0,0,0,"RET\tZ",
	1,0,0,0,1,0,0,"RET",
	3,0,1,0,0,0,0,"JP\tZ,e",
	0,0,0,0,0,1,0,"",			/*	CB	*/
	3,0,1,0,0,0,0,"CALL\tZ,e",
	3,0,1,0,0,0,0,"CALL\te",
	2,0,0,0,0,0,0,"ADC\tA,b",
	1,1,1,0,0,0,0,"RST\t8",
	1,0,0,0,0,0,0,"RET\tNC",
	1,0,0,0,0,0,0,"POP\tDE",
	3,0,1,0,0,0,0,"JP\tNC,e",
	2,0,0,0,0,0,0,"OUT\t(b),A",
	3,0,1,0,0,0,0,"CALL\tNC,e",
	1,0,0,0,0,0,0,"PUSH\tDE",
	2,0,0,0,0,0,0,"SUB\tb",
	1,1,1,0,0,0,0,"RST\t10H",
	1,0,0,0,0,0,0,"RET\tC",
	1,0,0,0,0,0,0,"EXX",
	3,0,1,0,0,0,0,"JP\tC,e",
	2,0,0,0,0,0,0,"IN\tA,(b)",
	3,0,1,0,0,0,0,"CALL\tC,e",
	0,0,0,0,0,1,0,"",			/*	DD - index IX	*/
	2,0,0,0,0,0,0,"SBC\tA,b",
	1,1,1,0,0,0,0,"RST\t18H",
	1,0,0,0,0,0,0,"RET\tPO",
	1,0,0,0,0,0,1,"POP\ti",
	3,0,1,0,0,0,0,"JP\tPO,e",
	1,0,0,0,0,0,1,"EX\t(SP),i",
	3,0,1,0,0,0,0,"CALL\tPO,e",
	1,0,0,0,0,0,1,"PUSH\ti",
	2,0,0,0,0,0,0,"AND\tb",
	1,1,1,0,0,0,0,"RST\t20H",
	1,0,0,0,0,0,0,"RET\tPE",
	1,0,0,0,1,0,1,"JP\t(i)",
	3,0,1,0,0,0,0,"JP\tPE,e",
	1,0,0,0,0,0,1,"EX\tDE,i",
	3,0,1,0,0,0,0,"CALL\tPE,e",
	0,0,0,0,0,1,0,"",			/*	DD	*/
	2,0,0,0,0,0,0,"XOR\tb",
	1,1,1,0,0,0,0,"RST\t28H",
	1,0,0,0,0,0,0,"RET\tP",
	1,0,0,0,0,0,0,"POP\tAF",
	3,0,1,0,0,0,0,"JP\tP,e",
	1,0,0,0,0,0,0,"DI",
	3,0,1,0,0,0,0,"CALL\tP,e",
	1,0,0,0,0,0,0,"PUSH\tAF",
	2,0,0,0,0,0,0,"OR\tb",
	1,1,1,0,0,0,0,"RST\t30H",
	1,0,0,0,0,0,0,"RET\tM",
	1,0,0,0,0,0,1,"LD\tSP,i",
	3,0,1,0,0,0,0,"JP\tM,e",
	1,0,0,0,0,0,0,"EI",
	3,0,1,0,0,0,0,"CALL\tM,e",
	0,0,0,0,0,1,0,"",			/*	FD - index IY	*/
	2,0,0,0,0,0,0,"CP\tb",
	1,1,1,0,0,0,0,"RST\t38H"};

info0	TableED[0x7C] = {		/*	Comienza en 0x40	*/
	1,0,0,0,0,0,0,"IN\tB,(C)",
	1,0,0,0,0,0,0,"OUT\t(C),B",
	1,0,0,0,0,0,0,"SBC\tHL,BC",
	3,0,0,0,0,0,0,"LD\t(e),BC",
	1,0,0,0,0,0,0,"NEG",
	1,0,0,0,1,0,0,"RETN",
	1,0,0,0,0,0,0,"IM\t0",
	1,0,0,0,0,0,0,"LD\tI,A",
	1,0,0,0,0,0,0,"INC\tC,(C)",
	1,0,0,0,0,0,0,"OUT\t(C),C",
	1,0,0,0,0,0,0,"ADC\tHL,BC",
	3,0,0,0,0,0,0,"LD\tBC,(e)",
	0,0,0,0,0,0,0,"",
	1,0,0,0,1,0,0,"RETI",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	1,0,0,0,0,0,0,"IN\tD,(C)",
	1,0,0,0,0,0,0,"OUT\t(C),D",
	1,0,0,0,0,0,0,"SBC\tHL,DE",
	3,0,0,0,0,0,0,"LD\t(e),DE",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	1,0,0,0,0,0,0,"IM\t1",
	1,0,0,0,0,0,0,"LD\tA,I",
	1,0,0,0,0,0,0,"IN\tE,(C)",
	1,0,0,0,0,0,0,"OUT\t(C),E",
	1,0,0,0,0,0,0,"ADC\tHL,DE",
	3,0,0,0,0,0,0,"LD\tDE,(e)",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	1,0,0,0,0,0,0,"IM\t2",
	0,0,0,0,0,0,0,"",
	1,0,0,0,0,0,0,"IN\tH,(C)",
	1,0,0,0,0,0,0,"OUT\t(C),H",
	1,0,0,0,0,0,0,"SBC\tHL,HL",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	1,0,0,0,0,0,0,"RRD",
	1,0,0,0,0,0,0,"IN\tL,(C)",
	1,0,0,0,0,0,0,"OUT\t(C),L",
	1,0,0,0,0,0,0,"ADC\tHL,HL",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	1,0,0,0,0,0,0,"RLD",
	1,0,0,0,0,0,0,"IN\tF,(C)",
	0,0,0,0,0,0,0,"",
	1,0,0,0,0,0,0,"SBC\tHL,SP",
	3,0,0,0,0,0,0,"LD\t(e),SP",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	1,0,0,0,0,0,0,"IN\tA,(C)",
	1,0,0,0,0,0,0,"OUT\t(C),A",
	1,0,0,0,0,0,0,"ADC\tHL,SP",
	3,0,0,0,0,0,0,"LD\tSP,(e)",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	1,0,0,0,0,0,0,"LDI",
	1,0,0,0,0,0,0,"CPI",
	1,0,0,0,0,0,0,"INI",
	1,0,0,0,0,0,0,"OUTI",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	1,0,0,0,0,0,0,"LDD",
	1,0,0,0,0,0,0,"CPD",
	1,0,0,0,0,0,0,"IND",
	1,0,0,0,0,0,0,"OUTD",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	1,0,0,0,0,0,0,"LDIR",
	1,0,0,0,0,0,0,"CPIR",
	1,0,0,0,0,0,0,"INIR",
	1,0,0,0,0,0,0,"OTIR",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	0,0,0,0,0,0,0,"",
	1,0,0,0,0,0,0,"LDDR",
	1,0,0,0,0,0,0,"CPDR",
	1,0,0,0,0,0,0,"INDR",
	1,0,0,0,0,0,0,"OTDR"};

char near *Reg[10] = {"B","C","D","E","H","L","(HL)","A","IX","IY"};
char near *Cod[11] = {"RLC","RRC","RL","RR","SLA","SRA","SLL","SRL","BIT","RES","SET"};

byte    PROG[Nmax];
word    PC,LIMIT,OFFSET = 0x100,symbols = 0;
type	far TYPE[Nmax];
LINK	ptr = 0,p,q;
SYM	*Sym = 0;
FILE    *m;

word LENED(word PC)
{
	byte	cod;

	cod = PROG[PC];
	if ((cod < 0x40) || (TableED[cod - 0x40].s == ""))
	  return(1);
	else
	  return(TableED[cod - 0x40].att.Size+1);
}

word LENIxCB(word PC)
{
	byte	cod;

	if ((PROG[PC + 1] >> 3 + 2) % 8)
	  return(1);
	else
	  return(4);
}

word LENIx(word PC)
{
	byte	cod;

	if (Table[cod = PROG[PC]].att.HL)
	  return(Table[cod].att.Size + 1 /***/);
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
	  }
	else
	  return(Table[cod].att.Size);
}

word PrintHex(word w, int label, int count)
{
	int     i,j;
	word    k = 0;
	char    s[3];

	for (i = count; i; ) {
	  j = w % 16;
	  w >>= 4;
	  s[--i] = (j < 10) ? ('0' + j) : ('A' + j - 10);
	}
	if (label)
	  k++,putc('L',m);
	else {
	  if (s[0] > '9')
	    k++,putc('0',m);
	}
	for (i = 0; i < count; i++)
	  k++,putc(s[i],m);
	if (!label)
	  k++,putc('H',m);
	return(k);
}

int cdecl compare(p,q)
SYM	*p,*q;
{
	return(p->dir - q->dir);
}

void PrintDIS(word *IP)
{
	putc('+',m);
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
	    case 'w': PrintHex(*(word *) &PROG[++IP],FALSE,4);
		      IP += 2;
		      break;
	    case 'e': label = *(word *) &PROG[++IP];
	    	      if ((Sym == 0) ||
		          ((p = bsearch(&label,Sym,symbols,sizeof(SYM),compare)) == NULL))
			PrintHex(label,TRUE,4);
		      else
		        fputs(p->sym,m);
		      IP += 2;
		      break;
	    case 'b': PrintHex(b = (byte) PROG[++IP],FALSE,2);
		      Byt = TRUE;
		      IP += 1;
		      break;
	    case 'r': PrintHex(OFFSET + PC + 2 + (signed char) PROG[++IP],TRUE,4);
		      break;
	    case 'i': switch (PROG[PC]) {
			case 0xDD: fprintf(m,"IX");
				   if (Par)
				     PrintDIS(&IP);
				   break;
			case 0xFD: fprintf(m,"IY");
				   if (Par)
				     PrintDIS(&IP);
				   break;
			default  : fprintf(m,"HL");
		      };
		      break;
	    case 'h': switch (PROG[PC]) {
			case 0xDD: fprintf(m,"XH");
				   break;
			case 0xFD: fprintf(m,"YH");
				   break;
			default  : fprintf(m,"H");
		      };
		      break;
	    case 'l': switch (PROG[PC]) {
			case 0xDD: fprintf(m,"XL");
				   break;
			case 0xFD: fprintf(m,"YL");
				   break;
			default  : fprintf(m,"L");
		      };
		      break;
	    default:
		      if (!Par)
		        Par = (*s == '(');
		      if (*s == ')')
		        Par = FALSE;
		      putc(*s,m);
	  }
	  s++;
	}
	if (Byt)
	  if ((b > 31) && (b < 127))
	    fprintf(m,"\t\t\t; '%c'",b);
	putc('\n',m);
}

void Error()
{
	fprintf(m,"??\t\t;");
	PrintHex(PROG[PC],FALSE,2);
	putc('\n',m);
}

void Proc1CB(int n)
{
	fprintf(m,"%s\t%s\n",Cod[n],Reg[PROG[PC] % 8]);
}

void Proc2CB(int n)
{
	fprintf(m,"%s\t%c,%s\n",Cod[n],'0' + ((PROG[PC] >> 3) & 0x07),Reg[PROG[PC] % 8]);
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
	fprintf(m,"%s\t(%s+",Cod[n],Reg[8+((PROG[PC-1] & 0x20) >> 5)]);
	PrintHex(PROG[PC+1],FALSE,2);
	fprintf(m,")\n");
}

void Proc2IxCB(int n)
{
	fprintf(m,"%s\t%c,(%s+",Cod[n],'0' + ((PROG[PC+2] >> 3) & 0x07),Reg[8+((PROG[PC-1] & 0x20) >> 5)]);
	PrintHex(PROG[PC+1],FALSE,2);
	fprintf(m,")\n");
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
	  PC += Table[cod].att.Size + 1;	/*  + (strchr(Table[PROG[PC+1]].s,'(') != NULL); */
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
	if ((cod < 0x40) || (TableED[cod - 0x40].s == "")) {
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
	      fputs(p->sym,m);
	    }
	    else
	      PrintHex(label,TRUE,4);
	    putc(':',m);
	  }
	  putc('\t',m);
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

	fprintf(m,"\tDB\t");
	do {
	  b = PROG[PC];
	  if ((b > 31) && (b < 127)) {
	    if (!ASCII) {
	      putc('\'',m);
	      ASCII = TRUE;
	    }
	    putc(b,m);
	    if (b == '\'')
	      putc('\'',m);
	  }
	  else {
	    if (ASCII) {
	      fprintf(m,"\',");
	      ASCII = FALSE;
	    }
	    PrintHex(b,FALSE,2);
	    if (b != c)
	      putc(',',m);
	  }
	}
	while (PROG[PC++] != c);
	if (ASCII)
	  putc('\'',m);
	putc('\n',m);
}

word PrintDB(word IP,word len)
{
	word	i,c = 0;

	fprintf(m,"\tDB\t");
	for (i = 1; i <= len; i++) {
	  c += PrintHex(PROG[IP++],FALSE,2);
	  if (i != len)
	    c++,putc(',',m);
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
	    putc('\t',m);
	  putc(';',m);
	  PrintHex(PC + OFFSET,FALSE,4);
	  putc('\t',m);
	  DISASMCODE(PC,PC+len-1);
	  count -= len;
	}
	if (count) {
	  PrintDB(PC,count);
	  putc('\n',m);
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
	      fputs(p->sym,m);
	    }
	    else
	      PrintHex(label,TRUE,4);
	    putc(':',m);
	  }
	  switch (TYPE[PC].Data) {
	    case 0: fprintf(m,"\tDB\t");
		    PrintHex(cod = PROG[PC],FALSE,2);
		    if (!TYPE[PC].Label) {
		      fprintf(m,"\t\t\t;");
		      PrintHex(PC + OFFSET,FALSE,4);
		      if ((cod > 31) && (cod < 127))
			fprintf(m," '%c'",cod);
		    }
		    putc('\n',m);
		    PC++;
		    break;
	    case 1: fprintf(m,"\tDW\t");
		    PrintHex(*(word *) &PROG[PC],FALSE,4);
		    if (!TYPE[PC].Label) {
		      fprintf(m,"\t\t\t;");
		      PrintHex(PC + OFFSET,FALSE,4);
		    }
		    PC += 2;
		    putc('\n',m);
		    break;
	    case 2: PrintString(0);
		    break;
	    case 3: PrintString('$');
		    break;
	    case 4: fprintf(m,"\tDS\t");
		    for (count = 1, PC++; ((TYPE[PC].Data == 4) && (!TYPE[PC].Label)); count++, PC++) ;
		    PrintHex(count,FALSE,4);
		    putc('\n',m);
		    break;
	    case 5: DISASMComment();
	  }
	}
}

int JMPED(word PC)
{
	byte	cod;

	cod = PROG[PC];
	if ((cod < 0x40) || (TableED[cod - 0x40].s == ""))
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
	  }
	else
	  return(Table[cod].att.JPa || Table[cod].att.JPr || Table[cod].att.RST);
}

int RETED(word PC)
{
	byte	cod;

	cod = PROG[PC];
	if ((cod < 0x40) || (TableED[cod - 0x40].s == ""))
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
	      case 0xED: return(strchr(TableED[PROG[PC+1]].s,'e') != NULL);
	      case 0xDD:
	      case 0xFD: return(strchr(Table[PROG[PC+1]].s,'e') != NULL);
	    }
	  else
	    return(strchr(Table[cod].s,'e') != NULL);
}

word CTargetED(word PC)
{
	byte	cod;

	cod = PROG[PC] - 0x40;
	if (TableED[cod].att.JPa)
	  return(*(word *) &PROG[PC+1] - OFFSET);
	else
	  return(PC + 2 + *(signed char *) &PROG[PC+1]);
}

word CTargetIx(word PC)
{
	byte	cod;

	if (Table[cod = PROG[PC]].att.JPa)
	  return(*(word *) &PROG[PC+1] - OFFSET);
	else
	  return(PC + 2 + *(signed char *) &PROG[PC+1]);
}

word CTarget(word PC)
{
	byte	cod;

	if (Table[cod = PROG[PC]].att.Mult)
	  switch (cod) {
	    case 0xED: return(CTargetED(PC+1));
	    case 0xDD:
	    case 0xFD: return(CTargetIx(PC+1));
	  }
	else
	  if (Table[cod].att.RST)
	    return(cod - 0xC7 - OFFSET);
	  else if (Table[cod].att.JPa)
	    return(*(word *) &PROG[PC+1] - OFFSET);
	  else	/*	Table[cod].att.JPr	*/
	    return(PC + 2 + *(signed char *) &PROG[PC+1]);
}

word DTarget(word PC)
{
	return(*(word *) &PROG[PC+1+Table[PROG[PC]].att.Mult] - OFFSET);
}

void SetToCode(word PC,word LEN)
{
	word	n;

	n = PC + LEN;
	while (PC < n)
	  TYPE[PC++].Code = 1;
}

void Travel(word PC)
{
	while ((PC <= LIMIT) && !TYPE[PC].Code) {
	  SetToCode(PC,LEN(PC));
	  if (JMP(PC))
	    Travel(CTarget(PC));
	  if (RET(PC))
	    return;
	  PC += LEN(PC);
	  if (TYPE[PC].Break)
	    return;
	}
}

void LoadProgram(char *s)
{
	int	h;
	char    n[80];

	strcpy(n,s);
	if (strchr(n,'.') == NULL)
	  strcat(n,".COM");

	if ((h = open(n,O_BINARY,S_IREAD)) == -1) {
	  printf("\aError al abrir el fichero %s\n",n);
	  exit(2);
	}
	LIMIT = read(h,(char *) PROG,0xff00);
	close(h);
}

void DefineTypes(char *s)              /* Primera divisi¢n en c¢digo y datos */
{
	FILE    *f;
	char    c;
	word    w1,w2;
	char    n[80];

	strcpy(n,s);
	if (strchr(n,'.') == NULL)
	  strcat(n,".PAR");
	if ((f = fopen(n,"r")) == NULL) {
	  printf("\n\aError al abrir el fichero %s\n",n);
	  exit(3);
	}
	while (fscanf(f,"%c",&c) != EOF)
	  switch (toupper(c)) {
	    case 'O': fscanf(f,"%x\n",&OFFSET);
		      break;
	    case 'U': fscanf(f,"%x\n",&w1);
		      LIMIT = w1 - OFFSET;
		      break;
	    case 'B': fscanf(f,"%x\n",&w1);
		      TYPE[w1 - OFFSET].Break = 1;
		      break;
	    case 'C': fscanf(f,"%x%x\n",&w1,&w2);
		      w1 -= OFFSET;
		      w2 -= OFFSET;
		      while (w1 <= w2) {
			TYPE[w1].Code = 0;
			TYPE[w1++].Data = 5;
		      }
		      break;
	    case 'D': fscanf(f,"%x%x\n",&w1,&w2);
		      w1 -= OFFSET;
		      w2 -= OFFSET;
		      while (w1 <= w2) {
			TYPE[w1].Code = 0;
			TYPE[w1++].Data = 4;
		      }
		      break;
	    case 'E': fscanf(f,"%x\n",&w1);
		      w1 -= OFFSET;
		      TYPE[w1].Label = 1;
		      Travel(w1);
		      break;
	    case 'L': fscanf(f,"%x\n",&w1);
		      TYPE[w1 - OFFSET].Label = 1;
		      break;
	    case 'S': fscanf(f,"%c%x\n",&c,&w1);
		      w1 -= OFFSET;
	      	      TYPE[w1].Code  = 0;
		      TYPE[w1].Label = 1;
		      switch (c) {
			case '0': TYPE[w1].Data = 2;
				  break;
			case '$': TYPE[w1].Data = 3;
		      }
		      break;
	    case 'W': fscanf(f,"%x%x\n",&w1,&w2);
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

void CheckTypes()		/* Precisar referencias para etiquetas	*/
{
	word	p,PC = 0;

	while (PC < LIMIT)
	  if (TYPE[PC].Code) {
	    if (JMP(PC)) {
	      if ((p = CTarget(PC)) <= LIMIT)
		TYPE[p].Label = 1;
	    }
	    else
	      if (DAT(PC))
		if ((p = DTarget(PC)) <= LIMIT)
		  TYPE[p].Label = 1;
	    PC += LEN(PC);
	  }
	  else
	    PC++;
}

insert()
{
	if (ptr) {
	  p->link = (LINK) malloc(sizeof(*ptr));
	  p = p->link;
	}
	else {
	  ptr = (LINK) malloc(sizeof(*ptr));
	  p = ptr;
	}
	p->link = 0;
}

void LoadSymbols(char *s)
{
	word    dir,i;
	char	sym[7];
	char    n[80];
	FILE    *f;

	strcpy(n,s);
	if (strchr(n,'.') == NULL)
	  strcat(n,".SYM");
	if ((f = fopen(n,"r")) == NULL) {
	  printf("\a\nError al abrir el fichero %s\n",n);
	  exit(4);
	}
	while (fscanf(f,"%x %s",&dir,sym) == 2) {
	  insert();
	  ++symbols;
	  p->dir = dir;
	  strcpy(p->sym,sym);
	  TYPE[dir - OFFSET].Symbol = 1;
	}
	fclose(f);
	Sym = (SYM *) malloc(symbols * sizeof(SYM));
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
	char    n[80];

	strcpy(n,s);
	if (strchr(n,'.') == NULL)
	  strcat(n,".MAC");
	if ((m = fopen(n,"w")) == NULL) {
	  printf("\a\nError al crear el fichero %s\n",n);
	  exit(5);
	}
	fprintf(m,"\t.Z80\n\n\tORG\t");
	PrintHex(OFFSET,FALSE,4);
	fprintf(m,"\n\n");
	while (PC < LIMIT) {
	  for (PC1 = PC; ((PC1 < LIMIT) && (TYPE[PC1].Code == TYPE[PC].Code)); PC1++) ;
	  if (TYPE[--PC1].Code)
	    DISASMCODE(PC,PC1);
	  else
	    DISASMDATA(PC,PC1);
	  PC = PC1 + 1;
	}
	fprintf(m,"\n\tEND\n");
	fclose(m);
}

cdecl main(int argc,char *argv[])
{
	printf("\nZ-80 DISASSEMBLER Versi¢n 3.2 19-10-88\n");
	printf("  Universidad Central de Las Villas\n");
	printf("      Instituto de Inform tica\n");
	printf("     Lic. David Izada Rodr¡guez\n\n");
	if ((argc < 4) || (argc > 5)) {
	  printf("\aUso: DISZ80 <.COM> <.PAR> <.MAC> [<.SYM>]\n");
	  exit(1);
	}
	LoadProgram(argv[1]);
	DefineTypes(argv[2]);
	CheckTypes();
	if (argc == 5)
	  LoadSymbols(argv[4]);
	ListProgram(argv[3]);
	exit(0);
}
