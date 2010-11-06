/*	$NetBSD: cpu_subr.c,v 1.59 2010/11/06 11:46:01 uebayasi Exp $	*/

/*-
 * Copyright (c) 2001 Matt Thomas.
 * Copyright (c) 2001 Tsubai Masanari.
 * Copyright (c) 1998, 1999, 2001 Internet Research Institute, Inc.
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
 *	This product includes software developed by
 *	Internet Research Institute, Inc.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_subr.c,v 1.59 2010/11/06 11:46:01 uebayasi Exp $");

#include "opt_ppcparam.h"
#include "opt_multiprocessor.h"
#include "opt_altivec.h"
#include "sysmon_envsys.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/types.h>
#include <sys/lwp.h>
#include <sys/malloc.h>
#include <sys/xcall.h>

#include <uvm/uvm.h>

#include <powerpc/spr.h>
#include <powerpc/oea/hid.h>
#include <powerpc/oea/hid_601.h>
#include <powerpc/oea/spr.h>
#include <powerpc/oea/cpufeat.h>

#include <dev/sysmon/sysmonvar.h>

static void cpu_enable_l2cr(register_t);
static void cpu_enable_l3cr(register_t);
static void cpu_config_l2cr(int);
static void cpu_config_l3cr(int);
static void cpu_probe_speed(struct cpu_info *);
static void cpu_idlespin(void);
static void cpu_set_dfs_xcall(void *, void *);
#if NSYSMON_ENVSYS > 0
static void cpu_tau_setup(struct cpu_info *);
static void cpu_tau_refresh(struct sysmon_envsys *, envsys_data_t *);
#endif

int cpu;
int ncpus;

struct fmttab {
	register_t fmt_mask;
	register_t fmt_value;
	const char *fmt_string;
};

/*
 * This should be one per CPU but since we only support it on 750 variants it
 * doesn't realy matter since none of them supports SMP
 */
envsys_data_t sensor;

static const struct fmttab cpu_7450_l2cr_formats[] = {
	{ L2CR_L2E, 0, " disabled" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO, " data-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2IO, " instruction-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO|L2CR_L2IO, " locked" },
	{ L2CR_L2E, ~0, " 256KB L2 cache" },
	{ L2CR_L2PE, 0, " no parity" },
	{ L2CR_L2PE, ~0, " parity enabled" },
	{ 0, 0, NULL }
};

static const struct fmttab cpu_7448_l2cr_formats[] = {
	{ L2CR_L2E, 0, " disabled" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO, " data-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2IO, " instruction-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO|L2CR_L2IO, " locked" },
	{ L2CR_L2E, ~0, " 1MB L2 cache" },
	{ L2CR_L2PE, 0, " no parity" },
	{ L2CR_L2PE, ~0, " parity enabled" },
	{ 0, 0, NULL }
};

static const struct fmttab cpu_7457_l2cr_formats[] = {
	{ L2CR_L2E, 0, " disabled" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO, " data-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2IO, " instruction-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO|L2CR_L2IO, " locked" },
	{ L2CR_L2E, ~0, " 512KB L2 cache" },
	{ L2CR_L2PE, 0, " no parity" },
	{ L2CR_L2PE, ~0, " parity enabled" },
	{ 0, 0, NULL }
};

static const struct fmttab cpu_7450_l3cr_formats[] = {
	{ L3CR_L3DO|L3CR_L3IO, L3CR_L3DO, " data-only" },
	{ L3CR_L3DO|L3CR_L3IO, L3CR_L3IO, " instruction-only" },
	{ L3CR_L3DO|L3CR_L3IO, L3CR_L3DO|L3CR_L3IO, " locked" },
	{ L3CR_L3SIZ, L3SIZ_2M, " 2MB" },
	{ L3CR_L3SIZ, L3SIZ_1M, " 1MB" },
	{ L3CR_L3PE|L3CR_L3APE, L3CR_L3PE|L3CR_L3APE, " parity" },
	{ L3CR_L3PE|L3CR_L3APE, L3CR_L3PE, " data-parity" },
	{ L3CR_L3PE|L3CR_L3APE, L3CR_L3APE, " address-parity" },
	{ L3CR_L3PE|L3CR_L3APE, 0, " no-parity" },
	{ L3CR_L3SIZ, ~0, " L3 cache" },
	{ L3CR_L3RT, L3RT_MSUG2_DDR, " (DDR SRAM)" },
	{ L3CR_L3RT, L3RT_PIPELINE_LATE, " (LW SRAM)" },
	{ L3CR_L3RT, L3RT_PB2_SRAM, " (PB2 SRAM)" },
	{ L3CR_L3CLK, ~0, " at" },
	{ L3CR_L3CLK, L3CLK_20, " 2:1" },
	{ L3CR_L3CLK, L3CLK_25, " 2.5:1" },
	{ L3CR_L3CLK, L3CLK_30, " 3:1" },
	{ L3CR_L3CLK, L3CLK_35, " 3.5:1" },
	{ L3CR_L3CLK, L3CLK_40, " 4:1" },
	{ L3CR_L3CLK, L3CLK_50, " 5:1" },
	{ L3CR_L3CLK, L3CLK_60, " 6:1" },
	{ L3CR_L3CLK, ~0, " ratio" },
	{ 0, 0, NULL },
};

static const struct fmttab cpu_ibm750_l2cr_formats[] = {
	{ L2CR_L2E, 0, " disabled" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO, " data-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2IO, " instruction-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO|L2CR_L2IO, " locked" },
	{ 0, ~0, " 512KB" },
	{ L2CR_L2WT, L2CR_L2WT, " WT" },
	{ L2CR_L2WT, 0, " WB" },
	{ L2CR_L2PE, L2CR_L2PE, " with ECC" },
	{ 0, ~0, " L2 cache" },
	{ 0, 0, NULL }
};

