#include        <string.h>
#include        <fcntl.h>
#include        <io.h>
#include        <sys\types.h>
#include        <sys\stat.h>
#include        <stdlib.h>
#include        <stdio.h>
#include	"Goliat1.h"

#define Nmax    0x8000

byte    PROG[Nmax];
word    OFFSET,LIMIT;
FILE	*f;

void PrintHex(word w, int count)
  {
    int     i,j;
    word    k = 0;
    char    s[3];

    for (i = count; i; )
      {
	j = w % 16;
	w >>= 4;
	s[--i] = (j < 10) ? ('0' + j) : ('A' + j - 10);
      }
    for (i = 0; i < count; i++)
      putc(s[i],f);
}

void LoadProgram(char *s)
  {
    int  h;
    char str[80];

    strcpy(str,s);
    if (strchr(str,'.') == NULL)
      strcat(str,".COM");
    if ((h = open(str,O_BINARY,S_IREAD)) == -1)
      {
	printf("\aError al abrir el fichero %s\n",str);
	exit(2);
      }
    LIMIT = read(h,(char *) PROG,Nmax);
    close(h);
  }

void WriteHex(char *s)
  {
    char  str[80];
    word  n,i,l;
    byte  sum;

    strcpy(str,s);
    if (strchr(str,'.') == NULL)
      strcat(str,".HEX");
    if ((f = fopen(str,"w")) == NULL)
      {
	printf("\a\nError al crear el fichero %s\n",str);
	exit(3);
      }
    for (n = 0; n < LIMIT; n += l)
      {
	sum = 0;
	l = (LIMIT - n < 16) ? (LIMIT - n) : 16;
	fputc(':',f);
	PrintHex(l,2);
	PrintHex(OFFSET + n,4);
	fprintf(f,"00");
	sum = l + (byte)(OFFSET + n) + (byte)((OFFSET + n) >> 8);
	for (i = 0; i < l; i++)
	  {
	    sum += PROG[n + i];
	    PrintHex(PROG[n + i],2);
	  }
	PrintHex(-sum,2);
	fprintf(f,"\n");
      }
    fprintf(f,":0000000000\n");
    fclose(f);
  }

main(int argc,char *argv[])
  {
    if ((argc < 2) || (argc > 4))
      {
	printf("\aCOMtoHEX <.COM> <.HEX> [Address]\n");
	exit(1);
      }
    OFFSET = atoi(argv[3]);
    LoadProgram(argv[1]);
    WriteHex(argv[2]);
    exit(0);
  }
