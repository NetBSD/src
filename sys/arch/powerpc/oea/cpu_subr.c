/*	$NetBSD: cpu_subr.c,v 1.2 2003/02/26 21:05:23 jklos Exp $	*/

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

#include "opt_ppcparam.h"
#include "opt_multiprocessor.h"
#include "opt_altivec.h"
#include "sysmon_envsys.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <powerpc/oea/hid.h>
#include <powerpc/oea/hid_601.h>
#include <powerpc/spr.h>

#include <dev/sysmon/sysmonvar.h>

static void cpu_config_l2cr(int);
static void cpu_print_speed(void);
#if NSYSMON_ENVSYS > 0
static void cpu_tau_setup(struct cpu_info *);
static int cpu_tau_gtredata __P((struct sysmon_envsys *,
    struct envsys_tre_data *));
static int cpu_tau_streinfo __P((struct sysmon_envsys *,
    struct envsys_basic_info *));
#endif

int cpu;
int ncpus;

#ifdef MULTIPROCESSOR
struct cpu_info cpu_info[CPU_MAXNUM];
#else
struct cpu_info cpu_info[1];
#endif

int cpu_altivec;
char cpu_model[80];

void
cpu_probe_cache(void)
{
	u_int assoc, pvr, vers;

	pvr = mfpvr();
	vers = pvr >> 16;

	switch (vers) {
#define	K	*1024
	case IBM750FX:
	case MPC601:
	case MPC750:
	case MPC7450:
	case MPC7455:
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
		curcpu()->ci_ci.dcache_size = 16 K;
		curcpu()->ci_ci.icache_size = 16 K;
		assoc = 4;
		break;
	case MPC604ev:
		curcpu()->ci_ci.dcache_size = 32 K;
		curcpu()->ci_ci.icache_size = 32 K;
		assoc = 4;
		break;
	default:
		curcpu()->ci_ci.dcache_size = NBPG;
		curcpu()->ci_ci.icache_size = NBPG;
		assoc = 1;
#undef	K
	}

	/* Presently common across all implementations. */
	curcpu()->ci_ci.dcache_line_size = CACHELINESIZE;
	curcpu()->ci_ci.icache_line_size = CACHELINESIZE;

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

	ncpus++;
	ci = &cpu_info[id];
#ifndef MULTIPROCESSOR
	/*
	 * If this isn't the primary CPU, print an error message
	 * and just bail out.
	 */
	if (id != 0) {
		printf(": ID %d\n", id);
		printf("%s: processor off-line; multiprocessor support "
		    "not present in kernel\n", self->dv_xname);
		return (NULL);
	}
#endif

	ci->ci_cpuid = id;
	ci->ci_intrdepth = -1;
	ci->ci_dev = self;

	pvr = mfpvr();
	vers = (pvr >> 16) & 0xffff;

	switch (id) {
	case 0:
		/* load my cpu_number to PIR */
		switch (vers) {
		case MPC601:
		case MPC604:
		case MPC604ev:
		case MPC7400:
		case MPC7410:
		case MPC7450:
		case MPC7455:
			mtspr(SPR_PIR, id);
		}
		cpu_setup(self, ci);
		break;
	default:
		if (id >= CPU_MAXNUM) {
			printf(": more than %d cpus?\n", CPU_MAXNUM);
			panic("cpuattach");
		}
#ifndef MULTIPROCESSOR
		printf(" not configured\n");
		return NULL;
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
	char *bitmask, hidbuf[128];
	char model[80];

	pvr = mfpvr();
	vers = (pvr >> 16) & 0xffff;

	cpu_identify(model, sizeof(model));
	printf(": %s, ID %d%s\n", model,  cpu_number(),
	    cpu_number() == 0 ? " (primary)" : "");

	hid0 = mfspr(SPR_HID0);
	cpu_probe_cache();

	/*
	 * Configure power-saving mode.
	 */
	switch (vers) {
	case MPC603:
	case MPC603e:
	case MPC603ev:
	case MPC604ev:
	case MPC750:
	case IBM750FX:
	case MPC7400:
	case MPC7410:
	case MPC8240:
	case MPC8245:
		/* Select DOZE mode. */
		hid0 &= ~(HID0_DOZE | HID0_NAP | HID0_SLEEP);
		hid0 |= HID0_DOZE | HID0_DPM;
		powersave = 1;
		break;

	case MPC7455:
	case MPC7450:
		/* Disable BTIC on 7450 Rev 2.0 or earlier */
		if ((pvr >> 16) == MPC7450 && (pvr & 0xFFFF) <= 0x0200)
			hid0 &= ~HID0_BTIC;
		/* Select NAP mode. */
		hid0 &= ~(HID0_DOZE | HID0_NAP | HID0_SLEEP);
		hid0 |= HID0_NAP | HID0_DPM;
		powersave = 0;		/* but don't use it */
		break;

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

	mtspr(SPR_HID0, hid0);

	switch (vers) {
	case MPC601:
		bitmask = HID0_601_BITMASK;
		break;
	case MPC7450:
	case MPC7455:
		bitmask = HID0_7450_BITMASK;
		break;
	default:
		bitmask = HID0_BITMASK;
		break;
	}
	bitmask_snprintf(hid0, bitmask, hidbuf, sizeof hidbuf);
	printf("%s: HID0 %s\n", self->dv_xname, hidbuf);

	/*
	 * Display speed and cache configuration.
	 */
	if (vers == MPC750 || vers == MPC7400 || vers == IBM750FX ||
	    vers == MPC7410 || vers == MPC7450 || vers == MPC7455) {
		printf("%s", self->dv_xname);
		cpu_print_speed();
		printf("%s", self->dv_xname);
		cpu_config_l2cr(vers);
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
	{ "604",	MPC604,		REVFMT_MAJMIN },
	{ "604ev",	MPC604ev,	REVFMT_MAJMIN },
	{ "620",	MPC620,  	REVFMT_HEX },
	{ "750",	MPC750,		REVFMT_MAJMIN },
	{ "750FX",	IBM750FX,	REVFMT_MAJMIN },
	{ "7400",	MPC7400,	REVFMT_MAJMIN },
	{ "7410",	MPC7410,	REVFMT_MAJMIN },
	{ "7450",	MPC7450,	REVFMT_MAJMIN },
	{ "7455",	MPC7455,	REVFMT_MAJMIN },
	{ "8240",	MPC8240,	REVFMT_MAJMIN },
	{ "",		0,		REVFMT_HEX }
};

void
cpu_identify(char *str, size_t len)
{
	u_int pvr, maj, min;
	uint16_t vers, rev, revfmt;
	const struct cputab *cp;
	const char *name;
	size_t n;

	pvr = mfpvr();
	vers = pvr >> 16;
	rev = pvr;
	switch (vers) {
	case MPC7410:
		min = (pvr >> 0) & 0xff;
		maj = min <= 4 ? 1 : 2;
		break;
	default:
		maj = (pvr >>  8) & 0xf;
		min = (pvr >>  0) & 0xf;
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
			snprintf(str + n, len - n, "%u.%u)", maj, min);
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
cpu_config_l2cr(int vers)
{
	u_int l2cr, x, msr;

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
		l2cr = l2cr_config;

		/* Disable interrupts and set the cache config bits. */
		msr = mfmsr();
		mtmsr(msr & ~PSL_EE);
#ifdef ALTIVEC
		if (cpu_altivec)
			__asm __volatile("dssall");
#endif
		__asm __volatile("sync");
		mtspr(SPR_L2CR, l2cr & ~L2CR_L2E);
		__asm __volatile("sync");

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

	if (l2cr & L2CR_L2E) {
		if (vers == MPC7450 || vers == MPC7455) {
			u_int l3cr;

			printf(": 256KB L2 cache");

			l3cr = mfspr(SPR_L3CR);

			/*
			 * Configure L3 cache if not enabled.
			 */
			if ((l3cr & L3CR_L3E) == 0 && l3cr_config != 0) {
				/* By The Book (numbered steps from section 3.7.1.3 of MPC7450UM) */
				
				/* 1: Set all L3CR bits for final config except L3E, L3I, L3PE, and L3CLKEN */
				/*  (also mask off reserved bits in case they were included in L3CR_CONFIG) */
				l3cr = l3cr_config & ~(L3CR_L3E|L3CR_L3I|L3CR_L3PE|L3CR_L3CLKEN|L3CR_RESERVED);
				mtspr(SPR_L3CR, l3cr);

				/* 2: Set L3CR[5] (otherwise reserved bit) to 1 */
				l3cr |= 0x04000000;
				mtspr(SPR_L3CR, l3cr);

				/* 3: Set L3CLKEN to 1*/
				l3cr |= L3CR_L3CLKEN;
				mtspr(SPR_L3CR, l3cr);

				/* 4/5: Perform a global cache invalidate (ref section 3.7.3.6) */
				__asm __volatile("dssall;sync");
				/* L3 cache is already disabled, no need to clear L3E */
				mtspr(SPR_L3CR, l3cr|L3CR_L3I);
				do {
					x = mfspr(SPR_L3CR);
				} while (x & L3CR_L3I);
				
				/* 6: Clear L3CLKEN to 0 */
				l3cr &= ~L3CR_L3CLKEN;
				mtspr(SPR_L3CR, l3cr);

				/* 7: Perform a 'sync' and wait at least 100 CPU cycles */
				__asm __volatile("sync");
				delay(100);

				/* 8: Set L3E and L3CLKEN */
				l3cr |= (L3CR_L3E|L3CR_L3CLKEN);
				mtspr(SPR_L3CR, l3cr);

				/* 9: Perform a 'sync' and wait at least 100 CPU cycles */
				__asm __volatile("sync");
				delay(100);
			}
			
			if (l3cr & L3CR_L3E) {
				printf(", %cMB L3 backside cache at ",
				   l3cr & L3CR_L3SIZ ? '2' : '1');
				switch (l3cr & L3CR_L3CLK) {
				case L3CLK_20:
					printf("2:1 ratio");
					break;
				case L3CLK_25:
					printf("2.5:1 ratio");
					break;
				case L3CLK_30:
					printf("3:1 ratio");
					break;
				case L3CLK_35:
					printf("3.5:1 ratio");
					break;
				case L3CLK_40:
					printf("4:1 ratio");
					break;
				case L3CLK_50:
					printf("5:1 ratio");
					break;
				case L3CLK_60:
					printf("6:1 ratio");
					break;
				default:
					printf("unknown ratio");
					break;
				}
			}
			printf("\n");
			return;
		}
		if (vers == IBM750FX) {
			printf(": 512KB L2 cache\n");
			return;
		}
		switch (l2cr & L2CR_L2SIZ) {
		case L2SIZ_256K:
			printf(": 256KB");
			break;
		case L2SIZ_512K:
			printf(": 512KB");
			break;
		case L2SIZ_1M:
			printf(": 1MB");
			break;
		default:
			printf(": unknown size");
		}
		if (l2cr & L2CR_L2WT) {
			printf(" write-through");
		} else {
			printf(" write-back");
		}
		switch (l2cr & L2CR_L2RAM) {
		case L2RAM_FLOWTHRU_BURST:
			printf(" Flow-through synchronous burst SRAM");
			break;
		case L2RAM_PIPELINE_BURST:
			printf(" Pipelined synchronous burst SRAM");
			break;
		case L2RAM_PIPELINE_LATE:
			printf(" Pipelined synchronous late-write SRAM");
			break;
		default:
			printf(" unknown type");
		}

		if (l2cr & L2CR_L2PE)
			printf(" with parity");
		printf(" backside cache");
	} else
		printf(": L2 cache not enabled");

	printf("\n");
}

void
cpu_print_speed(void)
{
	uint64_t cps;

	mtspr(SPR_MMCR0, SPR_MMCR0_FC);
	mtspr(SPR_PMC1, 0);
	mtspr(SPR_MMCR0, SPR_MMCR0_PMC1SEL(PMCN_CYCLES));
	delay(100000);
	cps = (mfspr(SPR_PMC1) * 10) + 4999;

	printf(": %lld.%02lld MHz\n", cps / 1000000, (cps / 10000) % 100);
}

#if NSYSMON_ENVSYS > 0
const struct envsys_range cpu_tau_ranges[] = {
	{ 0, 0, ENVSYS_STEMP}
};

struct envsys_basic_info cpu_tau_info[] = {
	{ 0, ENVSYS_STEMP, "CPU temp", 0, 0, ENVSYS_FVALID}
};

void
cpu_tau_setup(struct cpu_info *ci)
{
	struct sysmon_envsys *sme;
	int error;

	sme = &ci->ci_sysmon;
	sme->sme_nsensors = 1;
	sme->sme_envsys_version = 1000;
	sme->sme_ranges = cpu_tau_ranges;
	sme->sme_sensor_info = cpu_tau_info;
	sme->sme_sensor_data = &ci->ci_tau_info;
	
	sme->sme_sensor_data->sensor = 0;
	sme->sme_sensor_data->warnflags = ENVSYS_WARN_OK;
	sme->sme_sensor_data->validflags = ENVSYS_FVALID|ENVSYS_FCURVALID;
	sme->sme_cookie = ci;
	sme->sme_gtredata = cpu_tau_gtredata;
	sme->sme_streinfo = cpu_tau_streinfo;

	if ((error = sysmon_envsys_register(sme)) != 0)
		printf("%s: unable to register with sysmon (%d)\n",
		    ci->ci_dev->dv_xname, error);
}


/* Find the temperature of the CPU. */
int
cpu_tau_gtredata(sme, tred)
	 struct sysmon_envsys *sme;
	 struct envsys_tre_data *tred;
{
	struct cpu_info *ci;
	int i, threshold, count;

	if (tred->sensor != 0) {
		tred->validflags = 0;
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

	ci = (struct cpu_info *)sme->sme_cookie;
	/* Convert the temperature in degrees C to microkelvin */
	ci->ci_tau_info.cur.data_us = (threshold * 1000000) + 273150000;
	
	*tred = ci->ci_tau_info;

	return 0;
}

int
cpu_tau_streinfo(sme, binfo)
	 struct sysmon_envsys *sme;
	 struct envsys_basic_info *binfo;
{

	/* There is nothing to set here. */
	return (EINVAL);
}
#endif /* NSYSMON_ENVSYS > 0 */
