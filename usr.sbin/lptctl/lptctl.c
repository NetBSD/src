/* $NetBSD: lptctl.c,v 1.11 2008/04/28 20:24:16 martin Exp $ */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gary Thorpe.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: lptctl.c,v 1.11 2008/04/28 20:24:16 martin Exp $");

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>

#include <sys/ioctl.h>

#include <dev/ppbus/lptio.h> 

/* Prototypes */
static void usage(int status);
static void print_lpt_info(int, int);

int
main(const int argc, const char * const * argv) {
	int fd, i;
	int omode, mode, oflags, flags;

	setprogname(argv[0]);

	/* N = command name + device name + number of command-arg pairs */
	/* Check number of arguments: at least 2, always even */
	if((argc < 2) || (argc % 2 != 0))
		usage(1);

	if ((fd = open(argv[1], O_RDONLY, 0)) == -1)
		err(2, "device open");

	/* get current settings */
	if (ioctl(fd, LPTGFLAGS, &flags) == -1)
		err(2, "ioctl(LPTGFLAGS)");
	oflags = flags;

	if (ioctl(fd, LPTGMODE, &mode) == -1)
		err(2, "ioctl(LPTGMODE)");
	omode = mode;

	/* Get command and arg pairs (if any) and do an ioctl for each */
	for(i = 2; i < argc; i += 2) {
		if (strcmp("dma", argv[i]) == 0) {
			if (strcmp("yes", argv[i + 1]) == 0)
				flags |= LPT_DMA;
			else if (strcmp("no", argv[i + 1]) == 0)
				flags &= ~LPT_DMA;
			else {
				errx(2, "invalid '%s' command argument '%s'",
					argv[i], argv[i + 1]);
			}
		} else if (strcmp("mode", argv[i]) == 0) {
			if (strcmp("standard", argv[i + 1]) == 0)
				mode = mode_standard;
			else if (strcmp("ps2", argv[i + 1]) == 0)
				mode = mode_ps2;
			else if (strcmp("nibble", argv[i + 1]) == 0)
				mode = mode_nibble;
			else if (strcmp("fast", argv[i + 1]) == 0)
				mode = mode_fast;
			else if (strcmp("ecp", argv[i + 1]) == 0)
				mode = mode_ecp;
			else if (strcmp("epp", argv[i + 1]) == 0)
				mode = mode_epp;
			else {
				errx(2, "invalid '%s' command argument '%s'",
					argv[i], argv[i+1]);
			}
		} else if (strcmp("ieee", argv[i]) == 0) {
			if (strcmp("yes", argv[i + 1]) == 0)
				flags |= LPT_IEEE;
			else if (strcmp("no", argv[i + 1]) == 0)
				flags &= ~LPT_IEEE;
			else {
				errx(2, "invalid '%s' command argument '%s'",
					argv[i], argv[i+1]);
			}
		} else if (strcmp("intr", argv[i]) == 0) {
			if (strcmp("yes", argv[i + 1]) == 0)
				flags |= LPT_INTR;
			else if (strcmp("no", argv[i + 1]) == 0)
				flags &= ~LPT_INTR;
			else {
				errx(2, "invalid '%s' command argument '%s'",
					argv[i], argv[i+1]);
			}
		} else if (strcmp("prime", argv[i]) == 0) {
			if (strcmp("yes", argv[i + 1]) == 0)
				flags |= LPT_PRIME;
			else if (strcmp("no", argv[i + 1]) == 0)
				flags &= ~LPT_PRIME;
			else {
				errx(2, "invalid '%s' command argument '%s'",
					argv[i], argv[i+1]);
			}
		} else if (strcmp("autolf", argv[i]) == 0) {
			if (strcmp("yes", argv[i + 1]) == 0)
				flags |= LPT_AUTOLF;
			else if (strcmp("no", argv[i + 1]) == 0)
				flags &= ~LPT_AUTOLF;
			else {
				errx(2, "invalid '%s' command argument '%s'",
					argv[i], argv[i+1]);
			}
		} else {
			errx(2, "invalid command '%s'", argv[i]);
		}
	}

	/* update mode and flags */
	if (flags != oflags) {
		if (ioctl(fd, LPTSFLAGS, &flags) == -1)
			err(2, "ioctl(LPTSFLAGS)");
	}
	if (mode != omode) {
		if (ioctl(fd, LPTSMODE, &mode) == -1)
			err(2, "ioctl(LPTSMODE)");
	}

	/* Print out information on device */
	printf("%s status:\n\t", argv[1]);
	print_lpt_info(mode, flags);

	exit(0);
	/* NOTREACHED */
}

static void 
print_lpt_info(int mode, int flags) {
	printf("mode=");
	switch(mode) {
	case mode_standard:
		printf("standard ");
		break;
	case mode_nibble:
		printf("nibble ");
		break;
	case mode_fast:
		printf("fast ");
		break;
	case mode_ps2:
		printf("ps2 ");
		break;
	case mode_ecp:
		printf("ecp ");
		break;
	case mode_epp:
		printf("epp ");
		break;
	default:
		printf("<unknown> ");
		break;
	}

	printf("dma=%s ", (flags & LPT_DMA) ? "yes" : "no");
	printf("ieee=%s ", (flags & LPT_IEEE) ? "yes" : "no");
	printf("intr=%s ", (flags & LPT_INTR) ? "yes" : "no");
	printf("prime=%s ", (flags & LPT_PRIME) ? "yes" : "no");
	printf("autolf=%s ", (flags & LPT_AUTOLF) ? "yes" : "no");

	printf("\n");
}

static void 
usage(int status) {
	printf("usage:\t%s /dev/device [[command arg] ...]\n"
		"\tcommands are:\n"
		"\t\tmode [standard|ps2|nibble|fast|ecp|epp]\n"
		"\t\tdma [yes|no]\n"
		"\t\tieee [yes|no]\n"
		"\t\tintr [yes|no]\n"
		"\t\tprime [yes|no]\n"
		"\t\tautolf [yes|no]\n",
		getprogname());
	exit(status);
}
