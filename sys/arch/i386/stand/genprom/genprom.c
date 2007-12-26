/*	$NetBSD: genprom.c,v 1.6.44.1 2007/12/26 19:42:26 ad Exp $	 */

/*
 * mainly from netbsd:sys/arch/i386/netboot/genprom.c
 */

/*
 * Read a binary image of a bios extension, generate the
 * appropriate block count and checksum and write them
 * into the rom image (replacing 2nd and 5th bytes)
 * The binary image should be sized before being filtered
 * through this routine.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PROM_SIZE 0x10000	/* max */

static char *progname;

static void bail(const char *);
int main(int, char **);

static void
bail(const char *msg)
{
	(void)fprintf(stderr, "%s: %s\n", progname, msg);
	exit(1);
}

int
main(int argc, char **argv)
{
	char            w[PROM_SIZE];
	int             i, sum;
	int             romsize;
	unsigned short  ck16;

	if ((progname = strrchr(argv[0], '/')) != NULL)
		progname++;
	else
		progname = argv[0];

	if (argc > 1) {
		if (sscanf(argv[1], "%d", &romsize) != 1)
			bail("bad arg");
	} else {
		bail("arg: romsize / bytes");
	}

	(void)memset(w, 0x0, PROM_SIZE);
	i = fread(w, 1, PROM_SIZE, stdin);

	(void)fprintf(stderr, "bios extension size: %d (0x%x), read %d bytes\n",
	    romsize, romsize, i);

	if (i > romsize)
		bail("read longer than expected");

	w[2] = romsize / 512;	/* BIOS extension size */

	i = w[0x18] + (w[0x19] << 8);	/* if this is a PCI ROM, this is the
					 * offset to the "PCI data structure" */
	if ((i < romsize - 0x18) && (w[i] == 'P') && (w[i + 1] == 'C')
	    && (w[i + 2] == 'I') && (w[i + 3] == 'R')) {
		fprintf(stderr, "PCI Data Structure found\n");
		w[i + 0x10] = romsize / 512;	/* BIOS extension size again */
	}
	for (sum = 0, i = 0; i < romsize; i++)
		sum += w[i];

	w[5] = -sum;		/* left free for checksum adjustment */

	/* calculate CRC as used by PROM programmers */
	for (ck16 = 0, i = 0; i < romsize; i++)
		ck16 += (unsigned char) w[i];
	(void)fprintf(stderr, "ROM CRC: 0x%04x\n", ck16);

	(void)fwrite(w, 1, romsize, stdout);
	return (0);
}
