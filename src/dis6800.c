/*
 * Portable C11 port of DIS6800.C (6800, 24-12-90) by Lic. David Izada Rodriguez
 * (Instituto de Informatica, Universidad Central de Las Villas).
 * Spanish comments from the original are kept.
 *
 *   --compat   original control-flow walk, for tests against the DOS EXE
 *   --crlf     CR+LF output, as DOS text mode wrote it
 *
 * The default walk is the corrected one. See docs/known-issues.md.
 * / cl /Ox /Gc dis6800.c
 */
#include "dis_port.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* cl /Ox /Gc dis8.c /link /exe */






/*								 */
/*     Lista enlazada de cadenas de caracteres y su dirección	 */
/*								 */

typedef
  struct nodo
    {
      word	    dir;
      char	   *str;
      struct nodo  *link;
    } *LINK;

/*								 */
/*   Cadenas de caracteres en un arreglo ordenado por dirección  */
/*								 */

typedef
  struct
    {
      word	dir;
      char	*str;
    } STR;

/*									*/
/*	Información necesaria por cada byte del programa analizado	*/
/*									*/

typedef
  struct
    {
      unsigned Code : 1;	/*	0 - Dato		*/
			/*	1 - Código		*/
      unsigned Data : 3;	/*	000 - Byte		*/
			/*	001 - Word		*/
			/*	010 - String 0		*/
			/*	011 - String $		*/
			/*	100 - DS		*/
			/*	101 - Comentado 	*/
      unsigned Label : 1;	/*	0 - Sin etiqueta	*/
			/*      1 - Con etiqueta        */
      unsigned Break : 1;	/*      0 - Continuar auto...   */
			/*      1 - Parar automático    */
      unsigned Symbol : 1;	/*	0 - No tiene símbolo	*/
			/*	1 - Hay símbolo 	*/
      unsigned Commen : 1;	/*	0 - Sin comentario	*/
			/*	1 - Hay comentario	*/
    } type;

/*								*/
/*	Información necesaria por cada instrucción Z-80		*/
/*								*/

typedef
  struct
    {
      unsigned Size : 2;	/*	Tamaño en bytes		*/
      unsigned JPa : 1;	/*	JP absoluto o CALL	*/
      unsigned JPr : 1;	/*	JP relativo		*/
      unsigned Data : 1;	/*	Referencia a dato	*/
      unsigned Stop : 1;	/*	RET o JP incondicional	*/
      char *s;	/*	w   - data word		*/
			/*	d   - data byte 	*/
			/*	b   - byte		*/
			/*      e   - label             */
			/*	r   - relative		*/
			/*	x   - index X		*/
    } info;

