/*	$NetBSD: mdconfig.c,v 1.7 2020/08/18 19:26:29 christos Exp $	*/

/*
 * Copyright (c) 1995 Gordon W. Ross
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
__RCSID("$NetBSD: mdconfig.c,v 1.7 2020/08/18 19:26:29 christos Exp $");
#endif

/*
 * This program exists for the sole purpose of providing
 * user-space memory for the new memory-disk driver (md).
 * The job done by this is similar to mount_mfs.
 * (But this design allows any filesystem format!)
 */

#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/types.h>

#include <dev/md.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

int
main(int argc, char *argv[])
{
	struct md_conf md;
	char fname[MAXPATHLEN];
	int64_t num;
	size_t blks, bytes;
	int fd;

	setprogname(argv[0]);

	if (argc != 3) {
		(void)fprintf(stderr, "Usage: %s <device> <%d-byte-blocks>\n",
		    getprogname(), DEV_BSIZE);
		return EXIT_FAILURE;
	}

	if (dehumanize_number(argv[2], &num) == -1)
		goto bad_num;

	blks = (size_t)num;
	bytes = blks << DEV_BSHIFT;
	if (num <= 0 || bytes >> DEV_BSHIFT != blks) {
bad_num:	err(EXIT_FAILURE, "blocks: `%s'", argv[2]);
	}
	md.md_size = bytes;

	fd = opendisk(argv[1], O_RDWR, fname, sizeof(fname), 0);
	if (fd == -1)
		err(EXIT_FAILURE, "Can't open `%s'", argv[1]);

	md.md_addr = mmap(NULL, md.md_size, PROT_READ | PROT_WRITE,
	    MAP_ANON | MAP_PRIVATE, -1, 0);
	if (md.md_addr == MAP_FAILED)
		err(EXIT_FAILURE, "mmap");

	/* Become server! */
	md.md_type = MD_UMEM_SERVER;
	if (ioctl(fd, MD_SETCONF, &md) == -1)
		err(EXIT_FAILURE, "ioctl");

	return EXIT_SUCCESS;
}
