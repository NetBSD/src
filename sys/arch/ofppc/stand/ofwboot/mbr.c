/*	$NetBSD: mbr.c,v 1.1 2009/09/11 12:00:12 phx Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/bootblock.h>

#include <lib/libsa/byteorder.h>
#include <lib/libsa/stand.h>

#include "mbr.h"


static u_long
get_long(const void *p)
{
	const unsigned char *cp = p;

	return cp[0] | (cp[1] << 8) | (cp[2] << 16) | (cp[3] << 24);
}


/*
 * Find a valid MBR disklabel.
 */
int
search_mbr_label(struct of_dev *devp, u_long off, char *buf,
    struct disklabel *lp, u_long off0)
{
	size_t read;
	struct mbr_partition *p;
	int i;
	u_long poff;
	static int recursion;

	if (strategy(devp, F_READ, off, DEV_BSIZE, buf, &read)
	    || read != DEV_BSIZE)
		return ERDLAB;

	if (*(u_int16_t *)&buf[MBR_MAGIC_OFFSET] != sa_htole16(MBR_MAGIC))
		return ERDLAB;

	if (recursion++ <= 1)
		off0 += off;
	for (p = (struct mbr_partition *)(buf + MBR_PART_OFFSET), i = 0;
	     i < MBR_PART_COUNT; i++, p++) {
		if (p->mbrp_type == MBR_PTYPE_NETBSD
#ifdef COMPAT_386BSD_MBRPART
		    || (p->mbrp_type == MBR_PTYPE_386BSD &&
			(printf("WARNING: old BSD partition ID!\n"), 1)
			/* XXX XXX - libsa printf() is void */ )
#endif
		    ) {
			poff = get_long(&p->mbrp_start) + off0;
			if (strategy(devp, F_READ, poff + LABELSECTOR,
				     DEV_BSIZE, buf, &read) == 0
			    && read == DEV_BSIZE) {
				if (!getdisklabel(buf, lp)) {
					recursion--;
					return 0;
				}
			}
			if (strategy(devp, F_READ, off, DEV_BSIZE, buf, &read)
			    || read != DEV_BSIZE) {
				recursion--;
				return ERDLAB;
			}
		} else if (p->mbrp_type == MBR_PTYPE_EXT) {
			poff = get_long(&p->mbrp_start);
			if (!search_mbr_label(devp, poff, buf, lp, off0)) {
				recursion--;
				return 0;
			}
			if (strategy(devp, F_READ, off, DEV_BSIZE, buf, &read)
			    || read != DEV_BSIZE) {
				recursion--;
				return ERDLAB;
			}
		}
	}

	recursion--;
	return ERDLAB;
}
