/*	$NetBSD: palette.c,v 1.4 2003/07/15 01:44:54 lukem Exp $	*/
/*
 * pelette - manipulate text colormap for NetBSD/x68k.
 * author: Masaru Oki
 *
 * This software is in the Public Domain.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: palette.c,v 1.4 2003/07/15 01:44:54 lukem Exp $");

#include <stdio.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/fcntl.h>

#define PALETTE_OFFSET 0x2000 /* physical addr: 0xe82000 */
#define PALETTE_SIZE   0x1000 /* at least 1 page */

int
main(int argc, char *argv[])
{
	int fd;
	u_short *palette;
	char *mapaddr;
	int r, g, b;
	int c = 7;

#ifdef DEBUG
{
	int i;
	printf("argc = %d\n", argc);
	for (i = 0; i < argc; i++)
		printf("argv[%d] = \"%s\"\n", i, argv[i]);
}
#endif

	if ((fd = open("/dev/grf0", O_RDWR, 0)) < 0) {
		perror("open");
		exit(1);
	}

	mapaddr = mmap(0, PALETTE_SIZE, PROT_READ | PROT_WRITE,
		       MAP_FILE | MAP_SHARED, fd, PALETTE_OFFSET);
	if (mapaddr == (caddr_t)-1) {
		perror("mmap");
		close(fd);
		exit(1);
	}
	close(fd);
	palette = (u_short *)(mapaddr + 0x0200);

	if (argc == 5) {
		c = atoi(argv[--argc]);
		if (c > 15) {
			printf("Usage: %s [red green blue [code]]\n", argv[0]);
			exit(1);
		}
	}
	if (argc != 4)
		r = g = b = 31;
	else {
		r = atoi(argv[1]);
		g = atoi(argv[2]);
		b = atoi(argv[3]);
		if (r > 31 || g > 31 || b > 31) {
			printf("Usage: %s [red green blue [code]]\n", argv[0]);
			r = g = b = 31;
		}
	}
#ifdef DEBUG
	printf("color table offset = %d\n", c);
	printf("r = %d, g = %d, b = %d\n", r, g, b);
#endif
	r <<= 6;
	g <<= 11;
	b <<= 1;

	palette[c] = r | g | b | 1;

	exit(0);
}
