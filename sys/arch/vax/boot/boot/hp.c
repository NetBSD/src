/*	$NetBSD: hp.c,v 1.3 2000/05/20 13:30:03 ragge Exp $ */
/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

 /* All bugs are subject to removal without further notice */
		


#include "sys/param.h"
#include "sys/disklabel.h"

#include "lib/libsa/stand.h"

#include "lib/libkern/libkern.h"

#include "../include/pte.h"
#include "../include/rpb.h"
#include "../include/sid.h"
#define VAX780 1
#include "../include/ka750.h"

#include "../mba/mbareg.h"
#include "../mba/hpreg.h"

#include "vaxstand.h"

/*
 * These routines for HP disk standalone boot is wery simple,
 * assuming a lots of thing like that we only working at one hp disk
 * a time, no separate routines for mba driver etc..
 * But it works :)
 */

static struct disklabel hplabel;
static char io_buf[DEV_BSIZE];
static volatile struct mba_regs *mr;
static volatile struct hp_drv *hd;
static int dpart;

int
hpopen(struct open_file *f, int adapt, int ctlr, int unit, int part)
{
	char *msg;
	int i, err;

	if (askname == 0) { /* Take info from RPB */
		mr = (void *)bootrpb.adpphy;
		hd = (void *)&mr->mba_md[bootrpb.unit];
	} else {
		mr = (void *)nexaddr;
		hd = (void *)&mr->mba_md[unit];
		bootrpb.adpphy = (int)mr;
		bootrpb.unit = unit;
	}
	bzero(&hplabel, sizeof(struct disklabel));

	hplabel.d_secpercyl = 32;
	hplabel.d_nsectors = 32;

	/* Set volume valid and 16 bit format; only done once */
	mr->mba_cr = MBACR_INIT;
	hd->hp_cs1 = HPCS_PA;
	hd->hp_of = HPOF_FMT;

	err = hpstrategy(0, F_READ, LABELSECTOR, DEV_BSIZE, io_buf, &i);
	if (err) {
		printf("reading disklabel: %s\n", strerror(err));
		return 0;
	}

	msg = getdisklabel(io_buf + LABELOFFSET, &hplabel);
	if (msg)
		printf("getdisklabel: %s\n", msg);
	return 0;
}

int
hpstrategy(void *f, int func, daddr_t dblk,
    size_t size, void *buf, size_t *rsize)
{
	unsigned int pfnum, mapnr, nsize, bn, cn, sn, tn;

	pfnum = (u_int)buf >> VAX_PGSHIFT;

	for(mapnr = 0, nsize = size; (nsize + VAX_NBPG) > 0; nsize -= VAX_NBPG)
		*(int *)&mr->mba_map[mapnr++] = PG_V | pfnum++;

	mr->mba_var = ((u_int)buf & VAX_PGOFSET);
	mr->mba_bc = (~size) + 1;
	bn = dblk + hplabel.d_partitions[dpart].p_offset;

	if (bn) {
		cn = bn / hplabel.d_secpercyl;
		sn = bn % hplabel.d_secpercyl;
		tn = sn / hplabel.d_nsectors;
		sn = sn % hplabel.d_nsectors;
	} else
		cn = sn = tn = 0;

	hd->hp_dc = cn;
	hd->hp_da = (tn << 8) | sn;
#ifdef notdef
	if (func == F_WRITE)
		hd->hp_cs1 = HPCS_WRITE;
	else
#endif
		hd->hp_cs1 = HPCS_READ;

	while (mr->mba_sr & MBASR_DTBUSY)
		;

	if (mr->mba_sr & MBACR_ABORT)
		return 1;

	*rsize = size;
	return 0;
}
