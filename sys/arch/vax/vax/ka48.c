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

/*** needs to be completed MK-990306 ***/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ka48.c,v 1.21.36.1 2017/08/28 17:51:55 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/sid.h>
#include <machine/nexus.h>
#include <machine/ka410.h>
#include <machine/ka420.h>
#include <machine/ka48.h>
#include <machine/clock.h>
#include <machine/vsbus.h>

static	void	ka48_conf(void);
static	void	ka48_steal_pages(void);
static	void	ka48_memerr(void);
static	int	ka48_mchk(void *);
static	void	ka48_halt(void);
static	void	ka48_reboot(int);
static	void	ka48_cache_enable(void);

struct	vs_cpu *ka48_cpu;

static const char * const ka48_devs[] = { "cpu", "vsbus", NULL };

/* 
 * Declaration of 48-specific calls.
 */
const struct cpu_dep ka48_calls = {
	.cpu_steal_pages = ka48_steal_pages,
	.cpu_mchk	= ka48_mchk,
	.cpu_memerr	= ka48_memerr, 
	.cpu_conf	= ka48_conf,
	.cpu_gettime	= chip_gettime,
	.cpu_settime	= chip_settime,
	.cpu_vups	= 6,	/* ~VUPS */
	.cpu_scbsz	= 2,	/* SCB pages */
	.cpu_halt	= ka48_halt,
	.cpu_reboot	= ka48_reboot,
	.cpu_devs	= ka48_devs,
	.cpu_flags	= CPU_RAISEIPL,
};


void
ka48_conf(void)
{
	curcpu()->ci_cpustr = "KA48, SOC, 6KB L1 cache";

	ka48_cpu = (void *)vax_map_physmem(VS_REGS, 1);
	mtpr(2, PR_ACCS); /* Enable floating points */
	/*
	 * Setup parameters necessary to read time from clock chip.
	 */
	clk_adrshift = 1;       /* Addressed at long's... */
	clk_tweak = 2;          /* ...and shift two */
	clk_page = (short *)vax_map_physmem(VS_CLOCK, 1);
}

void
ka48_cache_enable(void)
{
	int i, *tmp;
	long *par_ctl = (long *)KA48_PARCTL;

	/* Disable cache */
	mtpr(0, PR_CADR);		/* disable */
	*par_ctl &= ~KA48_PARCTL_INVENA;	/* clear ? invalid enable */
	mtpr(2, PR_CADR);		/* flush */

	/* Clear caches */
	tmp = (void *)KA48_INVFLT;	/* inv filter */
	for (i = 0; i < KA48_INVFLTSZ / sizeof(int); i++)
		tmp[i] = 0;
	*par_ctl |= KA48_PARCTL_INVENA;	/* Enable ???? */
	mtpr(4, PR_CADR);		/* enable cache */
	*par_ctl |= (KA48_PARCTL_AGS |	/* AGS? */
	    KA48_PARCTL_NPEN |		/* N? Parity Enable */
	    KA48_PARCTL_CPEN);		/* CPU parity enable */
}

void
ka48_memerr(void)
{
	printf("Memory err!\n");
}

int
ka48_mchk(void *addr)
{
	panic("Machine check");
	return 0;
}

void
ka48_steal_pages(void)
{
	/* Turn on caches (to speed up execution a bit) */
	ka48_cache_enable();
}

#define	KA48_CPMBX	0x38
#define	KA48_HLT_HALT	0xcf	/* 11001111 */
#define	KA48_HLT_BOOT	0x8b	/* 10001011 */

void
ka48_halt(void)
{
	((volatile uint8_t *) clk_page)[KA48_CPMBX] = KA48_HLT_HALT;
	__asm("halt");
}

void
ka48_reboot(int arg)
{
	((volatile uint8_t *) clk_page)[KA48_CPMBX] = KA48_HLT_BOOT;
	__asm("halt");
}
