/*	$NetBSD: hp.c,v 1.1 1995/02/13 00:41:11 ragge Exp $ */
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
#include "lib/libsa/stand.h"
#include "sys/disklabel.h"
#include "vaxstand.h"
#include "../mba/mbareg.h"
#include "../include/pte.h"
#include "../include/macros.h"

/*
 * These routines for HP disk standalone boot is wery simple,
 * assuming a lots of thing like that we only working at one hp disk
 * a time, no separate routines for mba driver etc..
 * But it works :)
 */

struct	hp_softc {
	int adapt;
	int ctlr;
	int unit;
	int part;
};
struct	disklabel hplabel;
struct	hp_softc hp_softc;
char io_buf[MAXBSIZE];

hpopen(f, adapt, ctlr, unit, part)
	struct open_file *f;
        int ctlr, unit, part;
{
	char *msg;
	struct disklabel *lp=&hplabel;
	struct hp_softc *hs=&hp_softc;
	volatile struct mba_regs *mr=(void *)mbaaddr[ctlr];
	volatile struct hp_drv *hd=&mr->hp_drv[unit];
	int i,err;

	if(adapt>nsbi) return(EADAPT);
	if(ctlr>nmba) return(ECTLR);
	if(unit>MAXMBAU) return(EUNIT);
	bzero(lp, sizeof(struct disklabel));
	lp->d_secpercyl=32;
	lp->d_nsectors=32;
	hs->adapt=adapt;
	hs->ctlr=ctlr;
	hs->unit=unit;
	hs->part=part;
	/* Set volume valid and 16 bit format; only done once */
	mr->mba_cr=MBACR_INIT;
	hd->hp_cs1=HPCS_PA;
	hd->hp_of=HPOF_FMT;

	err=hpstrategy(hs,F_READ, LABELSECTOR, DEV_BSIZE, io_buf, &i);
	if(err){
		printf("reading disklabel: %s\n",strerror(err));
		return 0;
	}

	msg=getdisklabel(io_buf+LABELOFFSET, lp);
	if(msg)printf("getdisklabel: %s\n",msg);
	f->f_devdata=(void *)hs;
	return(0);
}

hpstrategy(hs, func, dblk, size, buf, rsize)
	struct hp_softc *hs;
	int func;
	daddr_t	dblk;
	char *buf;
	u_int size, *rsize;
{
	u_int i,pfnum, mapnr, nsize, bn, cn, sn, tn;
	volatile struct mba_regs *mr=(void *)mbaaddr[hs->ctlr];
	volatile struct hp_drv *hd=&mr->hp_drv[hs->unit];
	struct disklabel *lp=&hplabel;

	pfnum=(u_int)buf>>PGSHIFT;

	for(mapnr=0, nsize=size;(nsize+NBPG)>0;nsize-=NBPG)
		mr->mba_map[mapnr++]=PG_V|pfnum++;

	mr->mba_var=((u_int)buf&PGOFSET);
	mr->mba_bc=(~size)+1;
	bn=dblk;
	cn=bn / lp->d_secpercyl;
	sn=bn % lp->d_secpercyl;
	tn=sn / lp->d_nsectors;
	sn=sn % lp->d_nsectors;
	hd->hp_dc=cn;
	hd->hp_da=(tn<<8)|sn;
	hd->hp_cs1=HPCS_READ;

	while(mr->mba_sr&MBASR_DTBUSY);

	if(mr->mba_sr&MBACR_ABORT) return 1;
	
	*rsize=size;

	return 0;
}
