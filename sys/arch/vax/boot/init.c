/* $NetBSD: init.c,v 1.2 1995/04/25 14:14:28 ragge Exp $ */
/*
 * Copyright (c) 1995 Ludd, University of Lule}, Sweden. All rights reserved.
 * 
 * This code is derived from software contributed to Ludd by Bertram Barth.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution. 3. All advertising
 * materials mentioning features or use of this software must display the
 * following acknowledgement: This product includes software developed at
 * Ludd, University of Lule}, Sweden and its contributors. 4. The name of the
 * author may not be used to endorse or promote products derived from this
 * software without specific prior written permission
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* All bugs are subject to removal without further notice */



#include "lib/libsa/stand.h"

#include "../include/mtpr.h"	/* mfpr(), mtpr() */
#include "../include/sid.h"	/* cpu_type, cpu_number */

#define NRSP 0			/* Kludge, must be done before udareg.h
				 * includes */
#define NCMD 0			/* Kludge */
#include "../uba/udareg.h"	/* struct udadevice */

#include "data.h"		/* bootregs[], rpb, bqo */

int             bootregs[16];
struct rpb     *rpb;
struct bqo     *bqo;

/*
 * initData() sets up data gotten from start routines, mostly for uVAX.
 */
int
initData()
{
	int            *saved_regs;
	int            *tmp;
	int             i;

	saved_regs = (void *) (RELOC);
	saved_regs -= 12;

	for (i = 0; i < 12; i++)
		bootregs[i] = saved_regs[i];

	cpu_type = mfpr(PR_SID);
	cpunumber = (mfpr(PR_SID) >> 24) & 0xFF;

	/*
	 * for MicroVAXen (KA630, KA410, KA650, ...) we need to relocate rpb
	 * and iovec.
	 */
	if (cpunumber == VAX_78032 || cpunumber == VAX_650) {
		int             cpu_sie;	/* sid-extension */
		int             pfn_size;	/* size of bitmap */
		unsigned char  *pfn_bm;		/* the pfn-bitmap */
		int             ntop = 0;	/* top of good memory */

		cpu_sie = *((int *) 0x20040004) >> 24;
		cpu_type |= cpu_sie;

		rpb = (void *) bootregs[11];	/* get old rpb */

		pfn_size = rpb->pfnmap[0];
		pfn_bm = (void *) rpb->pfnmap[1];

		while (pfn_size--) {
			if (*pfn_bm++ != 0xFF)	/* count contigous good */
				break;		/* memory */

			ntop += (8 * 512);	/* 8 pages are coded in 1
						 * byte */
		}

		ntop = relocate(rpb->iovec, rpb->iovecsz, ntop);
		bqo = (void *) ntop;	/* new address of iovec */

		ntop = relocate(rpb, 512, ntop);
		rpb = (void *) ntop;	/* new address of rpb */
		rpb->iovec = (int) bqo;	/* update iovec in new rpb */
		bootregs[11] = (int) rpb;	/* update r11's value */

		/*
		 * bootregs 0..5 are stored in rpb. Since we want/need to use
		 * them we must copy them from rpb->bootrX into bootregs[X].
		 * Don\'t forget to update howto/boothowto from value of R5
		 */
		tmp = (int *) &(rpb->bootr0);
		for (i = 0; i < 6; i++)
			bootregs[i] = tmp[i];
	}
	return (0);
}



int
initCtrl()
{
	register int    (*init) (void);	/* ROM-based initialization routine */
	struct udadevice *up;	/* IP-register / SA-register */
	int res = 1;


	if (bqo->unit_init) {
		init = (void *) bqo + bqo->unit_init;
		if (rpb->devtyp == 17 ||	/* BTD$K_UDA = 17; */
		    rpb->devtyp == 18) {	/* BTD$K_TK50 = 18; */
			/*
			 * writing into the base-address of the controller
			 * (IP Register) starts initialisation of the RQDX3
			 * controller module ...
			 */
			up = (void *) rpb->csrphy;
			up->udaip = 0;

			/*
			 * reading the SA register gives the status. wait
			 * until initialisation started or error (???)
			 */
			while (1) {
				res = up->udasa;
				printf("status: 0x%x (%d)\n", res, res);
				if ((res & (UDA_STEP1 | UDA_STEP2 | UDA_STEP3 |
				    UDA_STEP4 | UDA_ERR)) != 0)
					break;
			}
			/*
			 * RQDX3:  0x0B40      (2880) TQK50:  0x0BC0
			 * (3008)
			 */
		}
		printf("init() ... ");
		res = init();	/* low bit clear indicates error */
		printf("done. (0x%x)\n", res);
	}
	return (res);
}
