/*	$NetBSD: ka680.c,v 1.2 2000/06/04 02:19:27 matt Exp $	*/
/*
 * Copyright (c) 2000 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of 
 *     Lule}, Sweden and its contributors.
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
#include <sys/kernel.h>

#include <machine/clock.h>
#include <machine/cpu.h>
#include <machine/scb.h>
#include <machine/sid.h>
#include <machine/mtpr.h>

static void    ka680_conf(void);
static void    ka680_memerr(void);
static int     ka680_mchk(caddr_t);
static void    ka680_halt(void);
static void    ka680_reboot(int);
#ifdef notyet
static void    ka680_softmem(int);
static void    ka680_hardmem(int);
#endif
static void    ka680_steal_pages(void);
static void    ka680_cache_enable(void);
static void    ka680_halt(void);

/* 
 * Declaration of 680-specific calls.
 */
struct cpu_dep ka680_calls = {
	ka680_steal_pages,
	ka680_mchk,
	ka680_memerr, 
	ka680_conf,
	generic_clkread,
	generic_clkwrite,
	24,	 /* ~VUPS */
	2,	/* SCB pages */
	ka680_halt,
	ka680_reboot,
};


void
ka680_conf()
{
	printf("cpu0: KA680, ucode rev %d\n", vax_cpudata & 0xff);
}

/*
 * Why may we get memory errors during startup???
 */

#ifdef notyet
void
ka680_hardmem(int arg)
{
	if (cold == 0)
		printf("Hard memory error\n");
	splhigh();
}

void
ka680_softmem(int arg)
{
	if (cold == 0)
		printf("Soft memory error\n");
	splhigh();
}
#endif

/*
 * KA680-specific IPRs. KA680 has the funny habit to control all caches
 * via IPRs.
 */
#define PR_CCTL	 0xa0
#define CCTL_ENABLE	0x00000001
#define CCTL_SW_ETM	0x40000000
#define CCTL_HW_ETM	0x80000000

#define PR_BCETSTS	0xa3
#define PR_BCEDSTS	0xa6
#define PR_NESTS	0xae

#define PR_VMAR	 0xd0
#define PR_VTAG	 0xd1
#define PR_ICSR	 0xd3
#define ICSR_ENABLE	0x01

#define PR_PCCTL	0xf8
#define PCCTL_P_EN	0x10
#define PCCTL_I_EN	0x02
#define PCCTL_D_EN	0x01

void
ka680_cache_enable()
{
	int start, slut;

	return;

	/*
	 * Turn caches off.
	 */
	mtpr(0, PR_ICSR);
	mtpr(0, PR_PCCTL);
	mtpr(mfpr(PR_CCTL) | CCTL_SW_ETM, PR_CCTL);

	/*
	 * Invalidate caches.
	 */
	mtpr(mfpr(PR_CCTL) | 6, PR_CCTL);	/* Set cache size and speed */
	mtpr(mfpr(PR_BCETSTS), PR_BCETSTS);	/* Clear error bits */
	mtpr(mfpr(PR_BCEDSTS), PR_BCEDSTS);	/* Clear error bits */
	mtpr(mfpr(PR_NESTS), PR_NESTS);	 /* Clear error bits */


	start = 0x01400000;
	slut  = 0x01420000;

	/* Flush cache lines */
	for (; start < slut; start += 0x20)
		mtpr(0, start);

	mtpr((mfpr(PR_CCTL) & ~(CCTL_SW_ETM|CCTL_ENABLE)) | CCTL_HW_ETM,
	    PR_CCTL);

	start = 0x01000000;
	slut  = 0x01020000;

	/* clear tag and valid */
	for (; start < slut; start += 0x20)
		mtpr(0, start);

	mtpr(mfpr(PR_CCTL) | 6 | CCTL_ENABLE, PR_CCTL); /* enab. bcache */

	start = 0x01800000;
	slut  = 0x01802000;

	/* Clear primary cache */
	for (; start < slut; start += 0x20)
		mtpr(0, start);

	/* Flush the pipes (via REI) */
	asm("movpsl -(sp); movab 1f,-(sp); rei; 1:;");

	/* Enable primary cache */
	mtpr(PCCTL_P_EN|PCCTL_I_EN|PCCTL_D_EN, PR_PCCTL);

	/* Enable the VIC */
	start = 0;
	slut  = 0x800;
	for (; start < slut; start += 0x20) {
		mtpr(start, PR_VMAR);
		mtpr(0, PR_VTAG);
	}
	mtpr(ICSR_ENABLE, PR_ICSR);
}

void
ka680_memerr()
{
	printf("Memory err!\n");
}

int
ka680_mchk(caddr_t addr)
{
	mtpr(0x00, PR_MCESR);
	printf("Machine Check\n");
	return 0;
}

void
ka680_steal_pages()
{

	/*
	 * Get the soft and hard memory error vectors now.
	 */
#ifdef notyet
	scb_vecalloc(0x54, ka680_softmem, NULL, 0, NULL);
	scb_vecalloc(0x60, ka680_hardmem, NULL, 0, NULL);
#endif

	/* Turn on caches (to speed up execution a bit) */
	ka680_cache_enable();
}

static void
ka680_halt()
{
	asm("halt");
}

static void
ka680_reboot(int arg)
{
	asm("halt");
}

