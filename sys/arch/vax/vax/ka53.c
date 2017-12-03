/*	$NetBSD: ka53.c,v 1.16.18.1 2017/12/03 11:36:48 jdolecek Exp $	*/
/*
 * Copyright (c) 2002 Hugh Graham.
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
 * 3. The name of the author may not be used to endorse or promote products
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

/*
 * Done by Michael Kukat (michael@unixiron.org)
 * Thanx for the good cooperation with Hugh Graham (hugh@openbsd.org)
 * and his useful hints for the console of these machines
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ka53.c,v 1.16.18.1 2017/12/03 11:36:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/clock.h>
#include <machine/scb.h>
#include <machine/sid.h>

static void    ka53_conf(void);
static void    ka53_memerr(void);
static int     ka53_mchk(void *);
static void    ka53_softmem(void *);
static void    ka53_hardmem(void *);
static void    ka53_steal_pages(void);
static void    ka53_cache_enable(void);
static void    ka53_attach_cpu(device_t);

static const char * const ka53_devs[] = { "cpu", "sgec", "vsbus", "uba", NULL };

/* 
 * Declaration of 53-specific calls.
 */
const struct cpu_dep ka53_calls = {
	.cpu_steal_pages = ka53_steal_pages,
	.cpu_mchk	= ka53_mchk,
	.cpu_memerr	= ka53_memerr, 
	.cpu_conf	= ka53_conf,
	.cpu_gettime	= generic_gettime,
	.cpu_settime	= generic_settime,
	.cpu_vups	= 32,	 /* ~VUPS */
	.cpu_scbsz	= 2,	/* SCB pages */
	.cpu_halt	= generic_halt,
	.cpu_reboot	= generic_reboot,
	.cpu_flags	= CPU_RAISEIPL,
	.cpu_devs	= ka53_devs,
	.cpu_attach_cpu	= ka53_attach_cpu,
};

void
ka53_conf(void)
{

	/* Don't ask why, but we seem to need this... */
	volatile int *hej = (void *)mfpr(PR_ISP);
	*hej = *hej;
	hej[-1] = hej[-1];

	cpmbx = (struct cpmbx *)vax_map_physmem(0x20140400, 1);
}

void
ka53_attach_cpu(device_t self)
{
	const char *cpuname;

	switch((vax_siedata & 0xff00) >> 8) {
		case VAX_STYP_51: cpuname = "KA51"; break;
		case VAX_STYP_52: cpuname = "KA52"; break;
		case VAX_STYP_53: cpuname = "KA53,54,57"; break;
		case VAX_STYP_55: cpuname = "KA55"; break;
		default: cpuname = "unknown NVAX";
	}
	printf("cpu0: %s, ucode rev %d\n", cpuname, vax_cpudata & 0xff);
}

/*
 * Why may we get memory errors during startup???
 */

void
ka53_hardmem(void *arg)
{
	if (cold == 0)
		printf("Hard memory error\n");
	splhigh();
}

void
ka53_softmem(void *arg)
{
	if (cold == 0)
		printf("Soft memory error\n");
	splhigh();
}

/*
 * KA53-specific IPRs. KA53 has the funny habit to control all caches
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
ka53_cache_enable(void)
{
	int start, slut;

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
	__asm("movpsl -(%sp); movab 1f,-(%sp); rei; 1:;");

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
ka53_memerr(void)
{
	printf("Memory err!\n");
}

int
ka53_mchk(void *addr)
{
	mtpr(0x00, PR_MCESR);
	printf("Machine Check\n");
	return 0;
}

void
ka53_steal_pages(void)
{

	/*
	 * Get the soft and hard memory error vectors now.
	 */

	scb_vecalloc(0x54, ka53_softmem, NULL, 0, NULL);
	scb_vecalloc(0x60, ka53_hardmem, NULL, 0, NULL);


	/* Turn on caches (to speed up execution a bit) */
	ka53_cache_enable();
}
