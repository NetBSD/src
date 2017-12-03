/*	$NetBSD: ka660.c,v 1.10.18.1 2017/12/03 11:36:48 jdolecek Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: ka660.c,v 1.10.18.1 2017/12/03 11:36:48 jdolecek Exp $");

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

#define KA660_CCR	37		/* Cache Control Register */
#define KA660_CTAG	0x20150000	/* Cache Tags */
#define KA660_CDATA	0x20150400	/* Cache Data */
#define KA660_BEHR	0x20150800	/* Bank Enable/Hit Register */
#define CCR_WWP		8	/* Write Wrong Parity */
#define CCR_ENA		4	/* Cache Enable */
#define CCR_FLU 2	/* Cache Flush */
#define CCR_DIA 1	/* Diagnostic mode */

static void ka660_conf(void);
static void ka660_memerr(void);
static void ka660_cache_enable(void);
static void ka660_attach_cpu(device_t);
static int ka660_mchk(void *);

static const char * const ka660_devs[] = { "cpu", "sgec", "shac", "uba", NULL };

/* 
 * Declaration of 660-specific calls.
 */
const struct cpu_dep ka660_calls = {
	.cpu_steal_pages = ka660_cache_enable,	/* ewww */
	.cpu_mchk	= ka660_mchk,
	.cpu_memerr	= ka660_memerr, 
	.cpu_conf	= ka660_conf,
	.cpu_gettime	= generic_gettime,
	.cpu_settime	= generic_settime,
	.cpu_vups	= 6,	/* ~VUPS */
	.cpu_scbsz	= 2,	/* SCB pages */
	.cpu_halt	= generic_halt,
	.cpu_reboot	= generic_reboot,
	.cpu_devs	= ka660_devs,
	.cpu_attach_cpu	= ka660_attach_cpu,
};


void
ka660_conf(void)
{
	cpmbx = (struct cpmbx *)vax_map_physmem(0x20140400, 1);
}

void
ka660_attach_cpu(device_t self)
{
	aprint_normal(
	    ": %s, SOC (ucode rev. %d), 6KB L1 cache\n",
	    "KA660",
	    vax_cpudata & 0377);
}

void
ka660_cache_enable(void)
{
	unsigned int *p;
	int cnt, bnk, behrtmp;

	mtpr(0, KA660_CCR);	/* Disable cache */
	mtpr(CCR_DIA, KA660_CCR);	/* Switch to diag mode */
	bnk = 1;
	behrtmp = 0;
	while(bnk <= 0x80)
	{
		*(int *)KA660_BEHR = bnk;
		p = (int *)KA660_CDATA;
		*p = 0x55aaff00L;
		if(*p == 0x55aaff00L) behrtmp |= bnk;
		*p = 0xffaa0055L;
		if(*p != 0xffaa0055L) behrtmp &= ~bnk;
		cnt = 256;
		while(cnt--) *p++ = 0L;
		p = (int *) KA660_CTAG;
		cnt =128;
		while(cnt--) { *p++ = 0x80000000L; p++; }
		bnk <<= 1;
	}
	*(int *)KA660_BEHR = behrtmp;

	mtpr(CCR_DIA|CCR_FLU, KA660_CCR);	/* Flush tags */
	mtpr(CCR_ENA, KA660_CCR);	/* Enable cache */
}

void
ka660_memerr(void)
{
	printf("Memory err!\n");
}

int
ka660_mchk(void *addr)
{
	panic("Machine check");
	return 0;
}
