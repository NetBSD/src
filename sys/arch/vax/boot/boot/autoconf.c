/*	$NetBSD: autoconf.c,v 1.28.40.1 2017/08/28 17:51:54 skrll Exp $ */
/*
 * Copyright (c) 1994, 1998 Ludd, University of Lule}, Sweden.
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

 /* All bugs are subject to removal without further notice */
		


#include <sys/param.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>

#include "../include/mtpr.h"
#include "../include/sid.h"
#include "../include/intr.h"
#include "../include/rpb.h"
#include "../include/scb.h"

#include "vaxstand.h"

void autoconf(void);
void findcpu(void);
void consinit(void);
void scbinit(void);
void scb_stray(void *);
void longjmp(int *, int);
void rtimer(void *);

long *bootregs;

/*
 * Do some initial setup. Also create a fake RPB for net-booted machines
 * that don't have an in-prom VMB.
 */

void
autoconf(void)
{
	int copyrpb = 1;
	int fromnet = (bootregs[12] != -1);

	findcpu(); /* Configures CPU variables */
	consinit(); /* Allow us to print out things */
	scbinit(); /* Fix interval clock etc */

#ifdef DEV_DEBUG
	printf("Register contents:\n");
	for (copyrpb = 0; copyrpb < 13; copyrpb++)
		printf("r%d: %lx\n", copyrpb, bootregs[copyrpb]);
#endif
	switch (vax_boardtype) {

	case VAX_BTYP_780:
	case VAX_BTYP_790:
	case VAX_BTYP_8000:
	case VAX_BTYP_9CC:
	case VAX_BTYP_9RR:
	case VAX_BTYP_1202:
		if (fromnet == 0)
			break;
		copyrpb = 0;
		bootrpb.devtyp = bootregs[0];
		bootrpb.adpphy = bootregs[1];
		bootrpb.csrphy = bootregs[2];
		bootrpb.unit = bootregs[3];
		bootrpb.rpb_bootr5 = bootregs[5];
		bootrpb.pfncnt = 0;
		break;

	case VAX_BTYP_46:
	case VAX_BTYP_48:
		{int *map, i;

		/* Map all 16MB of I/O space to low 16MB of memory */
		map = (int *)0x700000; /* XXX */
		*(int *)0x20080008 = (int)map; /* XXX */
		for (i = 0; i < 0x8000; i++)
			map[i] = 0x80000000 | i;
		}break;

		break;
	}

	if (copyrpb) {
		struct rpb *prpb = (struct rpb *)bootregs[11];
		memcpy(&bootrpb, (void *)prpb, sizeof(struct rpb));
		if (prpb->iovec) {
			bootrpb.iovec = (int)alloc(prpb->iovecsz);
			memcpy((void *)bootrpb.iovec, (void *)prpb->iovec,
			    prpb->iovecsz);
		}
	}
}

/*
 * Clock handling routines, needed to do timing in standalone programs.
 */

volatile int tickcnt;

satime_t
getsecs(void)
{
	return tickcnt/100;
}

struct ivec_dsp **scb;
struct ivec_dsp *scb_vec;
extern struct ivec_dsp idsptch;
extern int jbuf[10];

static void
mcheck(void *arg)
{
	int off, *mfp = (int *)(void *)&arg;

	off = (mfp[7]/4 + 8);
	printf("Machine check, pc=%x, psl=%x\n", mfp[off], mfp[off+1]);
	longjmp(jbuf, 1);
}

/*
 * Init the SCB and set up a handler for all vectors in the lower space,
 * to detect unwanted interrupts.
 */
void
scbinit(void)
{
	int i, addr;

	/*
	 * Allocate space. We need one page for the SCB, and 128*20 == 2.5k
	 * for the vectors. The SCB must be on a page boundary.
	 */
	i = (int)alloc(VAX_NBPG + 128*sizeof(scb_vec[0])) + VAX_PGOFSET;
	i &= ~VAX_PGOFSET;

	mtpr(i, PR_SCBB);
	scb = (void *)i;
	scb_vec = (struct ivec_dsp *)(i + VAX_NBPG);

	for (i = 0; i < 128; i++) {
		scb[i] = &scb_vec[i];
		addr = (int)scb[i];
		addr |= SCB_ISTACK;		/* Only interrupt stack */
		scb[i] = (struct ivec_dsp*)addr;
		scb_vec[i] = idsptch;
		scb_vec[i].hoppaddr = scb_stray;
		scb_vec[i].pushlarg = (void *) (i * 4);
		scb_vec[i].ev = NULL;
	}
	scb_vec[0xc0/4].hoppaddr = rtimer;
	scb_vec[4/4].hoppaddr = mcheck;

	if (vax_boardtype != VAX_BTYP_VXT)
		mtpr(-10000, PR_NICR);		/* Load in count register */
	mtpr(0x800000d1, PR_ICCS);	/* Start clock and enable interrupt */

	mtpr(20, PR_IPL);
}

extern int sluttid, senast, skip;

void
rtimer(void *arg)
{
	mtpr(IPL_HIGH, PR_IPL);
	tickcnt++;
	mtpr(0xc1, PR_ICCS);
	if (skip)
		return;
	if ((vax_boardtype == VAX_BTYP_46) ||
	    (vax_boardtype == VAX_BTYP_48) ||
	    (vax_boardtype == VAX_BTYP_49)) {
		int nu = sluttid - getsecs();
		if (senast != nu) {
			mtpr(20, PR_IPL);
			longjmp(jbuf, 1);
		}
	}
}

#ifdef __ELF__
#define	IDSPTCH "idsptch"
#define	EIDSPTCH "eidsptch"
#define	CMN_IDSPTCH "cmn_idsptch"
#else
#define	IDSPTCH "_idsptch"
#define	EIDSPTCH "_eidsptch"
#define	CMN_IDSPTCH "_cmn_idsptch"
#endif

__asm(
"	.text;"
"	.align	2;"
"	.globl  " IDSPTCH ", " EIDSPTCH ";"
IDSPTCH ":;"
"	pushr   $0x3f;"
"	.word	0x9f16;"
"	.long   " CMN_IDSPTCH ";"
"	.long	0;"
"	.long	0;"
"	.long	0;"
EIDSPTCH ":;"

CMN_IDSPTCH ":;"
"	movl	(%sp)+,%r0;"
"	pushl	4(%r0);"
"	calls	$1,*(%r0);"
"	popr	$0x3f;"
"	rei;"
);

/*
 * Stray interrupt handler.
 * This function must _not_ save any registers (in the reg save mask).
 */
void
scb_stray(void *arg)
{
	static int vector, ipl;

	ipl = mfpr(PR_IPL);
	vector = (int) arg;
	printf("stray interrupt: vector 0x%x, ipl %d\n", vector, ipl);
}
