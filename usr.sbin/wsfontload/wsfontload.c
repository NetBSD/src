/* $NetBSD: wsfontload.c,v 1.24 2022/05/12 22:08:55 uwe Exp $ */

/*
 * Copyright (c) 1999
 *	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <err.h>

#include <dev/wscons/wsconsio.h>

#define DEFDEV		"/dev/wsfont"
#define DEFWIDTH	8
#define DEFHEIGHT	16
#define DEFENC		WSDISPLAY_FONTENC_ISO
#define DEFBITORDER	WSDISPLAY_FONTORDER_L2R
#define DEFBYTEORDER	WSDISPLAY_FONTORDER_L2R

__dead static void usage(void);
static int getencoding(char *);
static const char *rgetencoding(int);
static const char *rgetfontorder(int);

static struct {
	const char *name;
	int val;
} fontorders[] = {
	{ "known", WSDISPLAY_FONTORDER_KNOWN}, 
	{ "l2r", WSDISPLAY_FONTORDER_L2R}, 
	{ "r2l", WSDISPLAY_FONTORDER_R2L}, 
};

static struct {
	const char *name;
	int val;
} encodings[] = {
	{"iso", WSDISPLAY_FONTENC_ISO},
	{"ibm", WSDISPLAY_FONTENC_IBM},
	{"pcvt", WSDISPLAY_FONTENC_PCVT},
	{"iso7", WSDISPLAY_FONTENC_ISO7},
	{"iso2", WSDISPLAY_FONTENC_ISO2},
	{"koi8r", WSDISPLAY_FONTENC_KOI8_R},
};

static void
usage(void)
{

	(void)fprintf(stderr,
		"usage: %s [-Bbv] [-e encoding] [-f wsdev] [-h height]"
		" [-N name] [-w width] [fontfile]\n"
		"       %s -l\n",
		      getprogname(),
		      getprogname());
	exit(1);
}

/*
 * map given fontorder to its string representation
 */
static const char *
rgetfontorder(int fontorder)
{
	size_t i;

	for (i = 0; i < sizeof(fontorders) / sizeof(fontorders[0]); i++)
		if (fontorders[i].val == fontorder)
			return (fontorders[i].name);

	return "unknown";
}

/* 
 * map given encoding to its string representation
 */
static const char *
rgetencoding(int enc)
{
	size_t i;

	for (i = 0; i < sizeof(encodings) / sizeof(encodings[0]); i++)
		if (encodings[i].val == enc)
			return (encodings[i].name);

	return "unknown";
}

/*
 * map given encoding string to integer value
 */
static int
getencoding(char *name)
{
	size_t i;
	int j;

	for (i = 0; i < sizeof(encodings) / sizeof(encodings[0]); i++)
		if (!strcmp(name, encodings[i].name))
			return (encodings[i].val);

	if (sscanf(name, "%d", &j) != 1)
		errx(1, "invalid encoding");
	return (j);
}

