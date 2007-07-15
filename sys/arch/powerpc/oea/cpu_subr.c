/*	$NetBSD: cpu_subr.c,v 1.28.8.3 2007/07/15 13:16:49 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cpu_subr.c,v 1.28.8.3 2007/07/15 13:16:49 ad Exp $");

#include "opt_ppcparam.h"
#include "opt_multiprocessor.h"
#include "opt_altivec.h"
#include "sysmon_envsys.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <powerpc/oea/hid.h>
#include <powerpc/oea/hid_601.h>
#include <powerpc/spr.h>

#include <dev/sysmon/sysmonvar.h>

static void cpu_enable_l2cr(register_t);
static void cpu_enable_l3cr(register_t);
static void cpu_config_l2cr(int);
static void cpu_config_l3cr(int);
static void cpu_probe_speed(struct cpu_info *);
static void cpu_idlespin(void);
#if NSYSMON_ENVSYS > 0
static void cpu_tau_setup(struct cpu_info *);
static int cpu_tau_gtredata(struct sysmon_envsys *, envsys_data_t *);
#endif

int cpu;
int ncpus;

struct fmttab {
	register_t fmt_mask;
	register_t fmt_value;
	const char *fmt_string;
};

static const struct fmttab cpu_7450_l2cr_formats[] = {
	{ L2CR_L2E, 0, " disabled" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO, " data-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2IO, " instruction-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO|L2CR_L2IO, " locked" },
	{ L2CR_L2E, ~0, " 256KB L2 cache" },
	{ 0, 0, NULL }
};

static const struct fmttab cpu_7448_l2cr_formats[] = {
	{ L2CR_L2E, 0, " disabled" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO, " data-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2IO, " instruction-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO|L2CR_L2IO, " locked" },
	{ L2CR_L2E, ~0, " 1MB L2 cache" },
	{ 0, 0, NULL }
};

static const struct fmttab cpu_7457_l2cr_formats[] = {
	{ L2CR_L2E, 0, " disabled" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO, " data-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2IO, " instruction-only" },
	{ L2CR_L2DO|L2CR_L2IO, L2CR_L2DO|L2CR_L2IO, " locked" },
	{ L2CR_L2E, ~0, " 512KB L2 cache" },
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
	{ "",		0,		REVFMT_HEX }
};


#ifdef MULTIPROCESSOR
struct cpu_info cpu_info[CPU_MAXNUM];
#else
struct cpu_info cpu_info[1];
#endif

int cpu_altivec;
int cpu_psluserset, cpu_pslusermod;
char cpu_model[80];

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
	curcpu()->ci_ci.dcache_line_size = CACHELINESIZE;
	curcpu()->ci_ci.icache_line_size = CACHELINESIZE;


	switch (vers) {
#define	K	*1024
	case IBM750FX:
	case MPC601:
	case MPC750:
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
	case IBM970:
	case IBM970FX:
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
cpu_setup(self, ci)
	struct device *self;
	struct cpu_info *ci;
{
	u_int hid0, pvr, vers;
	const char *bitmask;
	char hidbuf[128];
	char model[80];

	pvr = mfpvr();
	vers = (pvr >> 16) & 0xffff;

	cpu_identify(model, sizeof(model));
	aprint_normal(": %s, ID %d%s\n", model,  cpu_number(),
	    cpu_number() == 0 ? " (primary)" : "");

#if defined (PPC_OEA) || defined (PPC_OEA64)
	hid0 = mfspr(SPR_HID0);
#elif defined (PPC_OEA64_BRIDGE)
	hid0 = mfspr(SPR_HID0);
#endif

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
	case MPC750:
	case IBM750FX:
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

	case MPC7447A:
	case MPC7448:
	case MPC7457:
	case MPC7455:
	case MPC7450:
		/* Enable the 7450 branch caches */
		hid0 |= HID0_SGE | HID0_BTIC;
		hid0 |= HID0_LRSTK | HID0_FOLD | HID0_BHT;
		/* Disable BTIC on 7450 Rev 2.0 or earlier */
		if (vers == MPC7450 && (pvr & 0xFFFF) <= 0x0200)
			hid0 &= ~HID0_BTIC;
		/* Select NAP mode. */
		hid0 &= ~(HID0_HIGH_BAT_EN | HID0_SLEEP);
		hid0 |= HID0_NAP | HID0_DPM /* | HID0_XBSEN */;
		powersave = 1;
		break;

	case IBM970:
	case IBM970FX:
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

#if defined (PPC_OEA)
	mtspr(SPR_HID0, hid0);
	__asm volatile("sync;isync");
