/*	$NetBSD: bootinfo_biosgeom.c,v 1.8.22.1 2001/08/24 00:08:40 nathanw Exp $	*/

/*
 * Copyright (c) 1997
 *	Matthias Drochner.  All rights reserved.
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
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 */

#include <sys/types.h>
#include <machine/disklabel.h>
#include <machine/cpu.h>

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>

#include "libi386.h"
#include "biosdisk_ll.h"
#include "bootinfo.h"

void bi_getbiosgeom()
{
	struct btinfo_biosgeom *bibg;
	int i, j, nvalid;
	unsigned char nhd;
	unsigned int cksum;

	pvbcopy((void *)(0x400 + 0x75), &nhd, 1);
#ifdef GEOM_DEBUG
	printf("nhd %d\n", (int)nhd);
#endif

	bibg = alloc(sizeof(struct btinfo_biosgeom)
		     + (nhd - 1) * sizeof(struct bi_biosgeom_entry));
	if (!bibg)
		return;

	for (i = nvalid = 0; i < MAX_BIOSDISKS && nvalid < (int)nhd; i++) {
		struct biosdisk_ll d;
		struct biosdisk_ext13info ed;
		char buf[BIOSDISK_SECSIZE];

		d.dev = 0x80 + i;

		if (set_geometry(&d, &ed))
			continue;
		memset(&bibg->disk[nvalid], 0, sizeof(bibg->disk[nvalid]));

		bibg->disk[nvalid].sec = d.sec;
		bibg->disk[nvalid].head = d.head;
		bibg->disk[nvalid].cyl = d.cyl;
		bibg->disk[nvalid].dev = d.dev;

		if (readsects(&d, 0, 1, buf, 0)) {
			bibg->disk[nvalid].flags |= BI_GEOM_INVALID;
			nvalid++;
			continue;
		}

#ifdef GEOM_DEBUG
		printf("#%d: %x: C %d H %d S %d\n", nvalid,
		    d.dev, d.cyl, d.head, d.sec);
#endif

		if (d.flags & BIOSDISK_EXT13) {
			if (ed.flags & EXT13_GEOM_VALID)
				bibg->disk[nvalid].totsec = ed.totsec;
			else
				bibg->disk[nvalid].totsec = 0;
			bibg->disk[nvalid].flags |= BI_GEOM_EXTINT13;
		}
		for (j = 0, cksum = 0; j < BIOSDISK_SECSIZE; j++)
			cksum += buf[j];
		bibg->disk[nvalid].cksum = cksum;
		memcpy(bibg->disk[nvalid].dosparts, &buf[MBR_PARTOFF],
		      sizeof(bibg->disk[nvalid].dosparts));
		nvalid++;
	}

	bibg->num = nvalid;

	BI_ADD(bibg, BTINFO_BIOSGEOM, sizeof(struct btinfo_biosgeom)
	       + nvalid * sizeof(struct bi_biosgeom_entry));
}