static const struct fmttab cpu_l2cr_formats[] = {
	{ L2CR_L2E, 0, " disabled" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO, " data-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2IO, " instruction-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO|L2CR_L2IO, " locked" },
	{ L2CR_L2PE, L2CR_L2PE, " parity" },
	{ L2CR_L2PE, 0, " no-parity" },
	{ L2CR_L2SIZ, L2SIZ_2M, " 2MB" },
	{ L2CR_L2SIZ, L2SIZ_1M, " 1MB" },
	{ L2CR_L2SIZ, L2SIZ_512K, " 512KB" },
	{ L2CR_L2SIZ, L2SIZ_256K, " 256KB" },
	{ L2CR_L2WT, L2CR_L2WT, " WT" },
	{ L2CR_L2WT, 0, " WB" },
	{ L2CR_L2E, ~0, " L2 cache" },
	{ L2CR_L2RAM, L2RAM_FLOWTHRU_BURST, " (FB SRAM)" },
	{ L2CR_L2RAM, L2RAM_PIPELINE_LATE, " (LW SRAM)" },
	{ L2CR_L2RAM, L2RAM_PIPELINE_BURST, " (PB SRAM)" },
	{ L2CR_L2CLK, ~0, " at" },
	{ L2CR_L2CLK, L2CLK_10, " 1:1" },
	{ L2CR_L2CLK, L2CLK_15, " 1.5:1" },
	{ L2CR_L2CLK, L2CLK_20, " 2:1" },
	{ L2CR_L2CLK, L2CLK_25, " 2.5:1" },
	{ L2CR_L2CLK, L2CLK_30, " 3:1" },
	{ L2CR_L2CLK, L2CLK_35, " 3.5:1" },
	{ L2CR_L2CLK, L2CLK_40, " 4:1" },
	{ L2CR_L2CLK, ~0, " ratio" },
	{ 0, 0, NULL }
};

static void cpu_fmttab_print(const struct fmttab *, register_t);

struct cputab {
	const char name[8];
	uint16_t version;
	uint16_t revfmt;
};
#define	REVFMT_MAJMIN	1		/* %u.%u */
#define	REVFMT_HEX	2		/* 0x%04x */
#define	REVFMT_DEC	3		/* %u */
static const struct cputab models[] = {
	{ "601",	MPC601,		REVFMT_DEC },
	{ "602",	MPC602,		REVFMT_DEC },
	{ "603",	MPC603,		REVFMT_MAJMIN },
	{ "603e",	MPC603e,	REVFMT_MAJMIN },
	{ "603ev",	MPC603ev,	REVFMT_MAJMIN },
	{ "G2",		MPCG2,		REVFMT_MAJMIN },
	{ "604",	MPC604,		REVFMT_MAJMIN },
	{ "604e",	MPC604e,	REVFMT_MAJMIN },
	{ "604ev",	MPC604ev,	REVFMT_MAJMIN },
	{ "620",	MPC620,  	REVFMT_HEX },
	{ "750",	MPC750,		REVFMT_MAJMIN },
	{ "750FX",	IBM750FX,	REVFMT_MAJMIN },
	{ "7400",	MPC7400,	REVFMT_MAJMIN },
	{ "7410",	MPC7410,	REVFMT_MAJMIN },
	{ "7450",	MPC7450,	REVFMT_MAJMIN },
	{ "7455",	MPC7455,	REVFMT_MAJMIN },
	{ "7457",	MPC7457,	REVFMT_MAJMIN },
	{ "7447A",	MPC7447A,	REVFMT_MAJMIN },
	{ "7448",	MPC7448,	REVFMT_MAJMIN },
	{ "8240",	MPC8240,	REVFMT_MAJMIN },
	{ "8245",	MPC8245,	REVFMT_MAJMIN },
	{ "970",	IBM970,		REVFMT_MAJMIN },
	{ "970FX",	IBM970FX,	REVFMT_MAJMIN },
	{ "970MP",	IBM970MP,	REVFMT_MAJMIN },
	{ "POWER3II",   IBMPOWER3II,    REVFMT_MAJMIN },
	{ "",		0,		REVFMT_HEX }
};

#ifdef MULTIPROCESSOR
struct cpu_info cpu_info[CPU_MAXNUM] = { { .ci_curlwp = &lwp0, }, }; 
volatile struct cpu_hatch_data *cpu_hatch_data;
volatile int cpu_hatch_stack;
extern int ticks_per_intr;
#include <powerpc/oea/bat.h>
#include <arch/powerpc/pic/picvar.h>
#include <arch/powerpc/pic/ipivar.h>
extern struct bat battable[];
#else
struct cpu_info cpu_info[1] = { { .ci_curlwp = &lwp0, }, }; 
#endif /*MULTIPROCESSOR*/

int cpu_altivec;
int cpu_psluserset, cpu_pslusermod;
char cpu_model[80];

/* This is to be called from locore.S, and nowhere else. */

void
cpu_model_init(void)
{
	u_int pvr, vers;

	pvr = mfpvr();
	vers = pvr >> 16;

	oeacpufeat = 0;
	
	if ((vers >= IBMRS64II && vers <= IBM970GX) || vers == MPC620 ||
		vers == IBMCELL || vers == IBMPOWER6P5)
		oeacpufeat |= OEACPU_64 | OEACPU_64_BRIDGE | OEACPU_NOBAT;
	
	else if (vers == MPC601)
		oeacpufeat |= OEACPU_601;

	else if (MPC745X_P(vers) && vers != MPC7450)
		oeacpufeat |= OEACPU_XBSEN | OEACPU_HIGHBAT | OEACPU_HIGHSPRG;
}

void
cpu_fmttab_print(const struct fmttab *fmt, register_t data)
{
	for (; fmt->fmt_mask != 0 || fmt->fmt_value != 0; fmt++) {
		if ((~fmt->fmt_mask & fmt->fmt_value) != 0 ||
		    (data & fmt->fmt_mask) == fmt->fmt_value)
			aprint_normal("%s", fmt->fmt_string);
	}
}

