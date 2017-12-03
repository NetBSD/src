/*	$NetBSD: ka410.c,v 1.33.18.1 2017/12/03 11:36:48 jdolecek Exp $ */
/*
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Ludd by Bertram Barth.
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
__KERNEL_RCSID(0, "$NetBSD: ka410.c,v 1.33.18.1 2017/12/03 11:36:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/sid.h>
#include <machine/nexus.h>
#include <machine/ka410.h>
#include <machine/ka420.h>
#include <machine/clock.h>
#include <machine/vsbus.h>

static	void	ka410_conf(void);
static	void	ka410_memerr(void);
static	int	ka410_mchk(void *);
static	void	ka410_halt(void);
static	void	ka410_reboot(int);
static	void	ka41_cache_enable(void);
static	void	ka410_clrf(void);

static	const char * const ka410_devs[] = { "cpu", "vsbus", NULL };

static	void *	l2cache;	/* mapped in address */
static	long 	*cacr;		/* l2csche ctlr reg */

/* 
 * Declaration of 410-specific calls.
 */
const struct cpu_dep ka410_calls = {
	.cpu_mchk	= ka410_mchk,
	.cpu_memerr	= ka410_memerr, 
	.cpu_conf	= ka410_conf,
	.cpu_gettime	= chip_gettime,
	.cpu_settime	= chip_settime,
	.cpu_vups	= 1,	/* ~VUPS */
	.cpu_scbsz	= 2,	/* SCB pages */
	.cpu_halt	= ka410_halt,
	.cpu_reboot	= ka410_reboot,
	.cpu_clrf	= ka410_clrf,
	.cpu_devs	= ka410_devs,
	.cpu_flags	= CPU_RAISEIPL,
};


void
ka410_conf(void)
{
	struct cpu_info * const ci = curcpu();
	struct vs_cpu *ka410_cpu;

	ka410_cpu = (struct vs_cpu *)vax_map_physmem(VS_REGS, 1);

	switch (vax_cputype) {
	case VAX_TYP_UV2:
		ka410_cpu->vc_410mser = 1;
		ci->ci_cpustr = "KA410, UV2";
		break;

	case VAX_TYP_CVAX:
		ka410_cpu->vc_vdcorg = 0; /* XXX */
		ka410_cpu->vc_parctl = PARCTL_CPEN | PARCTL_DPEN ;
mtpr(KA420_CADR_S2E|KA420_CADR_S1E|KA420_CADR_ISE|KA420_CADR_DSE, PR_CADR);
		if (vax_confdata & KA420_CFG_CACHPR) {
			l2cache = (void *)vax_map_physmem(KA420_CH2_BASE,
			    (KA420_CH2_SIZE / VAX_NBPG));
			cacr = (void *)vax_map_physmem(KA420_CACR, 1);
			ka41_cache_enable();
			ci->ci_cpustr =
			    "KA420, CVAX, 1KB L1 cache, 64KB L2 cache";
		} else {
			ci->ci_cpustr = "KA420, CVAX, 1KB L1 cache";
		}
	}
	/* Done with ka410_cpu - release it */
	vax_unmap_physmem((vaddr_t)ka410_cpu, 1);
	/*
	 * Setup parameters necessary to read time from clock chip.
	 */
	clk_adrshift = 1;       /* Addressed at long's... */
	clk_tweak = 2;          /* ...and shift two */
	clk_page = (short *)vax_map_physmem(KA420_WAT_BASE, 1);
}

void
ka41_cache_enable(void)
{
	*cacr = KA420_CACR_TPE; 	/* Clear any error, disable cache */
	memset(l2cache, 0, KA420_CH2_SIZE); /* Clear whole cache */
	*cacr = KA420_CACR_CEN;		/* Enable cache */
}

void
ka410_memerr(void)
{
	printf("Memory err!\n");
}

int
ka410_mchk(void *addr)
{
	panic("Machine check");
	return 0;
}

static void
ka410_halt(void)
{
	__asm("movl $0xc, (%0)"::"r"((int)clk_page + 0x38)); /* Don't ask */
	__asm("halt");
}

static void
ka410_reboot(int arg)
{
	__asm("movl $0xc, (%0)"::"r"((int)clk_page + 0x38)); /* Don't ask */
	__asm("halt");
}

static void
ka410_clrf(void)
{
	volatile struct ka410_clock *clk = (volatile void *)clk_page;

	/*
	 * Clear restart and boot in progress flags
	 * in the CPMBX. (ie. clear bits 4 and 5)
	 */
	clk->cpmbx = (clk->cpmbx & ~0x30);
}
