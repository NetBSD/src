/*	$NetBSD: cpu_subr.c,v 1.1.4.4 2002/02/11 20:08:52 jdolecek Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>
#include <powerpc/mpc6xx/hid.h>
#include <powerpc/mpc6xx/hid_601.h>
#include <powerpc/spr.h>

static void cpu_config_l2cr(int);

int cpu;
int ncpus;

#ifdef MULTIPROCESSOR
struct cpu_info cpu_info[CPU_MAXNUM];
#else
struct cpu_info cpu_info_store;
#endif

char cpu_model[80];

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
		case MPC604:
		case MPC604ev:
		case MPC7400:
		case MPC7410:
		case MPC7450:
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
		/* Select DOZE mode. */
		hid0 &= ~(HID0_DOZE | HID0_NAP | HID0_SLEEP);
		hid0 |= HID0_DOZE | HID0_DPM;
		powersave = 1;
		break;

	case MPC7450:
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
	case MPC7400:
	case MPC7410:
		hid0 &= ~HID0_SPD;
		hid0 |= HID0_EMCP | HID0_BTIC | HID0_SGE | HID0_BHT;
		hid0 |= HID0_EIEC;
		break;
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
			bitmask = HID0_7450_BITMASK;
			break;
		default:
			bitmask = HID0_BITMASK;
		}
		bitmask_snprintf(hid0, bitmask, hidbuf, sizeof hidbuf);
		printf("%s: HID0 %s\n", self->dv_xname, hidbuf);
	}
#endif

	/*
	 * Display cache configuration.
	 */
	if (vers == MPC750 || vers == MPC7400 ||
	    vers == MPC7410 || vers == MPC7450) {
		printf("%s", self->dv_xname);
		cpu_config_l2cr(vers);
	}

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
	{ MPC8240,   "8240" },
	{ 0,	       NULL }
};

void
cpu_identify(char *str, size_t len)
{
	u_int pvr, vers, rev;
	const struct cputab *cp;

	asm ("mfpvr %0" : "=r"(pvr));
	vers = pvr >> 16;
	rev = pvr & 0xffff;

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
		snprintf(str, len, "%s (Revision %x)", cp->name, rev);
	} else {
		snprintf(str, len, "Version %x (Revision %x)", vers, rev);
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
