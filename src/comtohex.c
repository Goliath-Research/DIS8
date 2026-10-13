/*
 * Portable port of COMTOHEX.C (David Izada Rodriguez).
 * Intel HEX writer. The end record is the original non-standard
 * ":0000000000" (type 00), not the usual type 01 EOF.
 *
 *   COMtoHEX [--crlf] <.COM> <.HEX> [decimal address]
 * The address is decimal, as in the original atoi() call.
 * It is optional; the 1990 program read argv[3] even when it was absent.
 */
#include "dis_port.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Nbuf 0x8000

static byte prog[Nbuf];
static word offset_;
static word limit_;
static FILE *outf;

static void print_hex(unsigned w, int count)
{
	char s[8];
	int i;

	if (count <= 0 || count > 8)
		return;
	for (i = count; i;) {
		int j = (int)(w % 16u);
		w >>= 4;
		s[--i] = (char)((j < 10) ? ('0' + j) : ('A' + j - 10));
	}
	for (i = 0; i < count; i++)
		dis_putc(s[i], outf);
}

static void load_program(const char *s)
{
	char str[512];
	FILE *fp;
	size_t n;

	if (strlen(s) > sizeof str - 5) {
		printf("\aError al abrir el fichero %s\n", s);
		exit(2);
	}
	strcpy(str, s);
	if (strchr(str, '.') == NULL)
		strcat(str, ".COM");
	fp = fopen(str, "rb");
	if (fp == NULL) {
		printf("\aError al abrir el fichero %s\n", str);
		exit(2);
	}
	n = fread(prog, 1, Nbuf, fp);
	if (ferror(fp)) {
		printf("\aError al abrir el fichero %s\n", str);
		fclose(fp);
		exit(2);
	}
	fclose(fp);
	limit_ = (word)n;
}

static void write_hex(const char *s)
{
	char str[512];
	word n;
	word l;
	word i;
	byte sum;

	if (strlen(s) > sizeof str - 5) {
		printf("\a\nError al crear el fichero %s\n", s);
		exit(3);
	}
	strcpy(str, s);
	if (strchr(str, '.') == NULL)
		strcat(str, ".HEX");
	outf = fopen(str, "wb");
	if (outf == NULL) {
		printf("\a\nError al crear el fichero %s\n", str);
		exit(3);
	}
	for (n = 0; n < limit_; n = (word)(n + l)) {
		l = (word)((limit_ - n < 16) ? (limit_ - n) : 16);
		dis_putc(':', outf);
		print_hex(l, 2);
		print_hex((unsigned)offset_ + n, 4);
		dis_fputs("00", outf);
		sum = (byte)(l + (byte)(offset_ + n) + (byte)((offset_ + n) >> 8));
		for (i = 0; i < l; i++) {
			sum = (byte)(sum + prog[n + i]);
			print_hex(prog[n + i], 2);
		}
		print_hex((unsigned)(byte)(0 - sum), 2);
		dis_putc('\n', outf);
	}
	dis_fputs(":0000000000\n", outf);
	fclose(outf);
}

int main(int argc, char *argv[])
{
	dis_parse_flags(&argc, argv);
	if (argc < 3 || argc > 4) {
		printf("\aCOMtoHEX [--crlf] <.COM> <.HEX> [Address]\n");
		return 1;
	}
	offset_ = (argc == 4) ? (word)atoi(argv[3]) : 0;
	load_program(argv[1]);
	write_hex(argv[2]);
	return 0;
}