void
cpu_idlespin(void)
{
	register_t msr;

	if (powersave <= 0)
		return;

	__asm volatile(
		"sync;"
		"mfmsr	%0;"
		"oris	%0,%0,%1@h;"	/* enter power saving mode */
		"mtmsr	%0;"
		"isync;"
	    :	"=r"(msr)
	    :	"J"(PSL_POW));
}

void
cpu_probe_cache(void)
{
	u_int assoc, pvr, vers;

	pvr = mfpvr();
	vers = pvr >> 16;


	/* Presently common across almost all implementations. */
	curcpu()->ci_ci.dcache_line_size = 32;
	curcpu()->ci_ci.icache_line_size = 32;


	switch (vers) {
#define	K	*1024
	case IBM750FX:
	case MPC601:
	case MPC750:
	case MPC7400:
	case MPC7447A:
	case MPC7448:
	case MPC7450:
	case MPC7455:
	case MPC7457:
		curcpu()->ci_ci.dcache_size = 32 K;
		curcpu()->ci_ci.icache_size = 32 K;
		assoc = 8;
		break;
	case MPC603:
		curcpu()->ci_ci.dcache_size = 8 K;
		curcpu()->ci_ci.icache_size = 8 K;
		assoc = 2;
		break;
	case MPC603e:
	case MPC603ev:
	case MPC604:
	case MPC8240:
	case MPC8245:
	case MPCG2:
		curcpu()->ci_ci.dcache_size = 16 K;
		curcpu()->ci_ci.icache_size = 16 K;
		assoc = 4;
		break;
	case MPC604e:
	case MPC604ev:
		curcpu()->ci_ci.dcache_size = 32 K;
		curcpu()->ci_ci.icache_size = 32 K;
		assoc = 4;
		break;
	case IBMPOWER3II:
		curcpu()->ci_ci.dcache_size = 64 K;
		curcpu()->ci_ci.icache_size = 32 K;
		curcpu()->ci_ci.dcache_line_size = 128;
		curcpu()->ci_ci.icache_line_size = 128;
		assoc = 128; /* not a typo */
		break;
	case IBM970:
	case IBM970FX:
	case IBM970MP:
		curcpu()->ci_ci.dcache_size = 32 K;
		curcpu()->ci_ci.icache_size = 64 K;
		curcpu()->ci_ci.dcache_line_size = 128;
		curcpu()->ci_ci.icache_line_size = 128;
		assoc = 2;
		break;

	default:
		curcpu()->ci_ci.dcache_size = PAGE_SIZE;
		curcpu()->ci_ci.icache_size = PAGE_SIZE;
		assoc = 1;
#undef	K
	}

	/*
	 * Possibly recolor.
	 */
	uvm_page_recolor(atop(curcpu()->ci_ci.dcache_size / assoc));
}

struct cpu_info *
cpu_attach_common(struct device *self, int id)
{
	struct cpu_info *ci;
	u_int pvr, vers;

	ci = &cpu_info[id];
#ifndef MULTIPROCESSOR
	/*
	 * If this isn't the primary CPU, print an error message
	 * and just bail out.
	 */
	if (id != 0) {
		aprint_normal(": ID %d\n", id);
		aprint_normal("%s: processor off-line; multiprocessor support "
		    "not present in kernel\n", self->dv_xname);
		return (NULL);
	}
#endif

	ci->ci_cpuid = id;
	ci->ci_intrdepth = -1;
	ci->ci_dev = self;
	ci->ci_idlespin = cpu_idlespin;

	pvr = mfpvr();
	vers = (pvr >> 16) & 0xffff;

	switch (id) {
	case 0:
		/* load my cpu_number to PIR */
		switch (vers) {
		case MPC601:
		case MPC604:
		case MPC604e:
		case MPC604ev:
		case MPC7400:
		case MPC7410:
		case MPC7447A:
		case MPC7448:
		case MPC7450:
		case MPC7455:
		case MPC7457:
			mtspr(SPR_PIR, id);
		}
		cpu_setup(self, ci);
		break;
	default:
		if (id >= CPU_MAXNUM) {
			aprint_normal(": more than %d cpus?\n", CPU_MAXNUM);
			panic("cpuattach");
		}
#ifndef MULTIPROCESSOR
		aprint_normal(" not configured\n");
		return NULL;
#else
		mi_cpu_attach(ci);
		break;
#endif
	}
	return (ci);
}

