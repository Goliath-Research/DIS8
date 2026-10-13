#ifndef DIS_PORT_H
#define DIS_PORT_H

/*
 * Shared helpers for the portable DIS8 tools.
 * Line endings: the DOS originals wrote text mode, so '\n' became CR+LF.
 * Pass --crlf to reproduce that. The default is LF.
 * Pass --compat to reproduce the original control-flow walk
 * (see docs/known-issues.md).
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

typedef uint8_t byte;
typedef uint16_t word;

#define TRUE 1
#define FALSE 0

/* Historical PROG/TYPE bound was 0x8000, but the DOS read asked for
   0xFF00 bytes. The port keeps a full 64 KiB image plus a guard so a
   length that runs off the last opcode cannot write past the array. */
#define Nmax 0x8000
#define IMGX (0x10000u + 32u)

extern int dis_compat;
extern int dis_crlf;
extern word dis_loaded;

word rd16(const byte *p);
int rds8(byte b);
void *xmalloc(size_t n);

int dis_putc(int c, FILE *fp);
int dis_fputs(const char *s, FILE *fp);
int dis_fprintf(FILE *fp, const char *fmt, ...);
void dis_parse_flags(int *argc, char **argv);

#endif
