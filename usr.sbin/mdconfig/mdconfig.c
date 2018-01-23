/*	$NetBSD: mdconfig.c,v 1.6 2018/01/23 21:06:25 sevan Exp $	*/

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
__RCSID("$NetBSD: mdconfig.c,v 1.6 2018/01/23 21:06:25 sevan Exp $");
#endif

/*
 * This program exists for the sole purpose of providing
 * user-space memory for the new memory-disk driver (md).
 * The job done by this is similar to mount_mfs.
 * (But this design allows any filesystem format!)
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/param.h>

#include <dev/md.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	struct md_conf md;
	size_t nblks;
	int fd;

	if (argc <= 2) {
		fprintf(stderr, "usage: mdconfig <device> <%d-byte-blocks>\n",
				DEV_BSIZE);
		exit(1);
	}

	nblks = (size_t)strtoul(argv[2], NULL, 0);
	if (nblks == 0) {
		fprintf(stderr, "invalid number of blocks\n");
		exit(1);
	}
	md.md_size = nblks << DEV_BSHIFT;

	fd = open(argv[1], O_RDWR, 0);
	if (fd < 0) {
		perror(argv[1]);
		exit(1);
	}

	md.md_addr = mmap(NULL, md.md_size,
				PROT_READ | PROT_WRITE,
				MAP_ANON | MAP_PRIVATE,
				-1, 0);
	if (md.md_addr == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	/* Become server! */
	md.md_type = MD_UMEM_SERVER;
	if (ioctl(fd, MD_SETCONF, &md)) {
		perror("ioctl");
		exit(1);
	}

	exit(0);
}
