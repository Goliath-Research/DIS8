#include "dis_port.h"

#include <stdlib.h>
#include <string.h>

int dis_compat = 0;
int dis_crlf = 0;
word dis_loaded = 0;

word rd16(const byte *p)
{
	return (word)(p[0] | ((word)p[1] << 8));
}

int rds8(byte b)
{
	return (b & 0x80) ? (int)b - 256 : (int)b;
}

void *xmalloc(size_t n)
{
	void *p = malloc(n ? n : 1);
	if (p == NULL) {
		printf("\aError de memoria\n");
		exit(1);
	}
	return p;
}

int dis_putc(int c, FILE *fp)
{
	if (c == '\n' && dis_crlf) {
		if (fputc('\r', fp) == EOF)
			return EOF;
	}
	return fputc(c, fp);
}

int dis_fputs(const char *s, FILE *fp)
{
	while (*s) {
		if (dis_putc((unsigned char)*s++, fp) == EOF)
			return EOF;
	}
	return 0;
}

int dis_fprintf(FILE *fp, const char *fmt, ...)
{
	char stack_buf[1024];
	char *buf = stack_buf;
	va_list ap, aq;
	int n;

	va_start(ap, fmt);
	va_copy(aq, ap);
	n = vsnprintf(stack_buf, sizeof stack_buf, fmt, ap);
	va_end(ap);
	if (n < 0) {
		va_end(aq);
		return n;
	}
	if (n >= (int)sizeof stack_buf) {
		buf = (char *)xmalloc((size_t)n + 1);
		vsnprintf(buf, (size_t)n + 1, fmt, aq);
	}
	va_end(aq);
	dis_fputs(buf, fp);
	if (buf != stack_buf)
		free(buf);
	return n;
}

void dis_parse_flags(int *argc, char **argv)
{
	int i, w = 1;

	for (i = 1; i < *argc; i++) {
		if (strcmp(argv[i], "--compat") == 0)
			dis_compat = 1;
		else if (strcmp(argv[i], "--crlf") == 0)
			dis_crlf = 1;
		else if (strcmp(argv[i], "--") == 0) {
			while (++i < *argc)
				argv[w++] = argv[i];
			break;
		} else
			argv[w++] = argv[i];
	}
	*argc = w;
	argv[w] = NULL;
}