#endif

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
		bitmask = 0;
		break;
	default:
		bitmask = HID0_BITMASK;
		break;
	}
	bitmask_snprintf(hid0, bitmask, hidbuf, sizeof hidbuf);
	aprint_normal("%s: HID0 %s, powersave: %d\n", self->dv_xname, hidbuf, powersave);

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

		if (vers == IBM750FX || vers == MPC750 ||
		    vers == MPC7400  || vers == MPC7410 || MPC745X_P(vers)) {
			if (MPC745X_P(vers)) {
				cpu_config_l3cr(vers);
			} else {
				cpu_config_l2cr(pvr);
			}
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
}

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
	default:
		major = (pvr >>  4) & 0xf;
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
	mtspr(SPR_L2CR, l2cr | L2CR_L2I);
	do {
		x = mfspr(SPR_L2CR);
	} while (x & L2CR_L2IP);

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
	if ((pvr >> 16) == IBM750FX ||
	    (pvr & 0xffffff00) == 0x00082200 /* IBM750CX */ ||
	    (pvr & 0xffffef00) == 0x00082300 /* IBM750CXe */) {
		cpu_fmttab_print(cpu_ibm750_l2cr_formats, l2cr);
	} else {
		cpu_fmttab_print(cpu_l2cr_formats, l2cr);
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

	ci->ci_khz = cps / 1000;
}

#if NSYSMON_ENVSYS > 0
void
cpu_tau_setup(struct cpu_info *ci)
{
	struct {
		struct sysmon_envsys sme;
		envsys_data_t edata;
	} *datap;
	int error;

	datap = malloc(sizeof(*datap), M_DEVBUF, M_WAITOK | M_ZERO);

	datap->edata.sensor = 0;
	datap->edata.units = ENVSYS_STEMP;
	datap->edata.state = ENVSYS_SVALID;
	(void)strlcpy(datap->edata.desc, "CPU Temp",
	    sizeof(datap->edata.desc));

	ci->ci_sysmon_cookie = &datap->sme;
	datap->sme.sme_nsensors = 1;
	datap->sme.sme_sensor_data = &datap->edata;
	datap->sme.sme_name = ci->ci_dev->dv_xname;	
	datap->sme.sme_cookie = ci;
	datap->sme.sme_gtredata = cpu_tau_gtredata;

	if ((error = sysmon_envsys_register(&datap->sme)) != 0)
		aprint_error("%s: unable to register with sysmon (%d)\n",
		    ci->ci_dev->dv_xname, error);
}


/* Find the temperature of the CPU. */
int
cpu_tau_gtredata(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	int i, threshold, count;

	if (edata->sensor != 0) {
		edata->state = ENVSYS_SINVALID;
		return 0;
	}
	
	threshold = 64; /* Half of the 7-bit sensor range */
	mtspr(SPR_THRM1, 0);
	mtspr(SPR_THRM2, 0);
	/* XXX This counter is supposed to be "at least 20 microseonds, in
	 * XXX units of clock cycles". Since we don't have convenient
	 * XXX access to the CPU speed, set it to a conservative value,
	 * XXX that is, assuming a fast (1GHz) G3 CPU (As of February 2002,
	 * XXX the fastest G3 processor is 700MHz) . The cost is that
	 * XXX measuring the temperature takes a bit longer.
	 */
        mtspr(SPR_THRM3, SPR_THRM_TIMER(20000) | SPR_THRM_ENABLE); 

	/* Successive-approximation code adapted from Motorola
	 * application note AN1800/D, "Programming the Thermal Assist
	 * Unit in the MPC750 Microprocessor".
	 */
	for (i = 4; i >= 0 ; i--) {
		mtspr(SPR_THRM1, 
		    SPR_THRM_THRESHOLD(threshold) | SPR_THRM_VALID);
		count = 0;
		while ((count < 100) && 
		    ((mfspr(SPR_THRM1) & SPR_THRM_TIV) == 0)) {
			count++;
			delay(1);
		}
		if (mfspr(SPR_THRM1) & SPR_THRM_TIN) {
			/* The interrupt bit was set, meaning the 
			 * temperature was above the threshold 
			 */
			threshold += 2 << i;
		} else {
			/* Temperature was below the threshold */
			threshold -= 2 << i;
		}
	}
	threshold += 2;

	/* Convert the temperature in degrees C to microkelvin */
	sme->sme_sensor_data->value_cur = (threshold * 1000000) + 273150000;
	
	return 0;
}
#endif /* NSYSMON_ENVSYS > 0 */