void
cpu_setup(struct device *self, struct cpu_info *ci)
{
	u_int hid0, hid0_save, pvr, vers;
	const char *bitmask;
	char hidbuf[128];
	char model[80];

	pvr = mfpvr();
	vers = (pvr >> 16) & 0xffff;

	cpu_identify(model, sizeof(model));
	aprint_normal(": %s, ID %d%s\n", model,  cpu_number(),
	    cpu_number() == 0 ? " (primary)" : "");

	/* set the cpu number */
	ci->ci_cpuid = cpu_number();
	hid0_save = hid0 = mfspr(SPR_HID0);

	cpu_probe_cache();

	/*
	 * Configure power-saving mode.
	 */
	switch (vers) {
	case MPC604:
	case MPC604e:
	case MPC604ev:
		/*
		 * Do not have HID0 support settings, but can support
		 * MSR[POW] off
		 */
		powersave = 1;
		break;

	case MPC603:
	case MPC603e:
	case MPC603ev:
	case MPC7400:
	case MPC7410:
	case MPC8240:
	case MPC8245:
	case MPCG2:
		/* Select DOZE mode. */
		hid0 &= ~(HID0_DOZE | HID0_NAP | HID0_SLEEP);
		hid0 |= HID0_DOZE | HID0_DPM;
		powersave = 1;
		break;

	case MPC750:
	case IBM750FX:
		/* Select NAP mode. */
		hid0 &= ~(HID0_DOZE | HID0_NAP | HID0_SLEEP);
		hid0 |= HID0_NAP | HID0_DPM;
		powersave = 1;
		break;

	case MPC7447A:
	case MPC7448:
	case MPC7457:
	case MPC7455:
	case MPC7450:
		/* Enable the 7450 branch caches */
		hid0 |= HID0_SGE | HID0_BTIC;
		hid0 |= HID0_LRSTK | HID0_FOLD | HID0_BHT;
		/* Enable more and larger BAT registers */
		if (oeacpufeat & OEACPU_XBSEN)
			hid0 |= HID0_XBSEN;
		if (oeacpufeat & OEACPU_HIGHBAT)
			hid0 |= HID0_HIGH_BAT_EN;
		/* Disable BTIC on 7450 Rev 2.0 or earlier */
		if (vers == MPC7450 && (pvr & 0xFFFF) <= 0x0200)
			hid0 &= ~HID0_BTIC;
		/* Select NAP mode. */
		hid0 &= ~HID0_SLEEP;
		hid0 |= HID0_NAP | HID0_DPM;
		powersave = 1;
		break;

	case IBM970:
	case IBM970FX:
	case IBM970MP:
	case IBMPOWER3II:
	default:
		/* No power-saving mode is available. */ ;
	}

#ifdef NAPMODE
	switch (vers) {
	case IBM750FX:
	case MPC750:
	case MPC7400:
		/* Select NAP mode. */
		hid0 &= ~(HID0_DOZE | HID0_NAP | HID0_SLEEP);
		hid0 |= HID0_NAP;
		break;
	}
#endif

	switch (vers) {
	case IBM750FX:
	case MPC750:
		hid0 &= ~HID0_DBP;		/* XXX correct? */
		hid0 |= HID0_EMCP | HID0_BTIC | HID0_SGE | HID0_BHT;
		break;

	case MPC7400:
	case MPC7410:
		hid0 &= ~HID0_SPD;
		hid0 |= HID0_EMCP | HID0_BTIC | HID0_SGE | HID0_BHT;
		hid0 |= HID0_EIEC;
		break;
	}

	if (hid0 != hid0_save) {
		mtspr(SPR_HID0, hid0);
		__asm volatile("sync;isync");
	}


	switch (vers) {
	case MPC601:
		bitmask = HID0_601_BITMASK;
		break;
	case MPC7450:
	case MPC7455:
	case MPC7457:
		bitmask = HID0_7450_BITMASK;
		break;
	case IBM970:
	case IBM970FX:
	case IBM970MP:
		bitmask = 0;
		break;
	default:
		bitmask = HID0_BITMASK;
		break;
	}
	snprintb(hidbuf, sizeof hidbuf, bitmask, hid0);
	aprint_normal("%s: HID0 %s, powersave: %d\n", self->dv_xname, hidbuf,
	    powersave);

	ci->ci_khz = 0;

	/*
	 * Display speed and cache configuration.
	 */
	switch (vers) {
	case MPC604:
	case MPC604e:
	case MPC604ev:
	case MPC750:
	case IBM750FX:
	case MPC7400:
	case MPC7410:
	case MPC7447A:
	case MPC7448:
	case MPC7450:
	case MPC7455:
	case MPC7457:
		aprint_normal("%s: ", self->dv_xname);
		cpu_probe_speed(ci);
		aprint_normal("%u.%02u MHz",
			      ci->ci_khz / 1000, (ci->ci_khz / 10) % 100);
		switch (vers) {
		case MPC7450: /* 7441 does not have L3! */
		case MPC7455: /* 7445 does not have L3! */
		case MPC7457: /* 7447 does not have L3! */
			cpu_config_l3cr(vers);
			break;
		case IBM750FX:
		case MPC750:
		case MPC7400:
		case MPC7410:
		case MPC7447A:
		case MPC7448:
			cpu_config_l2cr(pvr);
			break;
		default:
			break;
		}
		aprint_normal("\n");
		break;
	}

#if NSYSMON_ENVSYS > 0
	/*
	 * Attach MPC750 temperature sensor to the envsys subsystem.
	 * XXX the 74xx series also has this sensor, but it is not
	 * XXX supported by Motorola and may return values that are off by 
	 * XXX 35-55 degrees C.
	 */
	if (vers == MPC750 || vers == IBM750FX)
		cpu_tau_setup(ci);
#endif

	evcnt_attach_dynamic(&ci->ci_ev_clock, EVCNT_TYPE_INTR,
		NULL, self->dv_xname, "clock");
	evcnt_attach_dynamic(&ci->ci_ev_softclock, EVCNT_TYPE_INTR,
		NULL, self->dv_xname, "soft clock");
	evcnt_attach_dynamic(&ci->ci_ev_softnet, EVCNT_TYPE_INTR,
		NULL, self->dv_xname, "soft net");
	evcnt_attach_dynamic(&ci->ci_ev_softserial, EVCNT_TYPE_INTR,
		NULL, self->dv_xname, "soft serial");
	evcnt_attach_dynamic(&ci->ci_ev_traps, EVCNT_TYPE_TRAP,
		NULL, self->dv_xname, "traps");
	evcnt_attach_dynamic(&ci->ci_ev_kdsi, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, self->dv_xname, "kernel DSI traps");
	evcnt_attach_dynamic(&ci->ci_ev_udsi, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, self->dv_xname, "user DSI traps");
	evcnt_attach_dynamic(&ci->ci_ev_udsi_fatal, EVCNT_TYPE_TRAP,
		&ci->ci_ev_udsi, self->dv_xname, "user DSI failures");
	evcnt_attach_dynamic(&ci->ci_ev_kisi, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, self->dv_xname, "kernel ISI traps");
	evcnt_attach_dynamic(&ci->ci_ev_isi, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, self->dv_xname, "user ISI traps");
	evcnt_attach_dynamic(&ci->ci_ev_isi_fatal, EVCNT_TYPE_TRAP,
		&ci->ci_ev_isi, self->dv_xname, "user ISI failures");
	evcnt_attach_dynamic(&ci->ci_ev_scalls, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, self->dv_xname, "system call traps");
	evcnt_attach_dynamic(&ci->ci_ev_pgm, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, self->dv_xname, "PGM traps");
	evcnt_attach_dynamic(&ci->ci_ev_fpu, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, self->dv_xname, "FPU unavailable traps");
	evcnt_attach_dynamic(&ci->ci_ev_fpusw, EVCNT_TYPE_TRAP,
		&ci->ci_ev_fpu, self->dv_xname, "FPU context switches");
	evcnt_attach_dynamic(&ci->ci_ev_ali, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, self->dv_xname, "user alignment traps");
	evcnt_attach_dynamic(&ci->ci_ev_ali_fatal, EVCNT_TYPE_TRAP,
		&ci->ci_ev_ali, self->dv_xname, "user alignment traps");
	evcnt_attach_dynamic(&ci->ci_ev_umchk, EVCNT_TYPE_TRAP,
		&ci->ci_ev_umchk, self->dv_xname, "user MCHK failures");
	evcnt_attach_dynamic(&ci->ci_ev_vec, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, self->dv_xname, "AltiVec unavailable");
#ifdef ALTIVEC
	if (cpu_altivec) {
		evcnt_attach_dynamic(&ci->ci_ev_vecsw, EVCNT_TYPE_TRAP,
		    &ci->ci_ev_vec, self->dv_xname, "AltiVec context switches");
	}
#endif
	evcnt_attach_dynamic(&ci->ci_ev_ipi, EVCNT_TYPE_INTR,
		NULL, self->dv_xname, "IPIs");
}

