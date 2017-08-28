/*	$NetBSD: ka670.c,v 1.16.36.1 2017/08/28 17:51:55 skrll Exp $	*/
/*
 * Copyright (c) 1999 Ludd, University of Lule}, Sweden.
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
__KERNEL_RCSID(0, "$NetBSD: ka670.c,v 1.16.36.1 2017/08/28 17:51:55 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/sid.h>
#include <machine/nexus.h>
#include <machine/vsbus.h>
#include <machine/ka670.h>
#include <machine/clock.h>

static void ka670_memerr(void);
static void ka670_conf(void);
static void ka670_attach_cpu(device_t);
static int ka670_mchk(void *);
static int ka670_cache_init(void);	/* "int mapen" as argument? */

static const char * const ka670_devs[] = { "cpu", "sgec", "shac", "uba", NULL };

const struct cpu_dep ka670_calls = {
	.cpu_mchk	= ka670_mchk,
	.cpu_memerr	= ka670_memerr,
	.cpu_conf	= ka670_conf,
	.cpu_gettime	= generic_gettime,
	.cpu_settime	= generic_settime,
	.cpu_vups	= 8,	/* 8 VUP */
	.cpu_scbsz	= 2,	/* SCB pages */
	.cpu_halt	= generic_halt,
	.cpu_reboot	= generic_reboot,
	.cpu_devs	= ka670_devs,
	.cpu_attach_cpu	= ka670_attach_cpu,
};

#define KA670_MC_RESTART	0x00008000	/* Restart possible*/
#define KA670_PSL_FPDONE	0x00010000	/* First Part Done */

struct ka670_mcframe {		/* Format of RigelMAX machine check frame: */
	int	mc670_bcnt;	/* byte count, always 24 (0x18) */
	int	mc670_code;	/* machine check type code and restart bit */
	int	mc670_addr;	/* most recent (faulting?) virtual address */
	int	mc670_viba;	/* contents of VIBA register */
	int	mc670_sisr;	/* ICCS bit 6 and SISR bits 15:0 */
	int	mc670_istate;	/* internal state */
	int	mc670_sc;	/* shift count register */
	int	mc670_pc;	/* trapped PC */
	int	mc670_psl;	/* trapped PSL */
};

#if 0

/*
 * This is not the mchk types on KA670.
 */
static const char * const ka670_mctype[] = {
	"no error (0)",			/* Code 0: No error */
	"FPA: protocol error",		/* Code 1-5: FPA errors */
	"FPA: illegal opcode",
	"FPA: operand parity error",
	"FPA: unknown status",
	"FPA: result parity error",
	"unused (6)",			/* Code 6-7: Unused */
	"unused (7)",
	"MMU error (TLB miss)",		/* Code 8-9: MMU errors */
	"MMU error (TLB hit)",
	"HW interrupt at unused IPL",	/* Code 10: Interrupt error */
	"MOVCx impossible state",	/* Code 11-13: Microcode errors */
	"undefined trap code (i-box)",
	"undefined control store address",
	"unused (14)",			/* Code 14-15: Unused */
	"unused (15)",
	"PC tag or data parity error",	/* Code 16: Cache error */
	"data bus parity error",	/* Code 17: Read error */
	"data bus error (NXM)",		/* Code 18: Write error */
	"undefined data bus state",	/* Code 19: Bus error */
};
#define MC670_MAX	19
#endif

static int ka670_error_count = 0;

int
ka670_mchk(void *addr)
{
	struct ka670_mcframe * const mcf = addr;

	mtpr(0x00, PR_MCESR);	/* Acknowledge the machine check */
	printf("machine check %d (0x%x)\n", mcf->mc670_code, mcf->mc670_code);
	printf("PC %x PSL %x\n", mcf->mc670_pc, mcf->mc670_psl);
	if (++ka670_error_count > 10) {
		printf("error_count exceeded: %d\n", ka670_error_count);
		return (-1);
	}

	/*
	 * If either the Restart flag is set or the First-Part-Done flag
	 * is set, and the TRAP2 (double error) bit is not set, then the
	 * error is recoverable.
	 */
	if (mfpr(PR_PCSTS) & KA670_PCS_TRAP2) {
		printf("TRAP2 (double error) in ka670_mchk.\n");
		panic("unrecoverable state in ka670_mchk.");
		return (-1);
	}
	if ((mcf->mc670_code & KA670_MC_RESTART) || 
	    (mcf->mc670_psl & KA670_PSL_FPDONE)) {
		printf("ka670_mchk: recovering from machine-check.\n");
		ka670_cache_init();	/* reset caches */
		return (0);		/* go on; */
	}

	/*
	 * Unknown error state, panic/halt the machine!
	 */
	printf("ka670_mchk: unknown error state!\n");
	return (-1);
}

void
ka670_memerr(void)
{
	char sbuf[256];

	/*
	 * Don\'t know what to do here. So just print some messages
	 * and try to go on...
	 */

	printf("memory error!\n");

	snprintb(sbuf, sizeof(sbuf), KA670_PCSTS_BITS, mfpr(PR_PCSTS));
	printf("primary cache status: %s\n", sbuf);

	snprintb(sbuf, sizeof(sbuf), KA670_BCSTS_BITS, mfpr(PR_BCSTS));
	printf("secondary cache status: %s\n", sbuf);
}

int
ka670_cache_init(void)
{
	int val;
#ifdef DEBUG
	char sbuf[256];
#endif

	mtpr(KA670_PCS_REFRESH, PR_PCSTS);	/* disable primary cache */
	val = mfpr(PR_PCSTS);
	mtpr(val, PR_PCSTS);			/* clear error flags */
	mtpr(8, PR_BCCTL);			/* disable backup cache */
	mtpr(0, PR_BCFBTS);	/* flush backup cache tag store */
	mtpr(0, PR_BCFPTS);	/* flush primary cache tag store */
	mtpr(0x0e, PR_BCCTL);	/* enable backup cache */
	mtpr(KA670_PCS_FLUSH | KA670_PCS_REFRESH, PR_PCSTS);	/* flush primary cache */
	mtpr(KA670_PCS_ENABLE | KA670_PCS_REFRESH, PR_PCSTS);	/* flush primary cache */

#ifdef DEBUG
	snprintb(sbuf, sizeof(sbuf), KA670_PCSTS_BITS, mfpr(PR_PCSTS));
	printf("primary cache status: %s\n", sbuf);

	snprintb(sbuf, sizeof(sbuf), KA670_BCSTS_BITS, mfpr(PR_BCSTS));
	printf("secondary cache status: %s\n", sbuf);
#endif

	return (0);
}

void
ka670_conf(void)
{

	/*
	 * ka670_conf() gets called with MMU enabled, now it's save to
	 * init/reset the caches.
	 */
	ka670_cache_init();

	cpmbx = (struct cpmbx *)vax_map_physmem(0x20140400, 1);
}

void
ka670_attach_cpu(device_t self)
{
	aprint_normal(
	    ": %s, Rigel (ucode rev %d), 2KB L1 cache, 128KB L2 cache\n",
	    "KA670",
	    vax_cpudata % 0377);
}