info Table[0x100] =
  {
    {0,0,0,0,0,""},
    {1,0,0,0,0,"NOP"},
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {1,0,0,0,0,"TAP"},
    {1,0,0,0,0,"TPA"},
    {1,0,0,0,0,"INX"},
    {1,0,0,0,0,"DEX"},
    {1,0,0,0,0,"CLV"},
    {1,0,0,0,0,"SEV"},
    {1,0,0,0,0,"CLC"},
    {1,0,0,0,0,"SEC"},
    {1,0,0,0,0,"CLI"},
    {1,0,0,0,0,"SEI"},
    {1,0,0,0,0,"SBA"},			/* 10 */
    {1,0,0,0,0,"CBA"},
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {1,0,0,0,0,"TAB"},
    {1,0,0,0,0,"TBA"},
    {0,0,0,0,0,""},
    {1,0,0,0,0,"DAA"},
    {0,0,0,0,0,""},
    {1,0,0,0,0,"ABA"},
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {2,1,0,0,0,"BRA	r"},		/* 20 */
    {0,0,0,0,0,""},
    {2,0,1,0,0,"BHI	r"},
    {2,0,1,0,0,"BLS	r"},
    {2,0,1,0,0,"BCC	r"},
    {2,0,1,0,0,"BCS	r"},
    {2,0,1,0,0,"BNE	r"},
    {2,0,1,0,0,"BEQ	r"},
    {2,0,1,0,0,"BVC	r"},
    {2,0,1,0,0,"BVS	r"},
    {2,0,1,0,0,"BPL	r"},
    {2,0,1,0,0,"BMI	r"},
    {2,0,1,0,0,"BGE	r"},
    {2,0,1,0,0,"BLT	r"},
    {2,0,1,0,0,"BGT	r"},
    {2,0,1,0,0,"BLE	r"},
    {1,0,0,0,0,"TSX"},			/* 30 */
    {1,0,0,0,0,"INS"},
    {1,0,0,0,0,"PULA"},
    {1,0,0,0,0,"PULB"},
    {1,0,0,0,0,"DES"},
    {1,0,0,0,0,"TXS"},
    {1,0,0,0,0,"PSHA"},
    {1,0,0,0,0,"PSHB"},
    {0,0,0,0,0,""},
    {1,0,0,0,1,"RTS"},
    {0,0,0,0,0,""},
    {1,0,0,0,1,"RTI"},
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {1,0,0,0,0,"WAI"},
    {1,0,0,0,0,"SWI"},
    {1,0,0,0,0,"NEGA"},			/* 40 */
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {1,0,0,0,0,"COMA"},
    {1,0,0,0,0,"LSRA"},
    {0,0,0,0,0,""},
    {1,0,0,0,0,"RORA"},
    {1,0,0,0,0,"ASRA"},
    {1,0,0,0,0,"ASLA"},
    {1,0,0,0,0,"ROLA"},
    {1,0,0,0,0,"DECA"},
    {0,0,0,0,0,""},
    {1,0,0,0,0,"INCA"},
    {1,0,0,0,0,"TSTA"},
    {0,0,0,0,0,""},
    {1,0,0,0,0,"CLRA"},
    {1,0,0,0,0,"NEGB"},			/* 50 */
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {1,0,0,0,0,"COMB"},
    {1,0,0,0,0,"LSRB"},
    {0,0,0,0,0,""},
    {1,0,0,0,0,"RORB"},
    {1,0,0,0,0,"ASRB"},
    {1,0,0,0,0,"ASLA"},
    {1,0,0,0,0,"ROLB"},
    {1,0,0,0,0,"DECB"},
    {0,0,0,0,0,""},
    {1,0,0,0,0,"INCB"},
    {1,0,0,0,0,"TSTB"},
    {0,0,0,0,0,""},
    {1,0,0,0,0,"CLRB"},
    {2,0,0,0,0,"NEG	b,X"},		/* 60 */
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {2,0,0,0,0,"COM	b,X"},
    {2,0,0,0,0,"LSR	b,X"},
    {0,0,0,0,0,""},
    {2,0,0,0,0,"ROR	b,X"},
    {2,0,0,0,0,"ASR	b,X"},
    {2,0,0,0,0,"ASL	b,X"},
    {2,0,0,0,0,"ROL	b,X"},
    {2,0,0,0,0,"DEC	b,X"},
    {0,0,0,0,0,""},
    {2,0,0,0,0,"INC	b,X"},
    {2,0,0,0,0,"TST	b,X"},
    {2,0,0,0,1,"JMP	b,X"},
    {2,0,0,0,0,"CLR	b,X"},
    {3,0,0,1,0,"NEG	w"},		/* 70 */
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {3,0,0,1,0,"COM	w"},
    {3,0,0,1,0,"LSR	w"},
    {0,0,0,0,0,""},
    {3,0,0,1,0,"ROR	w"},
    {3,0,0,1,0,"ASR	w"},
    {3,0,0,1,0,"ASL	w"},
    {3,0,0,1,0,"ROL	w"},
    {3,0,0,1,0,"DEC	w"},
    {0,0,0,0,0,""},
    {3,0,0,1,0,"INC	w"},
    {3,0,0,1,0,"TST	w"},
    {3,1,0,0,0,"JMP	w"},
    {3,0,0,1,0,"CLR	w"},
    {2,0,0,0,0,"SUBA	b"}, 		/* 80 */
    {2,0,0,0,0,"CMPA	b"},
    {2,0,0,0,0,"SBCA	b"},
    {0,0,0,0,0,""},
    {2,0,0,0,0,"ANDA	b"},
    {2,0,0,0,0,"BITA	b"},
    {2,0,0,0,0,"LDAA	b"},
    {0,0,0,0,0,""},
    {2,0,0,0,0,"EORA	b"},
    {2,0,0,0,0,"ADCA	b"},
    {2,0,0,0,0,"ORAA	b"},
    {2,0,0,0,0,"ADDA	b"},
    {3,0,0,1,0,"CPX	w"},
    {2,0,1,0,0,"BSR	r"},
    {3,0,0,1,0,"LDS	w"},
    {0,0,0,0,0,""},
    {2,0,0,1,0,"SUBA	d"}, 		/* 90 */
    {2,0,0,1,0,"CMPA	d"},
    {2,0,0,1,0,"SBCA	d"},
    {0,0,0,0,0,""},
    {2,0,0,1,0,"ANDA	d"},
    {2,0,0,1,0,"BITA	d"},
    {2,0,0,1,0,"LDAA	d"},
    {2,0,0,1,0,"STAA	d"},
    {2,0,0,1,0,"EORA	d"},
    {2,0,0,1,0,"ADCA	d"},
    {2,0,0,1,0,"ORAA	d"},
    {2,0,0,1,0,"ADDA	d"},
    {2,0,0,1,0,"CPX	d"},
    {0,0,0,0,0,""},
    {2,0,0,1,0,"LDS	d"},
    {2,0,0,1,0,"STS	d"},
    {2,0,0,0,0,"SUBA	b,X"},		/* A0 */
    {2,0,0,0,0,"CMPA	b,X"},
    {2,0,0,0,0,"SBCA	b,X"},
    {0,0,0,0,0,""},
    {2,0,0,0,0,"ANDA	b,X"},
    {2,0,0,0,0,"BITA	b,X"},
    {2,0,0,0,0,"LDAA	b,X"},
    {2,0,0,0,0,"STAA	b,X"},
    {2,0,0,0,0,"EORA	b,X"},
    {2,0,0,0,0,"ADCA	b,X"},
    {2,0,0,0,0,"ORAA	b,X"},
    {2,0,0,0,0,"ADDA	b,X"},
    {2,0,0,0,0,"CPX	b,X"},
    {2,0,0,0,1,"JSR	b,X"},
    {2,0,0,0,0,"LDS	b,X"},
    {2,0,0,0,0,"STS	b,X"},
    {3,0,0,1,0,"SUBA	w"}, 		/* B0 */
    {3,0,0,1,0,"CMPA	w"},
    {3,0,0,1,0,"SBCA	w"},
    {0,0,0,0,0,""},
    {3,0,0,1,0,"ANDA	w"},
    {3,0,0,1,0,"BITA	w"},
    {3,0,0,1,0,"LDAA	w"},
    {3,0,0,1,0,"STAA	w"},
    {3,0,0,1,0,"EORA	w"},
    {3,0,0,1,0,"ADCA	w"},
    {3,0,0,1,0,"ORAA	w"},
    {3,0,0,1,0,"ADDA	w"},
    {3,0,0,1,0,"CPX	w"},
    {3,1,0,0,0,"JSR	w"},
    {3,0,0,1,0,"LDS	w"},
    {3,0,0,1,0,"STS	w"},
    {2,0,0,0,0,"SUBB	b"}, 		/* C0 */
    {2,0,0,0,0,"CMPB	b"},
    {2,0,0,0,0,"SBCB	b"},
    {0,0,0,0,0,""},
    {2,0,0,0,0,"ANDB	b"},
    {2,0,0,0,0,"BITB	b"},
    {2,0,0,0,0,"LDAB	b"},
    {0,0,0,0,0,""},
    {2,0,0,0,0,"EORB	b"},
    {2,0,0,0,0,"ADCB	b"},
    {2,0,0,0,0,"ORAB	b"},
    {0,0,0,0,0,"ADDB	b"},
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {3,0,0,1,0,"LDX	w"},
    {0,0,0,0,0,""},
    {2,0,0,1,0,"SUBB	d"}, 		/* D0 */
    {2,0,0,1,0,"CMPB	d"},
    {2,0,0,1,0,"SBCB	d"},
    {0,0,0,0,0,""},
    {2,0,0,1,0,"ANDB	d"},
    {2,0,0,1,0,"BITB	d"},
    {2,0,0,1,0,"LDAB	d"},
    {2,0,0,1,0,"STAB	d"},
    {2,0,0,1,0,"EORB	d"},
    {2,0,0,1,0,"ADCB	d"},
    {2,0,0,1,0,"ORAB	d"},
    {2,0,0,1,0,"ADDB	d"},
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {2,0,0,1,0,"LDX	d"},
    {2,0,0,1,0,"STX	d"},
    {2,0,0,0,0,"SUBB	b,X"},		/* E0 */
    {2,0,0,0,0,"CMPB	b,X"},
    {2,0,0,0,0,"SBCB	b,X"},
    {0,0,0,0,0,""},
    {2,0,0,0,0,"ANDB	b,X"},
    {2,0,0,0,0,"BITB	b,X"},
    {2,0,0,0,0,"LDAB	b,X"},
    {2,0,0,0,0,"STAB	b,X"},
    {2,0,0,0,0,"EORB	b,X"},
    {2,0,0,0,0,"ADCB	b,X"},
    {2,0,0,0,0,"ORAB	b,X"},
    {2,0,0,0,0,"ADDB	b,X"},
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {2,0,0,0,0,"LDX	b,X"},
    {2,0,0,0,0,"STX	b,X"},
    {3,0,0,1,0,"SUBB	w"}, 		/* F0 */
    {3,0,0,1,0,"CMPB	w"},
    {3,0,0,1,0,"SBCB	w"},
    {0,0,0,0,0,""},
    {3,0,0,1,0,"ANDB	w"},
    {3,0,0,1,0,"BITB	w"},
    {3,0,0,1,0,"LDAB	w"},
    {3,0,0,1,0,"STAB	w"},
    {3,0,0,1,0,"EORB	w"},
    {3,0,0,1,0,"ADCB	w"},
    {3,0,0,1,0,"ORAB	w"},
    {3,0,0,1,0,"ADDB	w"},
    {0,0,0,0,0,""},
    {0,0,0,0,0,""},
    {3,0,0,1,0,"LDX	w"},
    {3,0,0,1,0,"STX	w"}};

