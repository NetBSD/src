/*	$NetBSD: cpuconf.c,v 1.6.2.3 1997/09/29 07:19:33 thorpej Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <machine/cpuconf.h>
#include <machine/rpb.h>

#include "dec_3000_500.h"
#if	NDEC_3000_500 > 0
extern void dec_3000_500_init __P((void));
#else
#define	dec_3000_500_init	nocpu
#endif

#include "dec_3000_300.h"
#if	NDEC_3000_300 > 0
extern void dec_3000_300_init __P((void));
#else
#define	dec_3000_300_init	nocpu
#endif

#include "dec_axppci_33.h"
#if	NDEC_AXPPCI_33 > 0
extern void dec_axppci_33_init __P((void));
#else
#define	dec_axppci_33_init	nocpu
#endif

#include "dec_kn8ae.h"
#if	NDEC_KN8AE > 0
extern void dec_kn8ae_init __P((void));
#else
#define	dec_kn8ae_init		nocpu
#endif

#include "dec_2100_a50.h"
#if	NDEC_2100_A50 > 0
extern void dec_2100_a50_init __P((void));
#else
#define	dec_2100_a50_init	nocpu
#endif

#include "dec_kn20aa.h"
#if	NDEC_KN20AA > 0
extern void dec_kn20aa_init __P((void));
#else
#define	dec_kn20aa_init		nocpu
#endif

#include "dec_eb64plus.h"
#if	NDEC_EB64PLUS > 0
extern void dec_eb64plus_init __P((void));
#else
#define	dec_eb64plus_init	nocpu
#endif

#include "dec_eb164.h"
#if	NDEC_EB164 > 0
extern void dec_eb164_init __P((void));
#else
#define	dec_eb164_init		nocpu
#endif

void (*cpuinit[]) __P((void)) = {
	nocpu,				/*  0: ??? */
	nocpu,				/*  1: ST_ADU */
	nocpu,				/*  2: ST_DEC_4000 */
	nocpu,				/*  3: ST_DEC_7000 */
	dec_3000_500_init,		/*  4: ST_DEC_3000_500 */
	nocpu,				/*  5: ??? */
	nocpu,				/*  6: ST_DEC_2000_300 */
	dec_3000_300_init,		/*  7: ST_DEC_3000_300 */
	nocpu,				/*  8: ??? */
	nocpu,				/*  9: ST_DEC_2100_A500 */
	nocpu,				/* 10: ST_DEC_APXVME_64 */
	dec_axppci_33_init,		/* 11: ST_DEC_AXPPCI_33 */
	dec_kn8ae_init,			/* 12: ST_DEC_21000 */
	dec_2100_a50_init,		/* 13: ST_DEC_2100_A50 */
	nocpu,				/* 14: ST_DEC_MUSTANG */
	dec_kn20aa_init,		/* 15: ST_DEC_KN20AA */
	nocpu,				/* 16: ??? */
	nocpu,				/* 17: ST_DEC_1000 */
	nocpu,				/* 18: ??? */
	nocpu,				/* 19: ST_EB66 */
	dec_eb64plus_init,		/* 20: ST_EB64P */
	nocpu,				/* 21: ??? */
	nocpu,				/* 22: ST_DEC_4100 */
	nocpu,				/* 23: ST_DEC_EV45_PBP */
	nocpu,				/* 24: ST_DEC_2100A_A500 */
	nocpu,				/* 25: ??? */
	dec_eb164_init			/* 26: ST_EB164 */
};
int ncpuinit = (sizeof (cpuinit) / sizeof (cpuinit[0]));

void
nocpu()
{
	extern int cputype;
	printf("\n");
	printf("Support for system type %d is not present in this kernel.\n",
	    cputype);
	if (unknown_cpu(cputype)) {
		printf("NetBSD doesn't support this platform yet.\n");
	} else {
		char *o;
		switch (cputype) {
		case ST_DEC_3000_500:
			o = "DEC_3000_500";
			break;
		case ST_DEC_3000_300:
			o = "DEC_3000_300";
			break;
		case ST_DEC_AXPPCI_33:
			o = "DEC_AXPPCI_33";
			break;
		case ST_DEC_21000:
			o = "DEC_KN8AE";
			break;
		case ST_DEC_2100_A50:
			o = "DEC_2100_A50";
			break;
		case ST_DEC_KN20AA:
			o = "DEC_KN20AA";
			break;
		case ST_EB64P:
			o = "DEC_EB64PLUS";
			break;
		case ST_EB164:
			o = "DEC_EB164";
			break;
		default:
			o = "JESUS_KNOWS";
			break;
		}
		printf("Build a kernel with \"options %s\" and reboot.\n", o);
	}
	printf("\n");   
	panic("support for system not present");
	/* NOTREACHED */
}
