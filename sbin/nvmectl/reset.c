/*	$NetBSD: reset.c,v 1.2 2018/03/17 11:07:26 jdolecek Exp $	*/

/*-
 * Copyright (C) 2012-2013 Intel Corporation
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: reset.c,v 1.2 2018/03/17 11:07:26 jdolecek Exp $");
#if 0
__FBSDID("$FreeBSD: head/sbin/nvmecontrol/reset.c 253109 2013-07-09 21:14:15Z jimharris $");
#endif
#endif

#include <sys/param.h>
#include <sys/ioccom.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nvmectl.h"

#ifdef RESET_USAGE
static void
reset_usage(void)
{
	fprintf(stderr, "usage:\n");
	fprintf(stderr, "\t%s " RESET_USAGE, getprogname());
	exit(1);
}

void
reset(int argc, char *argv[])
{
	int	ch, fd;

	while ((ch = getopt(argc, argv, "")) != -1) {
		switch (ch) {
		default:
			reset_usage();
		}
	}

	/* Check that a controller was specified. */
	if (optind >= argc)
		reset_usage();

	open_dev(argv[optind], &fd, 1, 1);
	if (ioctl(fd, NVME_RESET_CONTROLLER) < 0)
		err(1, "reset request to %s failed", argv[optind]);

	exit(0);
}
#endif
