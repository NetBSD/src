/*	$NetBSD: amiga.c,v 1.3 2004/06/20 22:20:17 jmc Exp $	*/

/*-
 * Copyright (c) 1999, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Hitch.
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
#if defined(__RCSID) && !defined(__lint)
__RCSID("$NetBSD: amiga.c,v 1.3 2004/06/20 22:20:17 jmc Exp $");
#endif	/* !__lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <assert.h>
#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "installboot.h"

/* XXX Must be kept in sync with bbstart.s! */
#define CMDLN_LOC 0x10
#define CMDLN_LEN 0x20

#define CHKSUMOFFS 1

u_int32_t chksum(u_int32_t *, int);

int		amiga_setboot(ib_params *);
int		amiga_clearboot(ib_params *);

int
amiga_clearboot(ib_params *params)
{
	return 0;
}

int
amiga_setboot(ib_params *params)
{
	int retval;
	ssize_t			rv;
	char *dline;
	int sumlen;
	u_int32_t sum2, sum16;
	
	struct stat		bootstrapsb;

	u_int32_t block[128*16];

	if (fstat(params->s1fd, &bootstrapsb) == -1) {
		warn("Examining `%s'", params->stage1);
		goto done;
	}
	if (!S_ISREG(bootstrapsb.st_mode)) {
		warnx("`%s' must be a regular file", params->stage1);
		goto done;
	}

	rv = pread(params->s1fd, &block, sizeof(block), 0);
	if (rv == -1) {
		warn("Reading `%s'", params->stage1);
		goto done;
	} else if (rv != sizeof(block)) {
		warnx("Reading `%s': short read", params->stage1);
		goto done;
	}

	/* XXX the choices should not be hardcoded */

	sum2  = chksum(block, 1024/4);
	sum16 = chksum(block, 8192/4);

	if (sum16 == 0xffffffff) {
		sumlen = 8192/4;
	} else if (sum2 == 0xffffffff) {
		sumlen = 1024/4;
	} else {
		errx(1, "%s: wrong checksum", params->stage1);
		/* NOTREACHED */
	}

	if (sum2 == sum16) {
		warnx("eek - both sums are the same");
	}

	if (params->flags & IB_COMMAND) {
		dline = (char *)&(block[CMDLN_LOC/4]);
		/* XXX keep the default default line in sync with bbstart.s */
		if (strcmp(dline, "netbsd -ASn2") != 0) {
			errx(1, "Old bootblock version? Can't change command line.");
		}
		(void)strncpy(dline, params->command, CMDLN_LEN-1);

		block[1] = 0;
		block[1] = 0xffffffff - chksum(block, sumlen);
	}

	if (params->flags & IB_NOWRITE) {
		retval = 1;
		goto done;
	}

	if (params->flags & IB_VERBOSE)
		printf("Writing boot block\n");
	rv = pwrite(params->fsfd, &block, sizeof(block), 0);
	if (rv == -1) {
		warn("Writing `%s'", params->filesystem);
		goto done;
	} else if (rv != sizeof(block)) {
		warnx("Writing `%s': short write", params->filesystem);
		goto done;
	} else {
		retval = 1;
	}

 done:
	return (retval);
}

u_int32_t
chksum(block, size)
	u_int32_t *block;
	int size;
{
	u_int32_t sum, lastsum;
	int i;

	sum = 0;

	for (i=0; i<size; i++) {
		lastsum = sum;
		sum += htobe32(block[i]);
		if (sum < lastsum)
			++sum;
	}

	return sum;
}
