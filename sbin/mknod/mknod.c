/*	$NetBSD: mknod.c,v 1.12 1998/01/17 13:04:18 mycroft Exp $	*/

/*
 * Copyright (c) 1998 Charles M. Hannum.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1998 Charles M. Hannum.  All rights reserved.\n");
__RCSID("$NetBSD: mknod.c,v 1.12 1998/01/17 13:04:18 mycroft Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main __P((int, char *[]));
static void usage __P((void));
typedef	dev_t pack_t __P((u_long, u_long, u_long *, u_long *));


pack_t pack_native;

dev_t
pack_native(maj, min, maj2, min2)
	u_long maj, min, *maj2, *min2;
{
	dev_t dev;

	dev = makedev(maj, min);
	*maj2 = major(dev);
	*min2 = minor(dev);
	return (dev);
}


#define	major_netbsd(x)		((int32_t)((((x) & 0x000fff00) >>  8)))
#define	minor_netbsd(x)		((int32_t)((((x) & 0xfff00000) >> 12) | \
					   (((x) & 0x000000ff) >>  0)))
#define	makedev_netbsd(x,y)	((dev_t)((((x) <<  8) & 0x000fff00) | \
					 (((y) << 12) & 0xfff00000) | \
					 (((y) <<  0) & 0x000000ff)))

pack_t pack_netbsd;

dev_t
pack_netbsd(maj, min, maj2, min2)
	u_long maj, min, *maj2, *min2;
{
	dev_t dev;

	dev = makedev_netbsd(maj, min);
	*maj2 = major_netbsd(dev);
	*min2 = minor_netbsd(dev);
	return (dev);
}


#define	major_freebsd(x)	((int32_t)(((x) & 0x0000ff00) >> 8))
#define	minor_freebsd(x)	((int32_t)(((x) & 0xffff00ff) >> 0))
#define	makedev_freebsd(x,y)	((dev_t)((((x) << 8) & 0x0000ff00) | \
					 (((y) << 0) & 0xffff00ff)))

pack_t pack_freebsd;

dev_t
pack_freebsd(maj, min, maj2, min2)
	u_long maj, min, *maj2, *min2;
{
	dev_t dev;

	dev = makedev_freebsd(maj, min);
	*maj2 = major_freebsd(dev);
	*min2 = minor_freebsd(dev);
	return (dev);
}


#define	major_8_8(x)		((int32_t)(((x) & 0x0000ff00) >> 8))
#define	minor_8_8(x)		((int32_t)(((x) & 0x000000ff) >> 0))
#define	makedev_8_8(x,y)	((dev_t)((((x) << 8) & 0x0000ff00) | \
					 (((y) << 0) & 0x000000ff)))

pack_t pack_8_8;

dev_t
pack_8_8(maj, min, maj2, min2)
	u_long maj, min, *maj2, *min2;
{
	dev_t dev;

	dev = makedev_8_8(maj, min);
	*maj2 = major_8_8(dev);
	*min2 = minor_8_8(dev);
	return (dev);
}


#define	major_12_20(x)		((int32_t)(((x) & 0xfff00000) >> 20))
#define	minor_12_20(x)		((int32_t)(((x) & 0x000fffff) >>  0))
#define	makedev_12_20(x,y)	((dev_t)((((x) << 20) & 0xfff00000) | \
					 (((y) <<  0) & 0x000fffff)))

pack_t pack_12_20;

dev_t
pack_12_20(maj, min, maj2, min2)
	u_long maj, min, *maj2, *min2;
{
	dev_t dev;

	dev = makedev_12_20(maj, min);
	*maj2 = major_12_20(dev);
	*min2 = minor_12_20(dev);
	return (dev);
}


#define	major_14_18(x)		((int32_t)(((x) & 0xfffc0000) >> 18))
#define	minor_14_18(x)		((int32_t)(((x) & 0x0003ffff) >>  0))
#define	makedev_14_18(x,y)	((dev_t)((((x) << 18) & 0xfffc0000) | \
					 (((y) <<  0) & 0x0003ffff)))

pack_t pack_14_18;

dev_t
pack_14_18(maj, min, maj2, min2)
	u_long maj, min, *maj2, *min2;
{
	dev_t dev;

	dev = makedev_14_18(maj, min);
	*maj2 = major_14_18(dev);
	*min2 = minor_14_18(dev);
	return (dev);
}


#define	major_8_24(x)		((int32_t)(((x) & 0xff000000) >> 24))
#define	minor_8_24(x)		((int32_t)(((x) & 0x00ffffff) >>  0))
#define	makedev_8_24(x,y)	((dev_t)((((x) << 24) & 0xff000000) | \
					 (((y) <<  0) & 0x00ffffff)))

pack_t pack_8_24;

dev_t
pack_8_24(maj, min, maj2, min2)
	u_long maj, min, *maj2, *min2;
{
	dev_t dev;

	dev = makedev_8_24(maj, min);
	*maj2 = major_8_24(dev);
	*min2 = minor_8_24(dev);
	return (dev);
}


struct format {
	char	*name;
	pack_t	*pack;
} formats[] = {
	{"386bsd",  pack_8_8},
	{"4bsd",    pack_8_8},
	{"bsdos",   pack_12_20},
	{"freebsd", pack_freebsd},
	{"hpux",    pack_8_24},
	{"linux",   pack_8_8},
	{"native",  pack_native},
	{"netbsd",  pack_netbsd},
	{"osf1",    pack_12_20},
	{"solaris", pack_14_18},
	{"sunos",   pack_8_8},
	{"svr3",    pack_8_8},
	{"svr4",    pack_14_18},
	{"ultrix",  pack_8_8},
};

int compare_format __P((const void *, const void *));

int
compare_format(key, element)
	const void *key;
	const void *element;
{
	const char *name;
	const struct format *format;

	name = key;
	format = element;

	return (strcmp(name, format->name));
}


int
main(argc, argv)
	int argc;
	char **argv;
{
	struct format *format;
	pack_t *pack;
	char *p;
	u_long maj, min, maj2, min2;
	mode_t mode;
	dev_t dev;
	int ch;

	pack = pack_native;

	while ((ch = getopt(argc, argv, "F:")) != -1) {
		switch (ch) {
		case 'F':
			format = bsearch(optarg, formats,
			    sizeof(formats)/sizeof(formats[0]),
			    sizeof(formats[0]), compare_format);
			if (format == 0)
				errx(1, "invalid format: %s", optarg);
			pack = format->pack;
			break;

		default:
		case '?':
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 3 && argc != 4)
		usage();

	mode = 0666;
	if (argv[1][0] == 'c')
		mode |= S_IFCHR;
	else if (argv[1][0] == 'b')
		mode |= S_IFBLK;
	else
		errx(1, "node must be type 'b' or 'c'.");

	if (argc == 4) {
		maj = strtoul(argv[2], &p, 0);
		if ((p && *p != '\0') || (maj == ULONG_MAX && errno == ERANGE))
			errx(1, "invalid major number: %s", argv[2]);

		min = strtoul(argv[3], &p, 0);
		if ((p && *p != '\0') || (min == ULONG_MAX && errno == ERANGE))
			errx(1, "invalid minor number: %s", argv[3]);

		dev = (*pack)(maj, min, &maj2, &min2);

		if (maj2 != maj)
			errx(1, "major number out of range: %s", argv[2]);

		if (min2 != min)
			errx(1, "minor number out of range: %s", argv[3]);
	} else {
		dev = (dev_t) strtoul(argv[2], &p, 0);
		if ((p && *p != '\0') || (dev == ULONG_MAX && errno == ERANGE))
			errx(1, "invalid device number: %s", argv[2]);
	}

	if (mknod(argv[0], mode, dev) < 0)
		err(1, "%s", argv[0]);

	exit(0);
}

void
usage()
{

	fprintf(stderr, "usage: mknod [-F format] name [b | c] major minor\n");
	fprintf(stderr, "       mknod name [b | c] number\n");
	exit(1);
}
