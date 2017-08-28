/*	$NetBSD: ka43.c,v 1.35.36.1 2017/08/28 17:51:55 skrll Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: ka43.c,v 1.35.36.1 2017/08/28 17:51:55 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/sid.h>
#include <machine/nexus.h>
#include <machine/vsbus.h>
#include <machine/ka43.h>
#include <machine/clock.h>

static	void ka43_conf(void);
static	void ka43_steal_pages(void);

static	int ka43_mchk(void *);
static	void ka43_memerr(void);
#if 0
static	void ka43_clear_errors(void);
#endif
static	int ka43_cache_init(void);	/* "int mapen" as argument? */
static	int ka43_cache_reset(void);
static	int ka43_cache_enable(void);
static	int ka43_cache_disable(void);
static	int ka43_cache_invalidate(void);
static  void ka43_halt(void);
static  void ka43_reboot(int);
static  void ka43_clrf(void);

static const char * const ka43_devs[] = { "cpu", "vsbus", NULL };

const struct cpu_dep ka43_calls = {
	.cpu_steal_pages = ka43_steal_pages,
	.cpu_mchk	= ka43_mchk,
	.cpu_memerr	= ka43_memerr,
	.cpu_conf	= ka43_conf,
	.cpu_gettime	= chip_gettime,
	.cpu_settime	= chip_settime,
	.cpu_vups	= 7,	/* 7.6 VUP */
	.cpu_scbsz	= 2,	/* SCB pages */
	.cpu_halt	= ka43_halt,
	.cpu_reboot	= ka43_reboot,
	.cpu_clrf	= ka43_clrf,
	.cpu_devs	= ka43_devs,
	.cpu_flags	= CPU_RAISEIPL,
};

/*
 * ka43_steal_pages() is called with MMU disabled, after that call MMU gets
 * enabled. Thus we initialize these four pointers with physical addresses,
 * but before leving ka43_steal_pages() we reset them to virtual addresses.
 */
static	volatile struct	ka43_cpu   *ka43_cpu	= (void*)KA43_CPU_BASE;
static	volatile u_int	*ka43_creg = (void*)KA43_CH2_CREG;
static	volatile u_int	*ka43_ctag = (void*)KA43_CT2_BASE;

#define KA43_MC_RESTART	0x00008000	/* Restart possible*/
#define KA43_PSL_FPDONE	0x00010000	/* First Part Done */

struct ka43_mcframe {		/* Format of RigelMAX machine check frame: */
	int	mc43_bcnt;	/* byte count, always 24 (0x18) */
	int	mc43_code;	/* machine check type code and restart bit */
	int	mc43_addr;	/* most recent (faulting?) virtual address */
	int	mc43_viba;	/* contents of VIBA register */
	int	mc43_sisr;	/* ICCS bit 6 and SISR bits 15:0 */
	int	mc43_istate;	/* internal state */
	int	mc43_sc;	/* shift count register */
	int	mc43_pc;	/* trapped PC */
	int	mc43_psl;	/* trapped PSL */
};

