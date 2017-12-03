/*	$NetBSD: ka46.c,v 1.25.18.1 2017/12/03 11:36:48 jdolecek Exp $ */
/*
 * Copyright (c) 1998 Ludd, University of Lule}, Sweden.
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
__KERNEL_RCSID(0, "$NetBSD: ka46.c,v 1.25.18.1 2017/12/03 11:36:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/sid.h>
#include <machine/nexus.h>
#include <machine/ka410.h>
#include <machine/ka420.h>
#include <machine/ka46.h>
#include <machine/clock.h>
#include <machine/vsbus.h>

static	void	ka46_conf(void);
static	void	ka46_steal_pages(void);
static	void	ka46_memerr(void);
static	int	ka46_mchk(void *);
static	void	ka46_halt(void);
static	void	ka46_reboot(int);
static	void	ka46_cache_enable(void);

static const char * const ka46_devs[] = { "cpu", "vsbus", NULL };

struct	vs_cpu *ka46_cpu;

/* 
 * Declaration of 46-specific calls.
 */
const struct cpu_dep ka46_calls = {
	.cpu_steal_pages = ka46_steal_pages,
	.cpu_mchk	= ka46_mchk,
	.cpu_memerr	= ka46_memerr, 
	.cpu_conf	= ka46_conf,
	.cpu_gettime	= chip_gettime,
	.cpu_settime	= chip_settime,
	.cpu_vups	= 12,      /* ~VUPS */
	.cpu_scbsz	= 2,	/* SCB pages */
	.cpu_halt	= ka46_halt,
	.cpu_reboot	= ka46_reboot,
	.cpu_devs	= ka46_devs,
	.cpu_flags	= CPU_RAISEIPL,
};

static const char * const ka46_cpustrs[4] = {
	[0] = "unknown KA46 type 0",
	[1] = "KA47, Mariah, 2KB L1 cache, 256KB L2 cache",
	[2] = "KA46, Mariah, 2KB L1 cache, 256KB L2 cache",
	[3] = "unknown KA46 type 3",
};

void
ka46_conf(void)
{
	curcpu()->ci_cpustr = ka46_cpustrs[vax_siedata & 0x3];
	ka46_cpu = (void *)vax_map_physmem(VS_REGS, 1);
	mtpr(2, PR_ACCS); /* Enable floating points */
	/*
	 * Setup parameters necessary to read time from clock chip.
	 */
	clk_adrshift = 1;       /* Addressed at long's... */
	clk_tweak = 2;          /* ...and shift two */
	clk_page = (short *)vax_map_physmem(VS_CLOCK, 1);
}

void
ka46_cache_enable(void)
{
	int i, *tmp;

	/* Disable caches */
	*(int *)KA46_CCR &= ~CCR_SPECIO;/* secondary */
	mtpr(PCSTS_FLUSH, PR_PCSTS);	/* primary */
	*(int *)KA46_BWF0 &= ~BWF0_FEN; /* invalidate filter */

	/* Clear caches */
	tmp = (void *)KA46_INVFLT;	/* inv filter */
	for (i = 0; i < 32768; i++)
		tmp[i] = 0;

	/* Write valid parity to all primary cache entries */
	for (i = 0; i < 256; i++) {
		mtpr(i << 3, PR_PCIDX);
		mtpr(PCTAG_PARITY, PR_PCTAG);
	}

	/* Secondary cache */
	tmp = (void *)KA46_TAGST;
	for (i = 0; i < KA46_TAGSZ*2; i+=2)
		tmp[i] = 0;

	/* Enable cache */
	*(int *)KA46_BWF0 |= BWF0_FEN; /* invalidate filter */
	mtpr(PCSTS_ENABLE, PR_PCSTS);
	*(int *)KA46_CCR = CCR_SPECIO | CCR_CENA;
}

void
ka46_memerr(void)
{
	printf("Memory err!\n");
}

int
ka46_mchk(void *addr)
{
	panic("Machine check");
	return 0;
}

void
ka46_steal_pages(void)
{

	/* Turn on caches (to speed up execution a bit) */
	ka46_cache_enable();
}

#define	KA46_CPMBX	0x38
#define KA46_HLT_HALT	0xcf
#define KA46_HLT_BOOT	0x8b

void
ka46_halt(void)
{
	((volatile uint8_t *) clk_page)[KA46_CPMBX] = KA46_HLT_HALT;
	__asm("halt");
}

void
ka46_reboot(int arg)
{
	((volatile uint8_t *) clk_page)[KA46_CPMBX] = KA46_HLT_BOOT;
	__asm("halt");
}