/*
 * According to a document labeled "PVR Register Settings":
 ** For integrated microprocessors the PVR register inside the device
 ** will identify the version of the microprocessor core. You must also
 ** read the Device ID, PCI register 02, to identify the part and the
 ** Revision ID, PCI register 08, to identify the revision of the
 ** integrated microprocessor.
 * This apparently applies to 8240/8245/8241, PVR 00810101 and 80811014
 */

void
cpu_identify(char *str, size_t len)
{
	u_int pvr, major, minor;
	uint16_t vers, rev, revfmt;
	const struct cputab *cp;
	const char *name;
	size_t n;

	pvr = mfpvr();
	vers = pvr >> 16;
	rev = pvr;

	switch (vers) {
	case MPC7410:
		minor = (pvr >> 0) & 0xff;
		major = minor <= 4 ? 1 : 2;
		break;
	case MPCG2: /*XXX see note above */
		major = (pvr >> 4) & 0xf;
		minor = (pvr >> 0) & 0xf;
		break;
	default:
		major = (pvr >>  8) & 0xf;
		minor = (pvr >>  0) & 0xf;
	}

	for (cp = models; cp->name[0] != '\0'; cp++) {
		if (cp->version == vers)
			break;
	}

	if (str == NULL) {
		str = cpu_model;
		len = sizeof(cpu_model);
		cpu = vers;
	}

	revfmt = cp->revfmt;
	name = cp->name;
	if (rev == MPC750 && pvr == 15) {
		name = "755";
		revfmt = REVFMT_HEX;
	}

	if (cp->name[0] != '\0') {
		n = snprintf(str, len, "%s (Revision ", cp->name);
	} else {
		n = snprintf(str, len, "Version %#x (Revision ", vers);
	}
	if (len > n) {
		switch (revfmt) {
		case REVFMT_MAJMIN:
			snprintf(str + n, len - n, "%u.%u)", major, minor);
			break;
		case REVFMT_HEX:
			snprintf(str + n, len - n, "0x%04x)", rev);
			break;
		case REVFMT_DEC:
			snprintf(str + n, len - n, "%u)", rev);
			break;
		}
	}
}

#ifdef L2CR_CONFIG
u_int l2cr_config = L2CR_CONFIG;
#else
u_int l2cr_config = 0;
#endif

#ifdef L3CR_CONFIG
u_int l3cr_config = L3CR_CONFIG;
#else
u_int l3cr_config = 0;
#endif

void
cpu_enable_l2cr(register_t l2cr)
{
	register_t msr, x;
	uint16_t vers;

	vers = mfpvr() >> 16;
	
	/* Disable interrupts and set the cache config bits. */
	msr = mfmsr();
	mtmsr(msr & ~PSL_EE);
#ifdef ALTIVEC
	if (cpu_altivec)
		__asm volatile("dssall");
#endif
	__asm volatile("sync");
	mtspr(SPR_L2CR, l2cr & ~L2CR_L2E);
	__asm volatile("sync");

	/* Wait for L2 clock to be stable (640 L2 clocks). */
	delay(100);

	/* Invalidate all L2 contents. */
	if (MPC745X_P(vers)) {
		mtspr(SPR_L2CR, l2cr | L2CR_L2I);
		do {
			x = mfspr(SPR_L2CR);
		} while (x & L2CR_L2I);
	} else {
		mtspr(SPR_L2CR, l2cr | L2CR_L2I);
		do {
			x = mfspr(SPR_L2CR);
		} while (x & L2CR_L2IP);
	}
	/* Enable L2 cache. */
	l2cr |= L2CR_L2E;
	mtspr(SPR_L2CR, l2cr);
	mtmsr(msr);
}

