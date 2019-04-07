/*	$NetBSD: subr_disklabel.c,v 1.3 2019/04/07 02:58:02 rin Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ufs_disksubr.c	7.16 (Berkeley) 5/4/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_disklabel.c,v 1.3 2019/04/07 02:58:02 rin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/disklabel.h>

#if !defined(__HAVE_SETDISKLABEL) || defined(_RUMPKERNEL)

#ifdef DEBUG
#define DPRINTF(a, ...) printf(a, ##__VA_ARGS__)
#else
#define DPRINTF(a, ...) __nothing
#endif

/*
 * Check new disk label for sensibility
 * before setting it.
 */
int
setdisklabel(struct disklabel *olp, struct disklabel *nlp, u_long openmask,
    struct cpu_disklabel *osdep)
{
	int i;
	struct partition *opp, *npp;

	/* sanity clause */
	if (nlp->d_secpercyl == 0 || nlp->d_secsize == 0
		|| (nlp->d_secsize % DEV_BSIZE) != 0) {
		DPRINTF("%s: secpercyl/secsize %u/%u\n", __func__,
		    nlp->d_secpercyl, nlp->d_secsize);
		return EINVAL;
	}

	/* special case to allow disklabel to be invalidated */
	if (nlp->d_magic == 0xffffffff) {
		*olp = *nlp;
		return 0;
	}

	if (nlp->d_magic != DISKMAGIC || nlp->d_magic2 != DISKMAGIC ||
	    nlp->d_npartitions > MAXPARTITIONS || dkcksum(nlp) != 0) {
		DPRINTF("%s: bad magic %#x/%#x != %#x, partitions %u != %u"
		    ", bad sum=%#x\n", __func__,
		    nlp->d_magic, nlp->d_magic2, DISKMAGIC,
		    nlp->d_npartitions, MAXPARTITIONS, dkcksum(nlp));
		return EINVAL;
	}

	while (openmask != 0) {
		i = ffs(openmask) - 1;
		openmask &= ~(1 << i);
		if (i >= nlp->d_npartitions) {
			DPRINTF("%s: partition not found\n", __func__);
			return EBUSY;
		}
		opp = &olp->d_partitions[i];
		npp = &nlp->d_partitions[i];
		/*
		 * Copy internally-set partition information
		 * if new label doesn't include it.		XXX
		 */
		if (npp->p_fstype == FS_UNUSED && opp->p_fstype != FS_UNUSED) {
			*npp = *opp;
			continue;
		}
		if (npp->p_offset != opp->p_offset || npp->p_size < opp->p_size)
		{
			DPRINTF("%s: mismatched offset/size", __func__);
			return EBUSY;
		}
	}
 	nlp->d_checksum = 0;
 	nlp->d_checksum = dkcksum(nlp);
	*olp = *nlp;
	return 0;
}
#endif
