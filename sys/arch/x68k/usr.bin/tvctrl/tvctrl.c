/*	$NetBSD: tvctrl.c,v 1.4 1999/01/13 10:23:40 itohy Exp $	*/

#include <sys/types.h>
#include <sys/ioctl.h>
#include <machine/iteioctl.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

int main __P((int, char *[]));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	unsigned long num;
	unsigned char ctl;
	char *ep;

	if (argc < 2) {
		fprintf(stderr, "usage: %s control_number [...]\n", argv[0]);
		return 1;
	}
	while (argv++, --argc != 0) {
		num = strtoul(argv[0], &ep, 10);
		if (num > 255 || *ep != '\0')
			errx(1, "illegal number -- %s", argv[0]);

		ctl = num;
		if (ioctl(0, ITETVCTRL, &ctl))
			err(1, "ioctl(ITETVCTRL)");
	}

	return 0;
}