void
cpu_enable_l3cr(register_t l3cr)
{
	register_t x;

	/* By The Book (numbered steps from section 3.7.1.3 of MPC7450UM) */
				
	/*
	 * 1: Set all L3CR bits for final config except L3E, L3I, L3PE, and
	 *    L3CLKEN.  (also mask off reserved bits in case they were included
	 *    in L3CR_CONFIG)
	 */
	l3cr &= ~(L3CR_L3E|L3CR_L3I|L3CR_L3PE|L3CR_L3CLKEN|L3CR_RESERVED);
	mtspr(SPR_L3CR, l3cr);

	/* 2: Set L3CR[5] (otherwise reserved bit) to 1 */
	l3cr |= 0x04000000;
	mtspr(SPR_L3CR, l3cr);

	/* 3: Set L3CLKEN to 1*/
	l3cr |= L3CR_L3CLKEN;
	mtspr(SPR_L3CR, l3cr);

	/* 4/5: Perform a global cache invalidate (ref section 3.7.3.6) */
	__asm volatile("dssall;sync");
	/* L3 cache is already disabled, no need to clear L3E */
	mtspr(SPR_L3CR, l3cr|L3CR_L3I);
	do {
		x = mfspr(SPR_L3CR);
	} while (x & L3CR_L3I);
	
	/* 6: Clear L3CLKEN to 0 */
	l3cr &= ~L3CR_L3CLKEN;
	mtspr(SPR_L3CR, l3cr);

	/* 7: Perform a 'sync' and wait at least 100 CPU cycles */
	__asm volatile("sync");
	delay(100);

	/* 8: Set L3E and L3CLKEN */
	l3cr |= (L3CR_L3E|L3CR_L3CLKEN);
	mtspr(SPR_L3CR, l3cr);

	/* 9: Perform a 'sync' and wait at least 100 CPU cycles */
	__asm volatile("sync");
	delay(100);
}

void
cpu_config_l2cr(int pvr)
{
	register_t l2cr;
	u_int vers = (pvr >> 16) & 0xffff;

	l2cr = mfspr(SPR_L2CR);

	/*
	 * For MP systems, the firmware may only configure the L2 cache
	 * on the first CPU.  In this case, assume that the other CPUs
	 * should use the same value for L2CR.
	 */
	if ((l2cr & L2CR_L2E) != 0 && l2cr_config == 0) {
		l2cr_config = l2cr;
	}

	/*
	 * Configure L2 cache if not enabled.
	 */
	if ((l2cr & L2CR_L2E) == 0 && l2cr_config != 0) {
		cpu_enable_l2cr(l2cr_config);
		l2cr = mfspr(SPR_L2CR);
	}

	if ((l2cr & L2CR_L2E) == 0) {
		aprint_normal(" L2 cache present but not enabled ");
		return;
	}
	aprint_normal(",");

	switch (vers) {
	case IBM750FX:
		cpu_fmttab_print(cpu_ibm750_l2cr_formats, l2cr);
		break;
	case MPC750:
		if ((pvr & 0xffffff00) == 0x00082200 /* IBM750CX */ ||
		    (pvr & 0xffffef00) == 0x00082300 /* IBM750CXe */)
			cpu_fmttab_print(cpu_ibm750_l2cr_formats, l2cr);
		else
			cpu_fmttab_print(cpu_l2cr_formats, l2cr);
		break;
	case MPC7447A:
	case MPC7457:
		cpu_fmttab_print(cpu_7457_l2cr_formats, l2cr);
		return;
	case MPC7448:
		cpu_fmttab_print(cpu_7448_l2cr_formats, l2cr);
		return;
	case MPC7450:
	case MPC7455:
		cpu_fmttab_print(cpu_7450_l2cr_formats, l2cr);
		break;
	default:
		cpu_fmttab_print(cpu_l2cr_formats, l2cr);
		break;
	}
}

void
cpu_config_l3cr(int vers)
{
	register_t l2cr;
	register_t l3cr;

	l2cr = mfspr(SPR_L2CR);

	/*
	 * For MP systems, the firmware may only configure the L2 cache
	 * on the first CPU.  In this case, assume that the other CPUs
	 * should use the same value for L2CR.
	 */
	if ((l2cr & L2CR_L2E) != 0 && l2cr_config == 0) {
		l2cr_config = l2cr;
	}

	/*
	 * Configure L2 cache if not enabled.
	 */
	if ((l2cr & L2CR_L2E) == 0 && l2cr_config != 0) {
		cpu_enable_l2cr(l2cr_config);
		l2cr = mfspr(SPR_L2CR);
	}
	
	aprint_normal(",");
	switch (vers) {
	case MPC7447A:
	case MPC7457:
		cpu_fmttab_print(cpu_7457_l2cr_formats, l2cr);
		return;
	case MPC7448:
		cpu_fmttab_print(cpu_7448_l2cr_formats, l2cr);
		return;
	default:
		cpu_fmttab_print(cpu_7450_l2cr_formats, l2cr);
		break;
	}

	l3cr = mfspr(SPR_L3CR);

	/*
	 * For MP systems, the firmware may only configure the L3 cache
	 * on the first CPU.  In this case, assume that the other CPUs
	 * should use the same value for L3CR.
	 */
	if ((l3cr & L3CR_L3E) != 0 && l3cr_config == 0) {
		l3cr_config = l3cr;
	}

	/*
	 * Configure L3 cache if not enabled.
	 */
	if ((l3cr & L3CR_L3E) == 0 && l3cr_config != 0) {
		cpu_enable_l3cr(l3cr_config);
		l3cr = mfspr(SPR_L3CR);
	}
	
	if (l3cr & L3CR_L3E) {
		aprint_normal(",");
		cpu_fmttab_print(cpu_7450_l3cr_formats, l3cr);
	}
}

void
cpu_probe_speed(struct cpu_info *ci)
{
	uint64_t cps;

	mtspr(SPR_MMCR0, MMCR0_FC);
	mtspr(SPR_PMC1, 0);
	mtspr(SPR_MMCR0, MMCR0_PMC1SEL(PMCN_CYCLES));
	delay(100000);
	cps = (mfspr(SPR_PMC1) * 10) + 4999;

	mtspr(SPR_MMCR0, MMCR0_FC);

	ci->ci_khz = (cps * cpu_get_dfs()) / 1000;
}

/*
 * Read the Dynamic Frequency Switching state and return a divisor for
 * the maximum frequency.
 */
int
cpu_get_dfs(void)
{
	u_int pvr, vers;

	pvr = mfpvr();
	vers = pvr >> 16;

	switch (vers) {
	case MPC7448:
		if (mfspr(SPR_HID1) & HID1_DFS4)
			return 4;
	case MPC7447A:
		if (mfspr(SPR_HID1) & HID1_DFS2)
			return 2;
	}
	return 1;
}