byte	PROG[IMGX];
word	PC,LIMIT, OFFSET = 0x100, symbols = 0, comments = 0;
type	TYPE[IMGX];
LINK	ptr  = 0, p, q;
STR	*Sym = 0;
STR	*Txt = 0, *p1, *q1;
FILE	*m;

word Swap(word w)
  {
    return(((w & 0xff) << 8) + (w >> 8));
  }

word Size(byte cod)
  {
    return(Table[cod].Size);
  }

word LEN(word PC)
  {
    return(Size(PROG[PC]));
  }

word PrintHex(word w, int label, int count)
  {
    int     i,j;
    word    k = 0;
    char    s[8];

    for (i = count; i; )
      {
        j = w % 16;
        w >>= 4;
        s[--i] = (char)((j < 10) ? ('0' + j) : ('A' + j - 10));
      }
    if (label)
      k++,dis_putc('L',m);
    else
      if (s[0] > '9')
        k++,dis_putc('0',m);
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

void Print(char *s, word IP)
  {
    int  Byt = FALSE;
    byte b;
    word label;
    STR	 *p;

    while (*s)
      {
	switch (*s)
	  {
	    case 'd': label = PROG[++IP];
        	      if ((Sym == 0) || ((p = bsearch(&label,Sym,symbols,sizeof(STR),compare)) == NULL))
			PrintHex(label,TRUE,4);
		      else
			dis_fputs(p->str,m);
		      IP++;
		      break;
	    case 'w': label = Swap(rd16(&PROG[++IP]));
        	      if ((Sym == 0) || ((p = bsearch(&label,Sym,symbols,sizeof(STR),compare)) == NULL))
			PrintHex(label,TRUE,4);
		      else
			dis_fputs(p->str,m);
		      IP += 2;
		      break;
	    case 'b': PrintHex(b = (byte) PROG[++IP],FALSE,2);
		      Byt = TRUE;
		      IP++;
		      break;
	    case 'r': label = OFFSET + PC + 2 + rds8(PROG[++IP]);
        	      if ((Sym == 0) || ((p = bsearch(&label,Sym,symbols,sizeof(STR),compare)) == NULL))
			PrintHex(label,TRUE,4);
		      else
			dis_fputs(p->str,m);
		      IP++;
		      break;
	    default:  dis_putc(*s,m);
	  }
	s++;
      }
    if (Byt && (b > 31) && (b < 127))
      dis_fprintf(m,"\t\t\t; '%c'",b);
    dis_putc('\n',m);
  }

void Error()
  {
    dis_fprintf(m,"??\t\t;");
    PrintHex(PROG[PC],FALSE,2);
    dis_putc('\n',m);
  }

void PrintHead()
  {
    word  label;
    STR   *p,*q;

    label = PC + OFFSET;
    if (TYPE[PC].Commen)
      {
	q = bsearch(&label,Txt,comments,sizeof(STR),compare);
	do
	  p = q--;
	while ((p != Txt) && (p->dir == q->dir));
	do
	  {
	    dis_fputs("; ",m);
	    dis_fputs(p->str,m);
	    dis_fputs("\n",m);
	    q  = p++;
	  }
	while ((p != Txt + comments) && (p->dir == q->dir));
      }
    if (TYPE[PC].Label)
      {
	if (TYPE[PC].Symbol)
	  {
	    p = bsearch(&label,Sym,symbols,sizeof(STR),compare);
	    dis_fputs(p->str,m);
	  }
	else
	  PrintHex(label,TRUE,4);
	dis_putc(':',m);
      }
    dis_putc('\t',m);
  }

void DISASMCODE(word PC1, word PC2)
  {
    byte cod;

    PC = PC1;
    while (PC <= PC2)
      {
	cod = PROG[PC];
	PrintHead();
	Print(Table[cod].s,PC);
	PC += Size(cod);
      }
  }

void PrintString(byte c)
  {
    int  ASCII = FALSE;
    byte b;

    dis_fprintf(m,"DB\t");
    do
      {
	b = PROG[PC];
	if ((b > 31) && (b < 127))
	  {
	    if (!ASCII)
	      {
		dis_putc('\'',m);
		ASCII = TRUE;
	      }
	    dis_putc(b,m);
	    if (b == '\'')
	      dis_putc('\'',m);
	  }
	else
	  {
	    if (ASCII)
	      {
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
    word i, c = 0;

    dis_fprintf(m,"\tDB\t");
    for (i = 1; i <= len; i++)
      {
	c += PrintHex(PROG[IP++],FALSE,2);
	if (i != len)
	  c++,dis_putc(',',m);
      }
    return(c);
  }

void DISASMComment()
  {
    int	 i,n;
    word len, IP, count = 0;

    IP = PC;
    do
      {
	count++;
	IP++;
      }
    while ((TYPE[IP].Data == 5) && (!TYPE[IP].Label));
    while ((len = LEN(PC)) <= count)
      {
	n = PrintDB(PC,len);
	for (i = 0; i <= ((23 - n) >> 3); i++)
	  dis_putc('\t',m);
	dis_putc(';',m);
	PrintHex(PC + OFFSET,FALSE,4);
	dis_putc('\t',m);
	DISASMCODE(PC,PC+len-1);
	count -= len;
      }
    if (count)
      {
	PrintDB(PC,count);
	dis_putc('\n',m);
	PC += count;
      }
  }

void DISASMDATA(word PC1,word LIM)
  {
    byte cod;
    word count;

    PC = PC1;
    while (PC <= LIM)
      {
	PrintHead();
	switch (TYPE[PC].Data)
	  {
	    case 0: dis_fprintf(m,"DB\t");
		    PrintHex(cod = PROG[PC],FALSE,2);
		    if (!TYPE[PC].Label)
		      {
			dis_fprintf(m,"\t\t\t;");
			PrintHex(PC + OFFSET,FALSE,4);
			if ((cod > 31) && (cod < 127))
			  dis_fprintf(m," '%c'",cod);
		      }
		    dis_putc('\n',m);
		    PC++;
		    break;
	    case 1: dis_fprintf(m,"DW\t");
		    PrintHex(rd16(&PROG[PC]),FALSE,4);
		    if (!TYPE[PC].Label)
		      {
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
	    case 4: dis_fprintf(m,"DS\t");
		    for (count = 1, PC++; ((TYPE[PC].Data == 4) && (!TYPE[PC].Label)); count++, PC++) ;
		    PrintHex(count,FALSE,4);
		    dis_putc('\n',m);
		    break;
	    case 5: DISASMComment();
	  }
      }
  }

int JMP(word PC)
  {
    byte  cod;

    cod = PROG[PC];
    return(Table[cod].JPa || Table[cod].JPr);
  }

int Stop(word PC)
  {
    return(Table[PROG[PC]].Stop);
  }

int DAT(word PC)
  {
    return(Table[PROG[PC]].Data);
    if (JMP(PC) || Stop(PC))
      return(FALSE);
    else
      return(strchr(Table[PROG[PC]].s,'w') != NULL);
  }

word CTarget(word PC)
  {
    if (Table[PROG[PC]].JPa)
      return(Swap(rd16(&PROG[PC+1])) - OFFSET);
    else	/*	Table[cod].JPr	*/
      return(PC + 2 + rds8(PROG[PC+1]));
  }

word DTarget(word PC)
  {
    if (strchr(Table[PROG[PC]].s,'w') != NULL)
      return(Swap(rd16(&PROG[PC+1])) - OFFSET);
    else
      return(PROG[PC+1] - OFFSET);
  }

void SetToCode(word PC,word LEN)
  {
    word  n;

    n = PC + LEN;
    while (PC < n)
      TYPE[PC++].Code = 1;
  }

static void TravelCompat(word PC)
{
	word len;

	while ((PC <= LIMIT) && !TYPE[PC].Code && !TYPE[PC].Break) {
	  len = LEN(PC);
	  if (!len)
	    return;
	  SetToCode(PC, len);
	  if (JMP(PC))
	    TravelCompat(CTarget(PC));
	  if (Stop(PC))
	    return;
	  PC = (word)(PC + len);
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
	        if (!Stop(PC)) {
	          word nxt = (word)(PC + len);
	          if (nxt < LIMIT && sp < 65536)
	            stack[sp++] = nxt;
	        }
	        if (sp < 65536)
	          stack[sp++] = tgt;
	        break;
	      }
	    }
	    if (Stop(PC))
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


void insert()
  {
    if (ptr)
      {
	p->link = (LINK) xmalloc(sizeof(*ptr));
	p = p->link;
      }
    else
      {
	ptr = (LINK) xmalloc(sizeof(*ptr));
	p = ptr;
      }
    p->link = 0;
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
    word i;
    FILE *f;
    char c;
    word w1, w2;
    char n[512];

    strcpy(n,s);
    if (strchr(n,'.') == NULL)
      strcat(n,".PAR");
    if ((f = fopen(n,"rb")) == NULL)
      {
	printf("\n\aError al abrir el fichero %s\n",n);
	exit(3);
      }
    while (fscanf(f,"%c",&c) != EOF)
      switch (toupper(c))
        {
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
		    while (w1 <= w2)
		      {
			TYPE[w1].Code = 0;
			TYPE[w1++].Data = 5;
		      }
		    break;
          case 'D': pull_hex2(f, &w1, &w2);
		    w1 -= OFFSET;
		    w2 -= OFFSET;
		    while (w1 <= w2)
		      {
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
		    switch (c)
		      {
			case '0': TYPE[w1].Data = 2;
				  break;
			case '$': TYPE[w1].Data = 3;
		      }
		    break;
          case 'W': pull_hex2(f, &w1, &w2);
		    w1 -= OFFSET;
		    w2 -= OFFSET;
		    while (w1 <= w2)
		      {
			TYPE[w1].Code = 0;
			TYPE[w1++].Data = 1;
			TYPE[w1++].Code = 0;
		      }
		    break;
          case 'H': { unsigned _u; if (fscanf(f, "%x %511[^\n\r]", &_u, n) == 2) w1 = (word)_u; }
		    insert();
		    ++comments;
		    p->dir = w1;
		    p->str = (char *) xmalloc(strlen(n) + 1);
		    strcpy(p->str,n);
		    TYPE[w1 - OFFSET].Commen = 1;
	}
    fclose(f);
    if (comments)
      {
	Txt = (STR *) xmalloc(comments * sizeof(STR));
	for (i = 0; i < comments; i++)
	  {
	    p = ptr;
	    Txt[i].dir = p->dir;
	    Txt[i].str = p->str;
	    ptr = p->link;
	    free(p);
          }
      qsort(Txt,comments,sizeof(STR),compare);
    }
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
    word PC = 0;

    while (PC < LIMIT)
      if (!TYPE[PC].Code)
        PC++;
      else
	{
	  if (JMP(PC))
	    {
	      label_if(CTarget(PC));
	    }
	  else
	    if (DAT(PC))
	      label_if(DTarget(PC));
	  PC += LEN(PC);
	}
  }

void LoadSymbols(char *s)
  {
    word dir,i;
	unsigned dir_u;
    char sym[7];
    char n[512];
    FILE *f;

    strcpy(n,s);
    if (strchr(n,'.') == NULL)
      strcat(n,".SYM");
    if ((f = fopen(n,"rb")) == NULL)
      {
        printf("\a\nError al abrir el fichero %s\n",n);
        exit(4);
      }
    while (fscanf(f,"%x %6s",&dir_u,sym) == 2 && ((dir = (word)dir_u), 1))
      {
        insert();
        ++symbols;
        p->dir = dir;
        p->str = (char *) xmalloc(7);
        strcpy(p->str,sym);
        TYPE[dir - OFFSET].Symbol = 1;
      }
    fclose(f);
    Sym = (STR *) xmalloc(symbols * sizeof(STR));
    for (i = 0; i < symbols; i++)
      {
        p = ptr;
        Sym[i].dir = p->dir;
        Sym[i].str = p->str;
        ptr = p->link;
        free(p);
      }
    qsort(Sym,symbols,sizeof(STR),compare);
    ptr = 0;
  }

void ListProgram(char *s)
  {
    word PC = 0,PC1;
    char n[512];

    strcpy(n,s);
    if (strchr(n,'.') == NULL)
      strcat(n,".MAC");
    if ((m = fopen(n,"wb")) == NULL)
      {
	printf("\a\nError al crear el fichero %s\n",n);
	exit(5);
      }
    dis_fprintf(m,"\tORG\t");
    PrintHex(OFFSET,FALSE,4);
    dis_fprintf(m,"\n\n");
    while (PC < LIMIT)
      {
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
	/* BRA was stored as an absolute jump that falls through, JMP extended
	   did not stop the walk, and JSR indexed was marked as a stop.
	   Compat leaves the 1990 table alone. */
	if (!dis_compat) {
	  Table[0x20].JPa = 0;
	  Table[0x20].JPr = 1;
	  Table[0x20].Stop = 1;
	  Table[0x7E].Stop = 1;
	  Table[0xAD].Stop = 0;
	}
	printf("\n6800 DISASSEMBLER Versión 1.0 24-12-90\n");
	printf("   Universidad Central de Las Villas\n");
	printf("       Instituto de Informática\n");
	printf("      Lic. David Izada Rodríguez\n\n");
	if ((argc < 4) || (argc > 5)) {
	  printf("\aUso: DIS6800 [--compat] [--crlf] <.COM> <.PAR> <.MAC> [<.SYM>]\n");
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
