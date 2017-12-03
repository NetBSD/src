/*
 * Copyright (c) 1999 Ludd, University of Lule}, Sweden.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ka49.c,v 1.19.2.1 2017/12/03 11:36:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/clock.h>
#include <machine/scb.h>
#include <machine/mainbus.h>

#define	KA49_CPMBX	0x38
#define	KA49_HLT_HALT	0xcf
#define	KA49_HLT_BOOT	0x8b

static	void	ka49_conf(void);
static	void	ka49_memerr(void);
static	int	ka49_mchk(void *);
static	void	ka49_halt(void);
static	void	ka49_reboot(int);
static	void	ka49_softmem(void *);
static	void	ka49_hardmem(void *);
static	void	ka49_steal_pages(void);
static	void	ka49_cache_enable(void);
static	void	ka49_halt(void);

static const char * const ka49_devs[] = { "cpu", "sgec", "vsbus", NULL };

/* 
 * Declaration of 49-specific calls.
 */
const struct cpu_dep ka49_calls = {
	.cpu_steal_pages = ka49_steal_pages,
	.cpu_mchk	= ka49_mchk,
	.cpu_memerr	= ka49_memerr, 
	.cpu_conf	= ka49_conf,
	.cpu_gettime	= chip_gettime,
	.cpu_settime	= chip_settime,
	.cpu_vups	= 32,      /* ~VUPS */
	.cpu_scbsz	= 2,	/* SCB pages */
	.cpu_halt	= ka49_halt,
	.cpu_reboot	= ka49_reboot,
	.cpu_devs	= ka49_devs,
	.cpu_flags	= CPU_RAISEIPL,
};

void
ka49_conf(void)
{
	curcpu()->ci_cpustr = "KA49, NVAX, 10KB L1 cache, 256KB L2 cache";

/* Why??? */
{ volatile int *hej = (void *)mfpr(PR_ISP); *hej = *hej; hej[-1] = hej[-1];}

	/*
	 * Setup parameters necessary to read time from clock chip.
	 */
	clk_adrshift = 1;       /* Addressed at long's... */
	clk_tweak = 2;          /* ...and shift two */
	clk_page = (short *)vax_map_physmem(0x25400000, 1);
}

/*
 * Why may we get memory errors during startup???
 */
void
ka49_hardmem(void *arg)
{
	if (cold == 0)
		printf("Hard memory error\n");
	splhigh();
}

void
ka49_softmem(void *arg)
{
	if (cold == 0)
		printf("Soft memory error\n");
	splhigh();
}

/*
 * KA49-specific IPRs. KA49 has the funny habit to control all caches
 * via IPRs.
 */
#define	PR_CCTL		0xa0
#define	CCTL_ENABLE	0x00000001
#define	CCTL_SSIZE	0x00000002
#define	CCTL_VSIZE	0x00000004
#define	CCTL_SW_ETM	0x40000000
#define	CCTL_HW_ETM	0x80000000

#define	PR_BCETSTS	0xa3
#define	PR_BCEDSTS	0xa6
#define	PR_NESTS	0xae

#define	PR_VMAR		0xd0
#define	PR_VTAG		0xd1
#define	PR_ICSR		0xd3
#define	ICSR_ENABLE	0x01

#define	PR_PCCTL	0xf8
#define	PCCTL_P_EN	0x10
#define	PCCTL_I_EN	0x02
#define	PCCTL_D_EN	0x01

void
ka49_cache_enable(void)
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
	mtpr(mfpr(PR_CCTL) | 0x10, PR_CCTL);	/* Set cache size */
	mtpr(mfpr(PR_BCETSTS), PR_BCETSTS);	/* Clear error bits */
	mtpr(mfpr(PR_BCEDSTS), PR_BCEDSTS);	/* Clear error bits */
	mtpr(mfpr(PR_NESTS), PR_NESTS);		/* Clear error bits */

	start = 0x01400000;
	slut  = 0x01440000;

	/* Flush cache lines */
	for (; start < slut; start += 0x20)
		mtpr(0, start);

	mtpr((mfpr(PR_CCTL) & ~(CCTL_SW_ETM|CCTL_ENABLE)) | CCTL_HW_ETM,
	    PR_CCTL);

	start = 0x01000000;
	slut  = 0x01040000;

	/* clear tag and valid */
	for (; start < slut; start += 0x20)
		mtpr(0, start);

	mtpr(mfpr(PR_CCTL) | 0x10 | CCTL_ENABLE, PR_CCTL); /* enab. bcache */

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
ka49_memerr(void)
{
	printf("Memory err!\n");
}

int
ka49_mchk(void *addr)
{
	panic("Machine check");
	return 0;
}

void
ka49_steal_pages(void)
{
	/*
	 * Get the soft and hard memory error vectors now.
	 */
	scb_vecalloc(0x54, ka49_softmem, NULL, 0, NULL);
	scb_vecalloc(0x60, ka49_hardmem, NULL, 0, NULL);

	/* Turn on caches (to speed up execution a bit) */
	ka49_cache_enable();
}

void
ka49_halt(void)
{
	((volatile uint8_t *) clk_page)[KA49_CPMBX] = KA49_HLT_HALT;
	__asm("halt");
}

void
ka49_reboot(int arg)
{
	((volatile uint8_t *) clk_page)[KA49_CPMBX] = KA49_HLT_BOOT;
	__asm("halt");
}