/*
 * Set the Dynamic Frequency Switching divisor the same for all cpus.
 */
void
cpu_set_dfs(int div)
{
	uint64_t where;
	u_int dfs_mask, pvr, vers;

	pvr = mfpvr();
	vers = pvr >> 16;
	dfs_mask = 0;

	switch (vers) {
	case MPC7448:
		dfs_mask |= HID1_DFS4;
	case MPC7447A:
		dfs_mask |= HID1_DFS2;
		break;
	default:
		printf("cpu_set_dfs: DFS not supported\n");
		return;

	}

	where = xc_broadcast(0, (xcfunc_t)cpu_set_dfs_xcall, &div, &dfs_mask);
	xc_wait(where);
}

static void
cpu_set_dfs_xcall(void *arg1, void *arg2)
{
	u_int dfs_mask, hid1, old_hid1;
	int *divisor, s;

	divisor = arg1;
	dfs_mask = *(u_int *)arg2;

	s = splhigh();
	hid1 = old_hid1 = mfspr(SPR_HID1);

	switch (*divisor) {
	case 1:
		hid1 &= ~dfs_mask;
		break;
	case 2:
		hid1 &= ~(dfs_mask & HID1_DFS4);
		hid1 |= dfs_mask & HID1_DFS2;
		break;
	case 4:
		hid1 &= ~(dfs_mask & HID1_DFS2);
		hid1 |= dfs_mask & HID1_DFS4;
		break;
	}

	if (hid1 != old_hid1) {
		__asm volatile("sync");
		mtspr(SPR_HID1, hid1);
		__asm volatile("sync;isync");
	}

	splx(s);
}

#if NSYSMON_ENVSYS > 0
void
cpu_tau_setup(struct cpu_info *ci)
{
	struct sysmon_envsys *sme;
	int error, therm_delay;

	mtspr(SPR_THRM1, SPR_THRM_VALID);
	mtspr(SPR_THRM2, 0);

	/*
	 * we need to figure out how much 20+us in units of CPU clock cycles
	 * are
	 */

	therm_delay = ci->ci_khz / 40;		/* 25us just to be safe */
	
        mtspr(SPR_THRM3, SPR_THRM_TIMER(therm_delay) | SPR_THRM_ENABLE); 

	sme = sysmon_envsys_create();

	sensor.units = ENVSYS_STEMP;
	(void)strlcpy(sensor.desc, "CPU Temp", sizeof(sensor.desc));
	if (sysmon_envsys_sensor_attach(sme, &sensor)) {
		sysmon_envsys_destroy(sme);
		return;
	}

	sme->sme_name = ci->ci_dev->dv_xname;	
	sme->sme_cookie = ci;
	sme->sme_refresh = cpu_tau_refresh;

	if ((error = sysmon_envsys_register(sme)) != 0) {
		aprint_error("%s: unable to register with sysmon (%d)\n",
		    ci->ci_dev->dv_xname, error);
		sysmon_envsys_destroy(sme);
	}
}


/* Find the temperature of the CPU. */
void
cpu_tau_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	int i, threshold, count;

	threshold = 64; /* Half of the 7-bit sensor range */

	/* Successive-approximation code adapted from Motorola
	 * application note AN1800/D, "Programming the Thermal Assist
	 * Unit in the MPC750 Microprocessor".
	 */
	for (i = 5; i >= 0 ; i--) {
		mtspr(SPR_THRM1, 
		    SPR_THRM_THRESHOLD(threshold) | SPR_THRM_VALID);
		count = 0;
		while ((count < 100000) && 
		    ((mfspr(SPR_THRM1) & SPR_THRM_TIV) == 0)) {
			count++;
			delay(1);
		}
		if (mfspr(SPR_THRM1) & SPR_THRM_TIN) {
			/* The interrupt bit was set, meaning the 
			 * temperature was above the threshold 
			 */
			threshold += 1 << i;
		} else {
			/* Temperature was below the threshold */
			threshold -= 1 << i;
		}
		
	}
	threshold += 2;

	/* Convert the temperature in degrees C to microkelvin */
	edata->value_cur = (threshold * 1000000) + 273150000;
	edata->state = ENVSYS_SVALID;
}
#endif /* NSYSMON_ENVSYS > 0 */

#ifdef MULTIPROCESSOR
extern volatile u_int cpu_spinstart_ack;