int
main(int argc, char **argv)
{
	const char *wsdev;
	struct wsdisplay_font f;
	struct stat st;
	size_t len;
	int c, res, wsfd, ffd, verbose = 0, listfonts = 0;
	int use_embedded_name = 1;
	void *buf;
	char nbuf[65];

	wsdev = DEFDEV;
	f.fontwidth = DEFWIDTH;
	f.fontheight = DEFHEIGHT;
	f.firstchar = 0;
	f.numchars = 256;
	f.stride = 0;
	f.encoding = DEFENC;
	f.name = 0;
	f.bitorder = DEFBITORDER;
	f.byteorder = DEFBYTEORDER;

	while ((c = getopt(argc, argv, "f:w:h:e:N:bBvl")) != -1) {
		switch (c) {
		case 'f':
			wsdev = optarg;
			break;
		case 'l':
			listfonts = 1;
			break;
		case 'w':
			if (sscanf(optarg, "%d", &f.fontwidth) != 1)
				errx(1, "invalid font width");
			break;
		case 'h':
			if (sscanf(optarg, "%d", &f.fontheight) != 1)
				errx(1, "invalid font height");
			break;
		case 'e':
			f.encoding = getencoding(optarg);
			break;
		case 'N':
			f.name = optarg;
			use_embedded_name = 0;
			break;
		case 'b':
			f.bitorder = WSDISPLAY_FONTORDER_R2L;
			break;
		case 'B':
			f.byteorder = WSDISPLAY_FONTORDER_R2L;
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 1)
		usage();

	wsfd = open(wsdev, listfonts ? O_RDONLY : O_RDWR, 0);
	if (wsfd < 0)
		err(2, "open ws-device %s", wsdev);

	if (listfonts == 1) {
		struct wsdisplayio_fontinfo fi;
		int ret;
		unsigned int i;

		fi.fi_buffersize = 4096;
		fi.fi_numentries = 0;
		fi.fi_fonts = malloc(4096);
		ret = ioctl(wsfd, WSDISPLAYIO_LISTFONTS, &fi);
		if (fi.fi_fonts == NULL || ret != 0) {
			err(1, "error fetching font list\n");
		}
		for (i = 0; i < fi.fi_numentries; i++) {
			printf("%s %dx%d\n", fi.fi_fonts[i].fd_name,
			    fi.fi_fonts[i].fd_width, fi.fi_fonts[i].fd_height);
		}
		return 0;
	}	

	if (argc > 0) {
		ffd = open(argv[0], O_RDONLY, 0);
		if (ffd < 0)
			err(4, "open font %s", argv[0]);
		if (!f.name)
			f.name = argv[0];
	} else
		ffd = 0;

	if (!f.stride)
		f.stride = (f.fontwidth + 7) / 8;
	len = f.fontheight * f.numchars * f.stride;
	if ((ffd != 0) && (fstat(ffd, &st) == 0)) {
		if ((off_t)len != st.st_size) {
			uint32_t foo = 0;
			char b[65];
			len = st.st_size;
			/* read header */
			read(ffd, b, 4);
			if (strncmp(b, "WSFT", 4) != 0)
				errx(1, "invalid wsf file ");
			read(ffd, b, 64);
			if (use_embedded_name) {
				b[64] = 0;
				strcpy(nbuf, b);
				f.name = nbuf;
			}
			read(ffd, &foo, 4);
			f.firstchar = le32toh(foo);
			read(ffd, &foo, 4);
			f.numchars = le32toh(foo);
			read(ffd, &foo, 4);
			f.encoding = le32toh(foo);
			read(ffd, &foo, 4);
			f.fontwidth = le32toh(foo);
			read(ffd, &foo, 4);
			f.fontheight = le32toh(foo);
			read(ffd, &foo, 4);
			f.stride = le32toh(foo);
			read(ffd, &foo, 4);
			f.bitorder = le32toh(foo);
			read(ffd, &foo, 4);
			f.byteorder = le32toh(foo);
			len = f.numchars * f.fontheight * f.stride;
		}
	}

	if (!len)
		errx(1, "invalid font size");

	buf = malloc(len);
	if (!buf)
		errx(1, "malloc");
	res = read(ffd, buf, len);
	if (res < 0)
		err(4, "read font");
	if ((size_t)res != len)
		errx(4, "short read");

	f.data = buf;

	if (verbose) {
		printf("name:       %s\n", f.name);
		printf("firstchar:  %d\n", f.firstchar);
		printf("numchars:   %d\n", f.numchars);
		printf("encoding:   %s (%d)\n", 
			rgetencoding(f.encoding), f.encoding);
		printf("fontwidth:  %d\n", f.fontwidth);
		printf("fontheight: %d\n", f.fontheight);
		printf("stride:     %d\n", f.stride);
		printf("bitorder:   %s (%d)\n",
			rgetfontorder(f.bitorder), f.bitorder);
		printf("byteorder:  %s (%d)\n",
			rgetfontorder(f.byteorder), f.byteorder);
	}

	res = ioctl(wsfd, WSDISPLAYIO_LDFONT, &f);
	if (res < 0)
		err(3, "WSDISPLAYIO_LDFONT");

	return (0);
}
