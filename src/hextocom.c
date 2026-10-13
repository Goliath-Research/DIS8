/*
 * Portable port of HEXTOCOM.C (David Izada Rodriguez).
 * Concatenates Intel HEX data records in order and ignores addresses,
 * which is what the 1988 program did. Lower case hex is not accepted.
 *
 *   HEX2COM [--crlf] Source[.HEX] [Target[.COM]]
 */
#include "dis_port.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int chr_hex(int x)
{
	return (x < ':') ? x - '0' : x - 'A' + 0x0A;
}

static byte hex_to_num(const char *hex)
{
	return (byte)(0x10 * chr_hex((unsigned char)*hex) + chr_hex((unsigned char)*(hex + 1)));
}

static char *load_source(const char *s)
{
	FILE *fp;
	long len;
	char *buf;
	size_t n;

	fp = fopen(s, "rb");
	if (fp == NULL) {
		printf("\aError al abrir el fichero %s\n", s);
		exit(2);
	}
	if (fseek(fp, 0, SEEK_END) != 0) {
		printf("\aError al abrir el fichero %s\n", s);
		fclose(fp);
		exit(2);
	}
	len = ftell(fp);
	if (len < 0) {
		printf("\aError al abrir el fichero %s\n", s);
		fclose(fp);
		exit(2);
	}
	if (fseek(fp, 0, SEEK_SET) != 0) {
		printf("\aError al abrir el fichero %s\n", s);
		fclose(fp);
		exit(2);
	}
	buf = (char *)xmalloc((size_t)len + 1);
	n = fread(buf, 1, (size_t)len, fp);
	if (ferror(fp)) {
		printf("\aError al abrir el fichero %s\n", s);
		fclose(fp);
		exit(2);
	}
	fclose(fp);
	buf[n] = '\0';
	return buf;
}

int main(int argc, char *argv[])
{
	char source[512];
	char target[512];
	char *pos;
	char *buffer;
	char *access;
	FILE *out;
	byte num;
	unsigned i;

	dis_parse_flags(&argc, argv);
	if (argc < 2 || argc > 3) {
		printf("\aError al utilizar el comando\n");
		printf("\nUso:\tHEX2COM [--crlf] Source[.HEX] [Target[.COM]]\n");
		return 1;
	}
	if (strlen(argv[1]) > sizeof source - 5) {
		printf("\aError al abrir el fichero %s\n", argv[1]);
		return 2;
	}
	strcpy(source, argv[1]);
	pos = strchr(source, '.');
	if (pos == NULL)
		strcat(source, ".HEX");
	if (argc > 2) {
		if (strlen(argv[2]) > sizeof target - 5) {
			printf("\aError en escritura\n");
			return 3;
		}
		strcpy(target, argv[2]);
		pos = strchr(target, '.');
		if (pos == NULL)
			strcat(target, ".COM");
	} else {
		strcpy(target, source);
		pos = strchr(target, '.');
		if (pos == NULL)
			strcat(target, ".COM");
		else
			strcpy(pos, ".COM");
	}
	buffer = load_source(source);
	out = fopen(target, "wb");
	if (out == NULL) {
		printf("\aError en escritura\n");
		free(buffer);
		return 3;
	}
	printf("\n Convirtiendo %s -> %s \n", source, target);
	access = buffer;
	while (access != NULL) {
		++access;
		if (access[0] == '\0' || access[1] == '\0')
			break;
		num = hex_to_num(access);
		if (num != 0)
			access += 8;
		for (i = 1; i <= num; ++i, access += 2) {
			byte x;
			if (access[0] == '\0' || access[1] == '\0')
				break;
			x = hex_to_num(access);
			if (fwrite(&x, 1, 1, out) != 1) {
				printf("\aError en escritura\n");
				fclose(out);
				free(buffer);
				return 3;
			}
		}
		access = strchr(access, ':');
	}
	if (fclose(out) == EOF) {
		printf("\aError en escritura\n");
		free(buffer);
		return 3;
	}
	free(buffer);
	return 0;
}
