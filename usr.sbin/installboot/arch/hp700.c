/*	$NetBSD: hp700.c,v 1.1 2005/05/14 14:46:21 chs Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn of Wasabi Systems.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if !defined(__lint)
__RCSID("$NetBSD: hp700.c,v 1.1 2005/05/14 14:46:21 chs Exp $");
#endif	/* !__lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <assert.h>
#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "installboot.h"

#define HP700_LABELOFFSET	512
#define HP700_LABELSIZE		404 /* reserve 16 partitions */
#define	HP700_BOOT_BLOCK_SIZE	8192

int
hp700_clearboot(ib_params *params)
{
	char		bb[HP700_BOOT_BLOCK_SIZE];
	int		retval, eol;
	ssize_t		rv;

	assert(params != NULL);
	assert(params->fsfd != -1);
	assert(params->filesystem != NULL);

	retval = 0;

	/* read disklabel on the target disk */
	rv = pread(params->fsfd, bb, sizeof bb, 0);
	if (rv == -1) {
		warn("Reading `%s'", params->filesystem);
		goto done;
	} else if (rv != sizeof bb) {
		warnx("Reading `%s': short read", params->filesystem);
		goto done;
	}

	/* clear header */
	memset(bb, 0, HP700_LABELOFFSET);
	eol = HP700_LABELOFFSET + HP700_LABELSIZE;
	memset(&bb[eol], 0, sizeof bb - eol);

	if (params->flags & IB_VERBOSE) {
		printf("%slearing bootstrap\n",
		    (params->flags & IB_NOWRITE) ? "Not c" : "C");
	}
	if (params->flags & IB_NOWRITE) {
		retval = 1;
		goto done;
	}

	rv = pwrite(params->fsfd, bb, sizeof bb, 0);
	if (rv == -1) {
		warn("Writing `%s'", params->filesystem);
		goto done;
	} else if (rv != HP700_BOOT_BLOCK_SIZE) {
		warnx("Writing `%s': short write", params->filesystem);
		goto done;
	} else
		retval = 1;

 done:
	return (retval);
}

int
hp700_setboot(ib_params *params)
{
	struct stat	bootstrapsb;
	char		bb[HP700_BOOT_BLOCK_SIZE];
	char		label[HP700_BOOT_BLOCK_SIZE];
	int		retval;
	ssize_t		rv;

	assert(params != NULL);
	assert(params->fsfd != -1);
	assert(params->filesystem != NULL);
	assert(params->s1fd != -1);
	assert(params->stage1 != NULL);

	retval = 0;

	/* read disklabel on the target disk */
	rv = pread(params->fsfd, label, sizeof label, 0);
	if (rv == -1) {
		warn("Reading `%s'", params->filesystem);
		goto done;
	} else if (rv != sizeof label) {
		warnx("Reading `%s': short read", params->filesystem);
		goto done;
	}

	if (fstat(params->s1fd, &bootstrapsb) == -1) {
		warn("Examining `%s'", params->stage1);
		goto done;
	}
	if (!S_ISREG(bootstrapsb.st_mode)) {
		warnx("`%s' must be a regular file", params->stage1);
		goto done;
	}

	/* read boot loader */
	memset(&bb, 0, sizeof bb);
	rv = read(params->s1fd, &bb, sizeof bb);
	if (rv == -1) {
		warn("Reading `%s'", params->stage1);
		goto done;
	}
	/* then, overwrite disklabel */
	memcpy(&bb[HP700_LABELOFFSET], &label[HP700_LABELOFFSET],
	    HP700_LABELSIZE);

	if (params->flags & IB_VERBOSE) {
		printf("Bootstrap start sector: %#x\n", 0);
		printf("Bootstrap byte count:   %#zx\n", rv);
		printf("%sriting bootstrap\n",
		    (params->flags & IB_NOWRITE) ? "Not w" : "W");
	}
	if (params->flags & IB_NOWRITE) {
		retval = 1;
		goto done;
	}

	/* write boot loader and disklabel into the target disk */
	rv = pwrite(params->fsfd, &bb, HP700_BOOT_BLOCK_SIZE, 0);
	if (rv == -1) {
		warn("Writing `%s'", params->filesystem);
		goto done;
	} else if (rv != HP700_BOOT_BLOCK_SIZE) {
		warnx("Writing `%s': short write", params->filesystem);
		goto done;
	} else
		retval = 1;

 done:
	return (retval);
}