int
cpu_spinup(struct device *self, struct cpu_info *ci)
{
	volatile struct cpu_hatch_data hatch_data, *h = &hatch_data;
	struct pglist mlist;
	int i, error, pvr, vers;
	char *cp, *hp;

	pvr = mfpvr();
	vers = pvr >> 16;
	KASSERT(ci != curcpu());

	/*
	 * Allocate some contiguous pages for the intteup PCB and stack
	 * from the lowest 256MB (because bat0 always maps it va == pa).
	 * Must be 16 byte aligned.
	 */
	error = uvm_pglistalloc(INTSTK, 0x10000, 0x10000000, 16, 0,
	    &mlist, 1, 1);
	if (error) {
		aprint_error(": unable to allocate idle stack\n");
		return -1;
	}

	KASSERT(ci != &cpu_info[0]);

	cp = (void *)VM_PAGE_TO_PHYS(TAILQ_FIRST(&mlist));
	memset(cp, 0, INTSTK);

	ci->ci_intstk = cp;

	/* Now allocate a hatch stack */
	error = uvm_pglistalloc(0x1000, 0x10000, 0x10000000, 16, 0,
	    &mlist, 1, 1);
	if (error) {
		aprint_error(": unable to allocate hatch stack\n");
		return -1;
	}

	hp = (void *)VM_PAGE_TO_PHYS(TAILQ_FIRST(&mlist));
	memset(hp, 0, 0x1000);

	/* Initialize secondary cpu's initial lwp to its idlelwp. */
	ci->ci_curlwp = ci->ci_data.cpu_idlelwp;
	ci->ci_curpcb = lwp_getpcb(ci->ci_curlwp);
	ci->ci_curpm = ci->ci_curpcb->pcb_pm;

	cpu_hatch_data = h;
	h->running = 0;
	h->self = self;
	h->ci = ci;
	h->pir = ci->ci_cpuid;

	cpu_hatch_stack = (uint32_t)hp;
	ci->ci_lasttb = cpu_info[0].ci_lasttb;

	/* copy special registers */

	h->hid0 = mfspr(SPR_HID0);
	
	__asm volatile ("mfsdr1 %0" : "=r"(h->sdr1));
	for (i = 0; i < 16; i++) {
		__asm ("mfsrin %0,%1" : "=r"(h->sr[i]) :
		       "r"(i << ADDR_SR_SHFT));
	}
	if (oeacpufeat & OEACPU_64)
		h->asr = mfspr(SPR_ASR);
	else
		h->asr = 0;

	/* copy the bat regs */
	__asm volatile ("mfibatu %0,0" : "=r"(h->batu[0]));
	__asm volatile ("mfibatl %0,0" : "=r"(h->batl[0]));
	__asm volatile ("mfibatu %0,1" : "=r"(h->batu[1]));
	__asm volatile ("mfibatl %0,1" : "=r"(h->batl[1]));
	__asm volatile ("mfibatu %0,2" : "=r"(h->batu[2]));
	__asm volatile ("mfibatl %0,2" : "=r"(h->batl[2]));
	__asm volatile ("mfibatu %0,3" : "=r"(h->batu[3]));
	__asm volatile ("mfibatl %0,3" : "=r"(h->batl[3]));
	__asm volatile ("sync; isync");

	if (md_setup_trampoline(h, ci) == -1)
		return -1;
	md_presync_timebase(h);
	md_start_timebase(h);

	/* wait for secondary printf */

	delay(200000);

	if (h->running < 1) {
		aprint_error("%d:CPU %d didn't start %d\n", cpu_spinstart_ack,
		    ci->ci_cpuid, cpu_spinstart_ack);
		Debugger();
		return -1;
	}

	/* Register IPI Interrupt */
	if (ipiops.ppc_establish_ipi)
		ipiops.ppc_establish_ipi(IST_LEVEL, IPL_HIGH, NULL);

	return 0;
}

static volatile int start_secondary_cpu;
extern void tlbia(void);

register_t
cpu_hatch(void)
{
	volatile struct cpu_hatch_data *h = cpu_hatch_data;
	struct cpu_info * const ci = h->ci;
	struct pcb *pcb;
	u_int msr;
	int i;

	/* Initialize timebase. */
	__asm ("mttbl %0; mttbu %0; mttbl %0" :: "r"(0));

	/*
	 * Set PIR (Processor Identification Register).  i.e. whoami
	 * Note that PIR is read-only on some CPU versions, so we write to it
	 * only if it has a different value than we need.
	 */

	msr = mfspr(SPR_PIR);
	if (msr != h->pir)
		mtspr(SPR_PIR, h->pir);
	
	__asm volatile ("mtsprg 0,%0" :: "r"(ci));
	cpu_spinstart_ack = 0;

	/* Initialize MMU. */
	__asm ("mtibatu 0,%0" :: "r"(h->batu[0]));
	__asm ("mtibatl 0,%0" :: "r"(h->batl[0]));
	__asm ("mtibatu 1,%0" :: "r"(h->batu[1]));
	__asm ("mtibatl 1,%0" :: "r"(h->batl[1]));
	__asm ("mtibatu 2,%0" :: "r"(h->batu[2]));
	__asm ("mtibatl 2,%0" :: "r"(h->batl[2]));
	__asm ("mtibatu 3,%0" :: "r"(h->batu[3]));
	__asm ("mtibatl 3,%0" :: "r"(h->batl[3]));

	mtspr(SPR_HID0, h->hid0);

	__asm ("mtibatl 0,%0; mtibatu 0,%1; mtdbatl 0,%0; mtdbatu 0,%1;"
	    :: "r"(battable[0].batl), "r"(battable[0].batu));

	__asm volatile ("sync");
	for (i = 0; i < 16; i++)
		__asm ("mtsrin %0,%1" :: "r"(h->sr[i]), "r"(i << ADDR_SR_SHFT));
	__asm volatile ("sync; isync");

	if (oeacpufeat & OEACPU_64)
		mtspr(SPR_ASR, h->asr);

	cpu_spinstart_ack = 1;
	__asm ("ptesync");
	__asm ("mtsdr1 %0" :: "r"(h->sdr1));
	__asm volatile ("sync; isync");

	cpu_spinstart_ack = 5;
	for (i = 0; i < 16; i++)
		__asm ("mfsrin %0,%1" : "=r"(h->sr[i]) :
		       "r"(i << ADDR_SR_SHFT));

	/* Enable I/D address translations. */
	msr = mfmsr();
	msr |= PSL_IR|PSL_DR|PSL_ME|PSL_RI;
	mtmsr(msr);
	__asm volatile ("sync; isync");
	cpu_spinstart_ack = 2;

	md_sync_timebase(h);

	cpu_setup(h->self, ci);

	h->running = 1;
	__asm volatile ("sync; isync");

	while (start_secondary_cpu == 0)
		;

	__asm volatile ("sync; isync");

	aprint_normal("cpu%d started\n", curcpu()->ci_index);
	__asm volatile ("mtdec %0" :: "r"(ticks_per_intr));

	md_setup_interrupts();

	ci->ci_ipending = 0;
	ci->ci_cpl = 0;

	mtmsr(mfmsr() | PSL_EE);
	pcb = lwp_getpcb(ci->ci_data.cpu_idlelwp);
	return pcb->pcb_sp;
}

void
cpu_boot_secondary_processors(void)
{
	start_secondary_cpu = 1;
	__asm volatile ("sync");
}

#endif /*MULTIPROCESSOR*/
