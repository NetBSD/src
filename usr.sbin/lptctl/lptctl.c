/* $NetBSD: lptctl.c,v 1.5 2004/01/28 17:54:03 jdolecek Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
static void print_lpt_info(int);

int
main(const int argc, const char * const * argv) {
	unsigned long ioctl_cmd = 0;
	int fd;
	int i;

	setprogname(argv[0]);

	/* N = command name + device name + number of command-arg pairs */
	/* Check number of arguments: at least 2, always even */
	if((argc < 2) || (argc % 2 != 0))
		usage(1);

	if ((fd = open(argv[1], O_RDONLY, 0)) == -1)
		err(2, "device open");

	/* Get command and arg pairs (if any) and do an ioctl for each */
	for(i = 2; i < argc; i += 2) {
		if (strcmp("dma", argv[i]) == 0) {
			if (strcmp("on", argv[i + 1]) == 0) {
				printf("Enabling DMA...\n");
				ioctl_cmd = LPTIO_ENABLE_DMA;
			} else if (strcmp("off", argv[i + 1]) == 0) {
				printf("Disabling DMA...\n");
				ioctl_cmd = LPTIO_DISABLE_DMA;
			} else {
				errx(2, "invalid '%s' command argument '%s'",
					argv[i], argv[i + 1]);
			}
		} else if (strcmp("mode", argv[i]) == 0) {
			if (strcmp("standard", argv[i + 1]) == 0) {
				printf("Using standard mode...\n");
				ioctl_cmd = LPTIO_MODE_STD;
			} else if (strcmp("ps2", argv[i + 1]) == 0) {
				printf("Using ps2 mode...\n");
				ioctl_cmd = LPTIO_MODE_PS2;
			} else if (strcmp("nibble", argv[i + 1]) == 0) {
				printf("Using nibble mode...\n");
				ioctl_cmd = LPTIO_MODE_NIBBLE;
			} else if (strcmp("fast", argv[i + 1]) == 0) {
				printf("Using fast centronics mode...\n");
				ioctl_cmd = LPTIO_MODE_FAST;
			} else if (strcmp("ecp", argv[i + 1]) == 0) {
				printf("Using ecp mode...\n");
				ioctl_cmd = LPTIO_MODE_ECP;
			} else if (strcmp("epp", argv[i + 1]) == 0) {
				printf("Using epp mode...\n");
				ioctl_cmd = LPTIO_MODE_EPP;
			} else {
				errx(2, "invalid '%s' command argument '%s'",
					argv[i], argv[i+1]);
			}
		} else if (strcmp("ieee", argv[i]) == 0) {
			if (strcmp("yes", argv[i + 1]) == 0) {
				printf("Using IEEE...\n");
				ioctl_cmd = LPTIO_ENABLE_IEEE;
			} else if (strcmp("no", argv[i + 1]) == 0) {
				printf("Not using IEEE...\n");
				ioctl_cmd = LPTIO_DISABLE_IEEE;
			} else {
				errx(2, "invalid '%s' command argument '%s'",
					argv[i], argv[i+1]);
			}
		} else {
			errx(2, "invalid command '%s'", argv[i]);
		}

		/* Do the command */
		if (ioctl(fd, ioctl_cmd, NULL) == -1)
			err(2, "%s: ioctl", __func__);
	}

	/* Print out information on device */
	print_lpt_info(fd);

	exit(0);
	/* NOTREACHED */
}

static void 
print_lpt_info(int fd) {
	LPT_INFO_T lpt_info;

	if (ioctl(fd, LPTIO_GET_STATUS, &lpt_info) == -1) {
		warnx("%s: ioctl", __func__);
		return;
	}

	printf("dma=%s ", (lpt_info.dma_status) ? "on" : "off");
		
	printf("mode=");
	switch(lpt_info.mode_status) {
	case standard:
		printf("standard ");
		break;
	case nibble:
		printf("nibble ");
		break;
	case fast:
		printf("fast ");
		break;
	case ps2:
		printf("ps2 ");
		break;
	case ecp:
		printf("ecp ");
		break;
	case epp:
		printf("epp ");
		break;
	}
		
	printf("ieee=%s ", (lpt_info.ieee_status) ? "yes" : "no");
		
	printf("\n");
}

static void 
usage(int status) {
	printf("usage:\t%s /dev/device [[command arg] ...]"
		"\n\t commands are:\n\tdma [on|off]\n\tieee [yes|no]"
		"\n\tmode [standard|ps2|nibble|fast|ecp|epp]\n",
		getprogname());
	exit(status);
}
