/*
 * Portable reading of PLUS.C, a fragment of the PLUS-80 loader
 * (David Izada Rodriguez). The original poked a Z80 image through an
 * 8255 at port 0xF6 and called external far routines Prog1..Prog4.
 *
 * Nothing here touches hardware. `plus` prints the reset sequence and
 * the 24-byte Prog1 image taken from original/P1.MAC, which is the
 * block ResetPLUS80() copied. The first byte is C3 (JP), not C9; the
 * original returned whether the Z80 had stored C9 over that JP after
 * it left HALT. A dry run cannot observe that handshake.
 *
 * SegOk() in the 1990 source writes the logical-not of a byte and
 * treats the segment as present when the value does not change, and
 * the address expression `seg << 16 + 0xFE00` is parsed as
 * `seg << (16 + 0xFE00)`. Those details are described in
 * docs/known-issues.md. This program uses the address that the
 * expression was aiming at, (segment << 16) + 0xFE00.
 */
#include "dis_port.h"

#include <stdio.h>
#include <string.h>

#define RC8255 0xF6

/* First 24 bytes of original/P1.MAC (ORG FE00h). */
static const byte prog1[24] = {
	0xC3, 0x03, 0xFE, 0x3E, 0x20, 0xD3, 0x00, 0x31,
	0x00, 0x00, 0xED, 0x56, 0x3E, 0xC9, 0x32, 0x38,
	0x00, 0xFB, 0x76, 0x32, 0x00, 0xFE, 0xF3, 0x76
};

static byte window[256];
static byte flag = 0x0D;

/* MSVC reserves outp as a port-I/O intrinsic, so this dry-run logger
   cannot use that name. The printed line still says outp. */
static void write_port(unsigned port, unsigned value)
{
	printf("outp(%02X) <- %02X\n", port, value & 0xFFu);
}

static void stop_z80(void)
{
	write_port(RC8255, 0xB4);
	write_port(RC8255, 0x0F);
	write_port(RC8255, flag);
}

static void continue_z80(void)
{
	write_port(RC8255, 0x0E);
	printf("delay 100 (host timer omitted)\n");
}

static void send_program(const byte *prog, word count)
{
	word i;

	stop_z80();
	for (i = 0; i < count && i < (word)sizeof window; i++)
		window[i] = prog[i];
	printf("copied %u bytes at FE00\n", (unsigned)count);
	continue_z80();
}

/* True only after the Z80 has overwritten FE00 with C9 (RET). */
static int reset_plus80(void)
{
	send_program(prog1, 24);
	return window[0] == 0xC9;
}

int main(int argc, char *argv[])
{
	int i;
	int ready;

	dis_parse_flags(&argc, argv);
	(void)argc;
	(void)argv;
	printf("PLUS-80 loader fragment (dry run, no hardware)\n");
	printf("Prog1 is the 24-byte image from P1.MAC\n");
	ready = reset_plus80();
	printf("image:");
	for (i = 0; i < 24; i++)
		printf(" %02X", window[i]);
	printf("\n");
	printf("FE00 is %02X; ResetPLUS80 returns %s until the Z80 stores C9\n",
	       window[0], ready ? "true" : "false");
	return (memcmp(window, prog1, 24) == 0) ? 0 : 1;
}
