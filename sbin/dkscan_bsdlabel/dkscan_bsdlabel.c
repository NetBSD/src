/* $NetBSD: dkscan_bsdlabel.c,v 1.4 2017/06/08 22:24:29 chs Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Martin Husemann <martin@NetBSD.org>.
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
 
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <util.h>
#include <sys/disk.h>

#include "dkscan_util.h"

struct disk {
	const char	*dk_name;	/* disk name */
	int		dk_blkshift;	/* shift to convert DEV_BSIZE to blks */
};

#include "dkwedge_bsdlabel.c"

__dead static void usage(void);

int
main(int argc, char **argv)
{
	struct disk d;
	int ch;
	char buf[PATH_MAX];
	const char *devpart;

	if (argc < 2)
		usage();

	while ((ch = getopt(argc, argv, "nv")) != -1) {
		switch (ch) {
		case 'n':
			no_action = 1;
			break;
		case 'v':
			verbose++;
			break;
		default:
			usage();
		}
	}
	if (optind >= argc)
		usage();

	disk_fd = opendisk(argv[optind], O_RDWR, buf, PATH_MAX, 0);
	if (disk_fd == -1)
		err(1, "%s", argv[optind]);

	devpart = strrchr(argv[optind], '/');
	if (devpart == NULL)
		devpart = argv[optind];

	memset(&d, 0, sizeof(d));
	d.dk_name  = devpart;
	dkwedge_discover_bsdlabel(&d, NULL);

	close(disk_fd);
	return 0;
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-vn] <diskname>\n"
	    "  where\n"
	    "    -n don't change anything, just print info\n"
	    "    -v be more verbose\n"
	    "    <diskname> device to scan\n",
	    getprogname());
	exit(1);
}
