/* $NetBSD: i386.c,v 1.1 2003/04/09 22:14:27 dsl Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David Laight.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: i386.c,v 1.1 2003/04/09 22:14:27 dsl Exp $");

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/param.h>

#include <assert.h>
#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "installboot.h"

/* Magic number also known by sys/arch/i386/stand/bootxx/Makefile.bootxx */
#define BOOT_MAGIC_1	('x' << 24 | 0x86b << 12 | 'm' << 4 | 1)

int
i386_setboot(ib_params *params)
{
	int		retval;
	char		*bootstrapbuf;
	uint		bootstrapsize;
	ssize_t		rv;
	uint32_t	magic;

	assert(params != NULL);
	assert(params->fsfd != -1);
	assert(params->filesystem != NULL);
	assert(params->s1fd != -1);
	assert(params->stage1 != NULL);

	retval = 0;
	bootstrapbuf = NULL;

	if (params->flags & IB_STAGE1START) {
		warnx("`-b bno' is not supported for %s",
		    params->machine->name);
		goto done;
	}
	if (params->flags & IB_STAGE2START) {
		warnx("`-B bno' is not supported for %s",
		    params->machine->name);
		goto done;
	}

	/*
	 * Allocate a buffer, with space to round up the input file
	 * to the next block size boundary, and with space for the boot
	 * block.
	 */
	bootstrapsize = roundup(params->s1stat.st_size, 512);

	bootstrapbuf = malloc(bootstrapsize);
	if (bootstrapbuf == NULL) {
		warn("Allocating %u bytes",  bootstrapsize);
		goto done;
	}
	memset(bootstrapbuf, 0, bootstrapsize);

	/* read the file into the buffer */
	rv = pread(params->s1fd, bootstrapbuf, params->s1stat.st_size, 0);
	if (rv == -1) {
		warn("Reading `%s'", params->stage1);
		return 0;
	} else if (rv != params->s1stat.st_size) {
		warnx("Reading `%s': short read", params->stage1);
		return 0;
	}

	magic = *(uint32_t *)(bootstrapbuf + 512 * 2 + 4);
	if (magic != BOOT_MAGIC_1) {
		warnx("Invalid magic in stage1 boostrap %x != %x",
			magic, BOOT_MAGIC_1);
		goto done;
	}

	if (params->flags & IB_NOWRITE) {
		retval = 1;
		goto done;
	}

	/* Write pbr code to sector zero */
	rv = pwrite(params->fsfd, bootstrapbuf, 512, 0);
	if (rv == -1) {
		warn("Writing `%s'", params->filesystem);
		goto done;
	} else if (rv != 512) {
		warnx("Writing `%s': short write", params->filesystem);
		goto done;
	}

	/* Skip disklabel and write bootxx to sectors 2 + */
	rv = pwrite(params->fsfd, bootstrapbuf + 512 * 2,
		    bootstrapsize - 512 * 2, 512 * 2);
	if (rv == -1) {
		warn("Writing `%s'", params->filesystem);
		goto done;
	} else if (rv != bootstrapsize - 512 * 2) {
		warnx("Writing `%s': short write", params->filesystem);
		goto done;
	}

	retval = 1;

 done:
	if (bootstrapbuf)
		free(bootstrapbuf);
	return retval;
}
