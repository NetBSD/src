/*	$NetBSD: cpu_subr.c,v 1.14.4.2 2002/06/20 02:53:03 lukem Exp $	*/

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

#include "opt_l2cr_config.h"
#include "opt_multiprocessor.h"
#include "sysmon_envsys.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>
#include <powerpc/mpc6xx/hid.h>
#include <powerpc/mpc6xx/hid_601.h>
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
struct cpu_info cpu_info_store;
#endif

char cpu_model[80];

void
cpu_probe_cache(void)
{
	u_int assoc, pvr, vers;

	__asm __volatile ("mfpvr %0" : "=r"(pvr));
	vers = pvr >> 16;

	switch (vers) {
#define	K	*1024
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
	u_int hid0, pvr, vers;
	char model[80];

	ncpus++;
#ifdef MULTIPROCESSOR
	ci = &cpu_info[id];
#else
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

	ci = &cpu_info_store;
#endif

	ci->ci_cpuid = id;
	ci->ci_intrdepth = -1;
	ci->ci_dev = self;

	__asm __volatile ("mfpvr %0" : "=r"(pvr));
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
			__asm __volatile ("mtspr %1,%0" :: "r"(id), "n"(SPR_PIR));
		}
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
	cpu_identify(model, sizeof(model));
	printf(": %s, ID %d%s\n", model,  cpu_number(),
	    id == 0 ? " (primary)" : "");

	__asm __volatile("mfspr %0,%1" : "=r"(hid0) : "n"(SPR_HID0));
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
		powersave = 1;
		break;

	default:
		/* No power-saving mode is available. */
	}

#ifdef NAPMODE
	switch (vers) {
	case MPC750:
	case MPC7400:
		/* Select NAP mode. */
		hid0 &= ~(HID0_DOZE | HID0_NAP | HID0_SLEEP);
		hid0 |= HID0_NAP;
		break;
	}
#endif

	switch (vers) {
	case MPC750:
		hid0 &= ~HID0_DBP;		/* XXX correct? */
		hid0 |= HID0_EMCP | HID0_BTIC | HID0_SGE | HID0_BHT;
		break;
#if 0
	case MPC7400:
	case MPC7410:
		hid0 &= ~HID0_SPD;
		hid0 |= HID0_EMCP | HID0_BTIC | HID0_SGE | HID0_BHT;
		hid0 |= HID0_EIEC;
		break;
#endif
	}

	__asm __volatile ("mtspr %1,%0" :: "r"(hid0), "n"(SPR_HID0));

#if 1
	{
		char hidbuf[128];
		char *bitmask;
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
	}
#endif

	/*
	 * Display speed and cache configuration.
	 */
	if (vers == MPC750 || vers == MPC7400 ||
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
	if (vers == MPC750)
		cpu_tau_setup(ci);
#endif

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
	if (vers == MPC7400 || vers == MPC7410 || vers == MPC7450) {
		evcnt_attach_dynamic(&ci->ci_ev_vec, EVCNT_TYPE_TRAP,
			&ci->ci_ev_traps, self->dv_xname,
			"user AltiVec unavailable");
		evcnt_attach_dynamic(&ci->ci_ev_vecsw, EVCNT_TYPE_TRAP,
			&ci->ci_ev_vec, self->dv_xname,
			"user AltiVec context switches");
	}

	return ci;
}

struct cputab {
	int version;
	char *name;
};
static const struct cputab models[] = {
	{ MPC601,     "601" },
	{ MPC602,     "602" },
	{ MPC603,     "603" },
	{ MPC603e,    "603e" },
	{ MPC603ev,   "603ev" },
	{ MPC604,     "604" },
	{ MPC604ev,   "604ev" },
	{ MPC620,     "620" },
	{ MPC750,     "750" },
	{ MPC7400,   "7400" },
	{ MPC7410,   "7410" },
	{ MPC7450,   "7450" },
	{ MPC7455,   "7455" },
	{ MPC8240,   "8240" },
	{ MPC8245,   "8245" },
	{ 0,	       NULL }
};

void
cpu_identify(char *str, size_t len)
{
	u_int pvr, vers, maj, min;
	const struct cputab *cp;

	asm ("mfpvr %0" : "=r"(pvr));
	vers = pvr >> 16;
	switch (vers) {
	case MPC7410:
		min = (pvr >> 0) & 0xff;
		maj = min <= 4 ? 1 : 2;
		break;
	default:
		maj = (pvr >>  8) & 0xff;
		min = (pvr >>  0) & 0xff;
	}

	for (cp = models; cp->name != NULL; cp++) {
		if (cp->version == vers)
			break;
	}

	if (str == NULL) {
		str = cpu_model;
		len = sizeof(cpu_model);
		cpu = vers;
	}

	if (cp->name != NULL) {
		snprintf(str, len, "%s (Revision %u.%u)", cp->name, maj, min);
	} else {
		snprintf(str, len, "Version %x (Revision %u.%u)", vers, maj, min);
	}
}

#ifdef L2CR_CONFIG
u_int l2cr_config = L2CR_CONFIG;
#else
u_int l2cr_config = 0;
#endif

void
cpu_config_l2cr(int vers)
{
	u_int l2cr, x;

	__asm __volatile ("mfspr %0,%1" : "=r"(l2cr) : "n"(SPR_L2CR));

	/*
	 * Configure L2 cache if not enabled.
	 */
	if ((l2cr & L2CR_L2E) == 0 && l2cr_config != 0) {
		l2cr = l2cr_config;
		asm volatile ("mtspr %1,%0" :: "r"(l2cr), "n"(SPR_L2CR));

		/* Wait for L2 clock to be stable (640 L2 clocks). */
		delay(100);

		/* Invalidate all L2 contents. */
		l2cr |= L2CR_L2I;
		asm volatile ("mtspr %1,%0" :: "r"(l2cr), "n"(SPR_L2CR));
		do {
			asm volatile ("mfspr %0,%1" : "=r"(x) : "n"(SPR_L2CR));
		} while (x & L2CR_L2IP);

		/* Enable L2 cache. */
		l2cr &= ~L2CR_L2I;
		l2cr |= L2CR_L2E;
		asm volatile ("mtspr %1,%0" :: "r"(l2cr), "n"(SPR_L2CR));
	}

	if (l2cr & L2CR_L2E) {
		if (vers == MPC7450 || vers == MPC7455) {
			u_int l3cr;

			printf(": 256KB L2 cache");

			__asm __volatile("mfspr %0,%1" :
			    "=r"(l3cr) : "n"(SPR_L3CR) );
			if (l3cr & L3CR_L3E)
				printf(", %cMB L3 backside cache",
				   l3cr & L3CR_L3SIZ ? '2' : '1');
			printf("\n");
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
#if 0
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
#endif
		printf(" backside cache");
	} else
		printf(": L2 cache not enabled");

	printf("\n");
}

void
cpu_print_speed(void)
{
	u_int64_t cps;

	mtspr(SPR_MMCR0, SPR_MMCR0_FC);
	mtspr(SPR_PMC1, 0);
	mtspr(SPR_MMCR0, SPR_MMCR0_PMC1SEL(PMCN_CYCLES));
	delay(100000);
	cps = (mfspr(SPR_PMC1) * 10) + 4999;

	printf(": %qd.%02qd MHz\n", cps / 1000000, (cps / 10000) % 100);
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