static const char * const ka43_mctype[] = {
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
#define MC43_MAX	19

static int ka43_error_count = 0;

int
ka43_mchk(void *addr)
{
	struct ka43_mcframe *mcf = (void*)addr;

	mtpr(0x00, PR_MCESR);	/* Acknowledge the machine check */
	printf("machine check %d (0x%x)\n", mcf->mc43_code, mcf->mc43_code);
	printf("reason: %s\n", ka43_mctype[mcf->mc43_code & 0xff]);
	if (++ka43_error_count > 10) {
		printf("error_count exceeded: %d\n", ka43_error_count);
		return (-1);
	}

	/*
	 * If either the Restart flag is set or the First-Part-Done flag
	 * is set, and the TRAP2 (double error) bit is not set, then the
	 * error is recoverable.
	 */
	if (mfpr(PR_PCSTS) & KA43_PCS_TRAP2) {
		printf("TRAP2 (double error) in ka43_mchk.\n");
		panic("unrecoverable state in ka43_mchk.");
		return (-1);
	}
	if ((mcf->mc43_code & KA43_MC_RESTART) || 
	    (mcf->mc43_psl & KA43_PSL_FPDONE)) {
		printf("ka43_mchk: recovering from machine-check.\n");
		ka43_cache_reset();	/* reset caches */
		return (0);		/* go on; */
	}

	/*
	 * Unknown error state, panic/halt the machine!
	 */
	printf("ka43_mchk: unknown error state!\n");
	return (-1);
}

void
ka43_memerr(void)
{
	char sbuf[256];

	/*
	 * Don\'t know what to do here. So just print some messages
	 * and try to go on...
	 */

	printf("memory error!\n");

	snprintb(sbuf, sizeof(sbuf), KA43_PCSTS_BITS, mfpr(PR_PCSTS));
	printf("primary cache status: %s\n", sbuf);

	snprintb(sbuf, sizeof(sbuf), KA43_SESR_BITS, *ka43_creg);
	printf("secondary cache status: %s\n", sbuf);
}

int
ka43_cache_init(void)
{
	return (ka43_cache_reset());
}

#if 0
void
ka43_clear_errors(void)
{
	int val = *ka43_creg;
	val |= KA43_SESR_SERR | KA43_SESR_LERR | KA43_SESR_CERR;
	*ka43_creg = val;
}
#endif

int
ka43_cache_reset(void)
{
	char sbuf[256];

	/*
	 * resetting primary and secondary caches is done in three steps:
	 *	1. disable both caches
	 *	2. manually clear secondary cache
	 *	3. enable both caches
	 */
	ka43_cache_disable();
	ka43_cache_invalidate();
	ka43_cache_enable();

	snprintb(sbuf, sizeof(sbuf), KA43_PCSTS_BITS, mfpr(PR_PCSTS));
	printf("primary cache status: %s\n", sbuf);

	snprintb(sbuf, sizeof(sbuf), KA43_SESR_BITS, *ka43_creg);
	printf("secondary cache status: %s\n", sbuf);

	return (0);
}

int
ka43_cache_disable(void)
{
	int val;

	/*
	 * first disable primary cache and clear error flags
	 */
	mtpr(KA43_PCS_REFRESH, PR_PCSTS);	/* disable primary cache */
	val = mfpr(PR_PCSTS);
	mtpr(val, PR_PCSTS);			/* clear error flags */

	/*
	 * now disable secondary cache and clear error flags
	 */
	val = *ka43_creg & ~KA43_SESR_CENB;	/* BICL !!! */
	*ka43_creg = val;			/* disable secondary cache */
	val = KA43_SESR_SERR | KA43_SESR_LERR | KA43_SESR_CERR;
	*ka43_creg = val;			/* clear error flags */

	return (0);
}

int
ka43_cache_invalidate(void)
{
	int i, val;

	val = KA43_PCTAG_PARITY;	/* clear valid flag, set parity bit */
	for (i = 0; i < 256; i++) {	/* 256 Quadword entries */
		mtpr(i*8, PR_PCIDX);	/* write index of tag */
		mtpr(val, PR_PCTAG);	/* write value into tag */
	}
	val = KA43_PCS_FLUSH | KA43_PCS_REFRESH;
	mtpr(val, PR_PCSTS);		/* flush primary cache */

	/*
	 * Rigel\'s secondary cache doesn\'t implement a valid-flag.
	 * Thus we initialize all entries with out-of-range/dummy
	 * addresses which will never be referenced (ie. never hit).
	 * After enabling cache we also access 128K of memory starting
	 * at 0x00 so that secondary cache will be filled with these
	 * valid addresses...
	 */
	val = 0xff;
	/* if (memory > 28 MB) val = 0x55; */
	for (i = 0; i < KA43_CT2_SIZE; i+= 4) {	/* Quadword entries ?? */
		ka43_ctag[i/4] = val;		/* reset upper and lower */
	}

	return (0);
}


int
ka43_cache_enable(void)
{
	volatile char *membase = (void*)0x80000000;	/* physical 0x00 */
	int i, val;

	val = KA43_PCS_FLUSH | KA43_PCS_REFRESH;
	mtpr(val, PR_PCSTS);		/* flush primary cache */

	/*
	 * now we enable secondary cache and access first 128K of memory
	 * so that secondary cache gets really initialized and holds
	 * valid addresses/data...
	 */
	*ka43_creg = KA43_SESR_CENB;	/* enable secondary cache */
	for (i=0; i<128*1024; i++) {
		val += membase[i];	/* some dummy operation... */
	}

	val = KA43_PCS_ENABLE | KA43_PCS_REFRESH;
	mtpr(val, PR_PCSTS);		/* enable primary cache */

	return (0);
}

void
ka43_conf(void)
{
	curcpu()->ci_cpustr = "Rigel, 2KB L1 cache, 128KB L2 cache";

	ka43_cpu = (void *)vax_map_physmem(VS_REGS, 1);
	ka43_creg = (void *)vax_map_physmem(KA43_CH2_CREG, 1);
	ka43_ctag = (void *)vax_map_physmem(KA43_CT2_BASE,
	    (KA43_CT2_SIZE/VAX_NBPG));

	/*
	 * ka43_conf() gets called with MMU enabled, now it's save to
	 * init/reset the caches.
	 */
	ka43_cache_init();

        clk_adrshift = 1;       /* Addressed at long's... */
        clk_tweak = 2;          /* ...and shift two */
	clk_page = (short *)vax_map_physmem(VS_CLOCK, 1);
}


/*
 * The interface for communication with the LANCE ethernet controller
 * is setup in the xxx_steal_pages() routine. We decrease highest
 * available address by 64K and use this area as communication buffer.
 */

void
ka43_steal_pages(void)
{
	int	val;


	/*
	 * if LANCE\'s io-buffer is above 16 MB, then the appropriate flag
	 * in the parity control register has to be set (it works as an
	 * additional address bit). In any case, don\'t enable CPEN and
	 * DPEN in the PARCTL register, somewhow they are internally managed
	 * by the RIGEL chip itself!?!
	 */
	val = ka43_cpu->parctl & 0x03;	/* read the old value */
	ka43_cpu->parctl = val;		/* and write new value */
}

void
ka43_clrf(void)
{
        volatile struct ka43_clock *clk = (volatile void *)clk_page;

        /*
         * Clear restart and boot in progress flags in the CPMBX.
	 * The cpmbx is split into two 4-bit fields.
	 * One for the current restart/boot in progress flags, and
	 * one for the permanent halt flag.
	 * The restart/boot in progress flag is also used as the action request
	 * for the CPU at a halt. /BQT
         */
        clk->req = 0;
}

void
ka43_halt(void)
{
	volatile struct ka43_clock *clk = (volatile void *)clk_page;
	clk->req = 3;		/* 3 is halt. */
	__asm("halt");
}

void
ka43_reboot(int arg)
{
	volatile struct ka43_clock *clk = (volatile void *)clk_page;
	clk->req = 2;		/* 2 is reboot. */
	__asm("halt");
}
