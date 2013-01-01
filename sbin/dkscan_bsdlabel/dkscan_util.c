/* $NetBSD: dkscan_util.c,v 1.3 2013/01/01 18:44:27 dsl Exp $ */

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
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <err.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/disk.h>
#include <sys/disklabel.h>

#include "dkscan_util.h"

int verbose = 0;	/* are we verbose? */
int no_action = 0;	/* don't do anything, just print info */
int disk_fd = -1;	/* file descriptor for disk access */


u_int
dkcksum(struct disklabel *lp)
{
	return dkcksum_sized(lp, lp->d_npartitions);
}

u_int
dkcksum_sized(struct disklabel *lp, size_t npartitions)
{
        u_short *start, *end;
        u_short sum = 0;

        start = (u_short *)lp;  
        end = (u_short *)&lp->d_partitions[npartitions];
        while (start < end)
                sum ^= *start++;   
        return (sum);
}

int
dkwedge_read(struct disk *pdk, struct vnode *vp, daddr_t blkno,
	void *tbuf, size_t len)
{
	if (pread(disk_fd, tbuf, len, blkno * BLOCK_SIZE) < 0)
		return errno;
	return 0;
}
	
int
dkwedge_add(struct dkwedge_info *dkw)
{
	printf("wedge \"%s\" type \"%s\" start %" PRIu64 " size %" PRIu64,
	    dkw->dkw_wname, dkw->dkw_ptype, dkw->dkw_offset, dkw->dkw_size);

	if (no_action) {
		printf("\n");
		return 0;
	}

	if (ioctl(disk_fd, DIOCAWEDGE, dkw) == -1)
		err(1, "addwedge");

	printf(": %s\n", dkw->dkw_devname);
	return 0;
}

void
aprint_error(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}

void
aprint_verbose(const char *format, ...)
{
	va_list ap;

	if (!verbose) return;

	va_start(ap, format);
	vfprintf(stdout, format, ap);
	va_end(ap);
}

