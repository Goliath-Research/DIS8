#include        <string.h>
#include        <memory.h>
#include        <fcntl.h>
#include        <io.h>
#include	<sys\types.h>
#include	<sys\stat.h>
#include        <stdlib.h>
#include        <stdio.h>
#include	<malloc.h>

#define ChrHex(X)	((X < ':') ? X - '0' : X - 'A' + 0x0A)

typedef	unsigned char	byte;
char	*Buffer;

void LoadSource(char *s)
  {
	int		Handle;
	unsigned	BufSize;

	if ((Handle = open(s,O_BINARY,S_IREAD)) == -1) {
	  printf("\aError al abrir el fichero %s\n",s);
	  exit(2);
	}
	Buffer  = (char *) malloc(filelength(Handle));
	BufSize = read(Handle,Buffer,0xff00);
	Buffer[BufSize] = '\0';
	close(Handle);
  }

byte HexToNum(char *Hex)
  {
     return (0x10 * ChrHex(*Hex) + ChrHex(*(Hex + 1)));
  }

main(int argc,char *argv[])
  {
	int	Targ;
	char    Source[80];     /* Nombre de fichero fuente */
	char    Target[80];     /* Nombre de fichero de destino */
	char    *Pos,*Access;
	byte	Num,i;

	if (argc == 1)
	  {
	       printf("\aError al utilizar el comando\n");
	       printf("\n\Uso:\tHEX2COM Source[.HEX] [Target[.COM]]\n");
	       exit(1);
	  }
	  else strcpy(Source,argv[1]);
	Pos = strchr(Source,'.');
	if (Pos == NULL)
	   strcat(Source,".HEX");
	if (argc > 2)
	     {
	       strcpy(Target,argv[2]);
	       Pos = strchr(Target,'.');
	       if (Pos == NULL)
	         strcat(Target,".COM");

	     }
	  else 
	    {
	       strcpy(Target,Source);
	       Pos = strchr(Target,'.');
	       if (Pos == NULL)
	         strcat(Target,".COM");
	       else strcpy(Pos,".COM");
	    }
	LoadSource(Source);
	Targ = creat(Target,S_IWRITE);
	if (Targ == -1)
	  {
	     printf("\aError en escritura\n");
	     exit(3);
	  }
	printf("\n Convirtiendo %s -> %s \n",Source,Target);
	Access = Buffer;
	while (Access != NULL)
	  {
	    ++Access;
	    Num = HexToNum(Access);
	    if (Num != 0)
	       Access += 8;
	    for (i = 1; i <= Num; ++i,Access +=2)
	      {
		byte  x;

		x = HexToNum(Access);
		write(Targ,(char *) &x,1);
	      }
	    Access = strchr(Access,':');
	  }
	close(Targ);
	free(Buffer);
  }
