/* $NetBSD: cpu.c,v 1.106 2022/04/09 23:39:18 riastradh Exp $ */

/*-
 * Copyright (c) 1998, 1999, 2000, 2001, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.106 2022/04/09 23:39:18 riastradh Exp $");

#include "opt_ddb.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/sysctl.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/cpuvar.h>
#include <machine/rpb.h>
#include <machine/prom.h>
#include <machine/alpha.h>

struct cpu_info cpu_info_primary __cacheline_aligned = {
	.ci_curlwp = &lwp0
};
struct cpu_info *cpu_info_list __read_mostly = &cpu_info_primary;

#if defined(MULTIPROCESSOR)
/*
 * Array of CPU info structures.  Must be statically-allocated because
 * curproc, etc. are used early.
 */
struct cpu_info *cpu_info[ALPHA_MAXPROCS];

/* Bitmask of CPUs booted, currently running, and paused. */
volatile u_long cpus_booted __read_mostly;
volatile u_long cpus_running __read_mostly;
volatile u_long cpus_paused __read_mostly;

void	cpu_boot_secondary(struct cpu_info *);
#endif /* MULTIPROCESSOR */

static void
cpu_idle_default(void)
{
	/*
	 * Default is to do nothing.  Platform code can overwrite
	 * as needed.
	 */
}

void
cpu_idle_wtint(void)
{
	/*
	 * Some PALcode versions implement the WTINT call to idle
	 * in a low power mode.
	 */
	alpha_pal_wtint(0);
}

void	(*cpu_idle_fn)(void) __read_mostly = cpu_idle_default;

/*
 * The Implementation Version and the Architecture Mask must be
 * consistent across all CPUs in the system, so we set it for the
 * primary and announce the AMASK extensions if they exist.
 *
 * Note, we invert the AMASK so that if a bit is set, it means "has
 * extension".
 */
u_long	cpu_implver __read_mostly;
u_long	cpu_amask __read_mostly;

/* Definition of the driver for autoconfig. */
static int	cpumatch(device_t, cfdata_t, void *);
static void	cpuattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, sizeof(struct cpu_softc),
    cpumatch, cpuattach, NULL, NULL);

static void	cpu_announce_extensions(struct cpu_info *);

extern struct cfdriver cpu_cd;

static const char * const lcaminor[] = {
	"",
	"21066", "21066",
	"21068", "21068",
	"21066A", "21068A",
	NULL
};

const struct cputable_struct {
	const char *cpu_evname;
	const char *cpu_major_name;
	const char * const *cpu_minor_names;
} cpunametable[] = {
[PCS_PROC_EV3]       ={	"EV3",		NULL,		NULL		},
[PCS_PROC_EV4]       ={	"EV4",		"21064",	NULL		},
[PCS_PROC_SIMULATION]={ "Sim",		NULL,		NULL		},
[PCS_PROC_LCA4]      ={	"LCA4",		NULL,		lcaminor	},
[PCS_PROC_EV5]       ={	"EV5",		"21164",	NULL		},
[PCS_PROC_EV45]      ={	"EV45",		"21064A",	NULL		},
[PCS_PROC_EV56]      ={	"EV56",		"21164A",	NULL		},
[PCS_PROC_EV6]       ={	"EV6",		"21264",	NULL		},
[PCS_PROC_PCA56]     ={	"PCA56",	"21164PC",	NULL		},
[PCS_PROC_PCA57]     ={	"PCA57",	"21164PC"/*XXX*/,NULL		},
[PCS_PROC_EV67]      ={	"EV67",		"21264A",	NULL		},
[PCS_PROC_EV68CB]    ={	"EV68CB",	"21264C",	NULL		},
[PCS_PROC_EV68AL]    ={	"EV68AL",	"21264B",	NULL		},
[PCS_PROC_EV68CX]    ={	"EV68CX",	"21264D",	NULL		},
[PCS_PROC_EV7]       ={	"EV7",		"21364",	NULL		},
[PCS_PROC_EV79]      ={	"EV79",		NULL,		NULL		},
[PCS_PROC_EV69]      ={	"EV69",		NULL,		NULL		},
};

static bool
cpu_description(const struct cpu_softc * const sc,
    char * const buf, size_t const buflen)
{
	const char * const *s;
	const char *ev;
	int i;

	const uint32_t major = sc->sc_major_type;
	const uint32_t minor = sc->sc_minor_type;

	if (major < __arraycount(cpunametable) &&
	    (ev = cpunametable[major].cpu_evname) != NULL) {
		s = cpunametable[major].cpu_minor_names;
		for (i = 0; s != NULL && s[i] != NULL; i++) {
			if (i == minor && strlen(s[i]) != 0) {
				break;
			}
		}
		if (s == NULL || s[i] == NULL) {
			s = &cpunametable[major].cpu_major_name;
			i = 0;
			if (s[i] == NULL) {
				s = NULL;
			}
		}

		/*
		 * Example strings:
		 *
		 *	Sim-0
		 *	21068-3 (LCA4)		[uses minor table]
		 *	21264C-5 (EV68CB)
		 *	21164PC-1 (PCA56)
		 */
		if (s != NULL) {
			snprintf(buf, buflen, "%s-%d (%s)", s[i], minor, ev);
		} else {
			snprintf(buf, buflen, "%s-%d", ev, minor);
		}
		return true;
	}

	snprintf(buf, buflen, "UNKNOWN CPU TYPE (%u:%u)", major, minor);
	return false;
}

static int
cpu_sysctl_model(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	const struct cpu_softc * const sc = node.sysctl_data;
	char model[32];

	cpu_description(sc, model, sizeof(model));
	node.sysctl_data = model;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

static int
cpu_sysctl_amask_bit(SYSCTLFN_ARGS, unsigned long const bit)
{
	struct sysctlnode node = *rnode;
	const struct cpu_softc * const sc = node.sysctl_data;

	bool result = (sc->sc_amask & bit) ? true : false;
	node.sysctl_data = &result;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

static int
cpu_sysctl_bwx(SYSCTLFN_ARGS)
{
	return cpu_sysctl_amask_bit(SYSCTLFN_CALL(rnode), ALPHA_AMASK_BWX);
}

static int
cpu_sysctl_fix(SYSCTLFN_ARGS)
{
	return cpu_sysctl_amask_bit(SYSCTLFN_CALL(rnode), ALPHA_AMASK_FIX);
}

static int
cpu_sysctl_cix(SYSCTLFN_ARGS)
{
	return cpu_sysctl_amask_bit(SYSCTLFN_CALL(rnode), ALPHA_AMASK_CIX);
}

static int
cpu_sysctl_mvi(SYSCTLFN_ARGS)
{
	return cpu_sysctl_amask_bit(SYSCTLFN_CALL(rnode), ALPHA_AMASK_MVI);
}

static int
cpu_sysctl_pat(SYSCTLFN_ARGS)
{
	return cpu_sysctl_amask_bit(SYSCTLFN_CALL(rnode), ALPHA_AMASK_PAT);
}

static int
cpu_sysctl_pmi(SYSCTLFN_ARGS)
{
	return cpu_sysctl_amask_bit(SYSCTLFN_CALL(rnode), ALPHA_AMASK_PMI);
}

static int
cpu_sysctl_primary(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	const struct cpu_softc * const sc = node.sysctl_data;

	bool result = CPU_IS_PRIMARY(sc->sc_ci);
	node.sysctl_data = &result;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

/*
 * The following is an attempt to map out how booting secondary CPUs
 * works.
 *
 * As we find processors during the autoconfiguration sequence, all
 * processors have idle stacks and PCBs created for them, including
 * the primary (although the primary idles on lwp0's PCB until its
 * idle PCB is created).
 *
 * Right before calling uvm_scheduler(), main() calls, on lwp0's
 * context, cpu_boot_secondary_processors().  This is our key to
 * actually spin up the additional processor's we've found.  We
 * run through our cpu_info[] array looking for secondary processors
 * with idle PCBs, and spin them up.
 *
 * The spinup involves switching the secondary processor to the
 * OSF/1 PALcode, setting the entry point to cpu_spinup_trampoline(),
 * and sending a "START" message to the secondary's console.
 *
 * Upon successful processor bootup, the cpu_spinup_trampoline will call
 * cpu_hatch(), which will print a message indicating that the processor
 * is running, and will set the "hatched" flag in its softc.  At the end
 * of cpu_hatch() is a spin-forever loop; we do not yet attempt to schedule
 * anything on secondary CPUs.
 */

static int
cpumatch(device_t parent, cfdata_t cfdata, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	/* make sure that we're looking for a CPU. */
	if (strcmp(ma->ma_name, cpu_cd.cd_name) != 0)
		return (0);

	/* XXX CHECK SLOT? */
	/* XXX CHECK PRIMARY? */

	return (1);
}

static void
cpuattach(device_t parent, device_t self, void *aux)
{
	struct cpu_softc * const sc = device_private(self);
	const struct mainbus_attach_args * const ma = aux;
	struct cpu_info *ci;
	char model[32];

	const bool primary = ma->ma_slot == hwrpb->rpb_primary_cpu_id;

	sc->sc_dev = self;

	const struct pcs * const p = LOCATE_PCS(hwrpb, ma->ma_slot);
	sc->sc_major_type = PCS_CPU_MAJORTYPE(p);
	sc->sc_minor_type = PCS_CPU_MINORTYPE(p);

	const bool recognized = cpu_description(sc, model, sizeof(model));

	aprint_normal(": ID %d%s, ", ma->ma_slot, primary ? " (primary)" : "");
	if (recognized) {
		aprint_normal("%s", model);
	} else {
		aprint_error("%s", model);
	}

	aprint_naive("\n");
	aprint_normal("\n");

	if (p->pcs_proc_var != 0) {
		bool needcomma = false;
		const char *vaxfp = "";
		const char *ieeefp = "";
		const char *pe = "";

		if (p->pcs_proc_var & PCS_VAR_VAXFP) {
			sc->sc_vax_fp = true;
			vaxfp = "VAX FP support";
			needcomma = true;
		}
		if (p->pcs_proc_var & PCS_VAR_IEEEFP) {
			sc->sc_ieee_fp = true;
			ieeefp = ", IEEE FP support";
			if (!needcomma)
				ieeefp += 2;
			needcomma = true;
		}
		if (p->pcs_proc_var & PCS_VAR_PE) {
			sc->sc_primary_eligible = true;
			pe = ", Primary Eligible";
			if (!needcomma)
				pe += 2;
			needcomma = true;
		}
		aprint_debug_dev(sc->sc_dev, "%s%s%s", vaxfp, ieeefp, pe);
		if (p->pcs_proc_var & PCS_VAR_RESERVED)
			aprint_debug("%sreserved bits: %#lx",
			    needcomma ? ", " : "",
			    p->pcs_proc_var & PCS_VAR_RESERVED);
		aprint_debug("\n");
	}

	if (ma->ma_slot > ALPHA_WHAMI_MAXID) {
		if (primary)
			panic("cpu_attach: primary CPU ID too large");
		aprint_error_dev(sc->sc_dev,
		    "processor ID too large, ignoring\n");
		return;
	}

	if (primary) {
		ci = &cpu_info_primary;
	} else {
		/*
		 * kmem_zalloc() will guarante cache line alignment for
		 * all allocations >= CACHE_LINE_SIZE.
		 */
		ci = kmem_zalloc(sizeof(*ci), KM_SLEEP);
		KASSERT(((uintptr_t)ci & (CACHE_LINE_SIZE - 1)) == 0);
	}
#if defined(MULTIPROCESSOR)
	cpu_info[ma->ma_slot] = ci;
#endif
	ci->ci_cpuid = ma->ma_slot;
	ci->ci_softc = sc;
	ci->ci_pcc_freq = hwrpb->rpb_cc_freq;

	sc->sc_ci = ci;

#if defined(MULTIPROCESSOR)
	/*
	 * Make sure the processor is available for use.
	 */
	if ((p->pcs_flags & PCS_PA) == 0) {
		if (primary)
			panic("cpu_attach: primary not available?!");
		aprint_normal_dev(sc->sc_dev,
		    "processor not available for use\n");
		return;
	}

	/* Make sure the processor has valid PALcode. */
	if ((p->pcs_flags & PCS_PV) == 0) {
		if (primary)
			panic("cpu_attach: primary has invalid PALcode?!");
		aprint_error_dev(sc->sc_dev, "PALcode not valid\n");
		return;
	}
#endif /* MULTIPROCESSOR */

	/*
	 * If we're the primary CPU, no more work to do; we're already
	 * running!
	 */
	if (primary) {
		cpu_announce_extensions(ci);
#if defined(MULTIPROCESSOR)
		ci->ci_flags |= CPUF_PRIMARY|CPUF_RUNNING;
		atomic_or_ulong(&cpus_booted, (1UL << ma->ma_slot));
		atomic_or_ulong(&cpus_running, (1UL << ma->ma_slot));
#endif /* MULTIPROCESSOR */
	} else {
#if defined(MULTIPROCESSOR)
		int error;

		error = mi_cpu_attach(ci);
		if (error != 0) {
			aprint_error_dev(sc->sc_dev,
			    "mi_cpu_attach failed with %d\n", error);
			return;
		}

		/*
		 * Boot the secondary processor.  It will announce its
		 * extensions, and then spin until we tell it to go
		 * on its merry way.
		 */
		cpu_boot_secondary(ci);

		/*
		 * Link the processor into the list.
		 */
		ci->ci_next = cpu_info_list->ci_next;
		cpu_info_list->ci_next = ci;
#else /* ! MULTIPROCESSOR */
		aprint_normal_dev(sc->sc_dev, "processor off-line; "
		    "multiprocessor support not present in kernel\n");
#endif /* MULTIPROCESSOR */
	}

	evcnt_attach_dynamic(&sc->sc_evcnt_clock, EVCNT_TYPE_INTR,
	    NULL, device_xname(sc->sc_dev), "clock");
	evcnt_attach_dynamic(&sc->sc_evcnt_device, EVCNT_TYPE_INTR,
	    NULL, device_xname(sc->sc_dev), "device");
#if defined(MULTIPROCESSOR)
	alpha_ipi_init(ci);
#endif

	struct sysctllog **log = &sc->sc_sysctllog;
	const struct sysctlnode *rnode, *cnode;
	int error;

	error = sysctl_createv(log, 0, NULL, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("cpu properties"),
	    NULL, 0,
	    NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_STRING, "model",
	    SYSCTL_DESCR("cpu model"),
	    cpu_sysctl_model, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_INT, "major",
	    SYSCTL_DESCR("cpu major type"),
	    NULL, 0,
	    &sc->sc_major_type, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_INT, "minor",
	    SYSCTL_DESCR("cpu minor type"),
	    NULL, 0,
	    &sc->sc_minor_type, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_LONG, "implver",
	    SYSCTL_DESCR("cpu implementation version"),
	    NULL, 0,
	    &sc->sc_implver, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_HEX, CTLTYPE_LONG, "amask",
	    SYSCTL_DESCR("architecture extensions mask"),
	    NULL, 0,
	    &sc->sc_amask, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_BOOL, "bwx",
	    SYSCTL_DESCR("cpu supports BWX extension"),
	    cpu_sysctl_bwx, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_BOOL, "fix",
	    SYSCTL_DESCR("cpu supports FIX extension"),
	    cpu_sysctl_fix, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_BOOL, "cix",
	    SYSCTL_DESCR("cpu supports CIX extension"),
	    cpu_sysctl_cix, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_BOOL, "mvi",
	    SYSCTL_DESCR("cpu supports MVI extension"),
	    cpu_sysctl_mvi, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_BOOL, "pat",
	    SYSCTL_DESCR("cpu supports PAT extension"),
	    cpu_sysctl_pat, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_BOOL, "pmi",
	    SYSCTL_DESCR("cpu supports PMI extension"),
	    cpu_sysctl_pmi, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_BOOL, "vax_fp",
	    SYSCTL_DESCR("cpu supports VAX FP"),
	    NULL, 0,
	    &sc->sc_vax_fp, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_BOOL, "ieee_fp",
	    SYSCTL_DESCR("cpu supports IEEE FP"),
	    NULL, 0,
	    &sc->sc_ieee_fp, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_BOOL, "primary_eligible",
	    SYSCTL_DESCR("cpu is primary-eligible"),
	    NULL, 0,
	    &sc->sc_primary_eligible, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_BOOL, "primary",
	    SYSCTL_DESCR("cpu is the primary cpu"),
	    cpu_sysctl_primary, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_LONG, "cpu_id",
	    SYSCTL_DESCR("hardware cpu ID"),
	    NULL, 0,
	    &sc->sc_ci->ci_cpuid, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_LONG, "pcc_freq",
	    SYSCTL_DESCR("PCC frequency"),
	    NULL, 0,
	    &sc->sc_ci->ci_pcc_freq, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return;
}

static void
cpu_announce_extensions(struct cpu_info *ci)
{
	u_long implver, amask = 0;
	char bits[64];

	implver = alpha_implver();
	if (implver >= ALPHA_IMPLVER_EV5)
		amask = (~alpha_amask(ALPHA_AMASK_ALL)) & ALPHA_AMASK_ALL;

	ci->ci_softc->sc_implver = implver;
	ci->ci_softc->sc_amask = amask;

	if (ci->ci_cpuid == hwrpb->rpb_primary_cpu_id) {
		cpu_implver = implver;
		cpu_amask = amask;
	} else {
		if (implver < cpu_implver)
			aprint_error_dev(ci->ci_softc->sc_dev,
			    "WARNING: IMPLVER %lu < %lu\n",
			    implver, cpu_implver);

		/*
		 * Cap the system architecture mask to the intersection
		 * of features supported by all processors in the system.
		 */
		cpu_amask &= amask;
	}

	if (amask) {
		snprintb(bits, sizeof(bits),
		    ALPHA_AMASK_BITS, amask);
		aprint_normal_dev(ci->ci_softc->sc_dev,
		    "Architecture extensions: %s\n", bits);
	}
}

#if defined(MULTIPROCESSOR)
void
cpu_boot_secondary_processors(void)
{
	struct cpu_info *ci;
	u_long i;
	bool did_patch = false;

	for (i = 0; i < ALPHA_MAXPROCS; i++) {
		ci = cpu_info[i];
		if (ci == NULL || ci->ci_data.cpu_idlelwp == NULL)
			continue;
		if (CPU_IS_PRIMARY(ci))
			continue;
		if ((cpus_booted & (1UL << i)) == 0)
			continue;

		/* Patch MP-criticial kernel routines. */
		if (did_patch == false) {
			alpha_patch(true);
			did_patch = true;
		}

		/*
		 * Launch the processor.
		 */
		atomic_or_ulong(&ci->ci_flags, CPUF_RUNNING);
		atomic_or_ulong(&cpus_running, (1U << i));
	}
}

void
cpu_boot_secondary(struct cpu_info *ci)
{
	long timeout;
	struct pcs *pcsp, *primary_pcsp;
	struct pcb *pcb;
	u_long cpumask;

	pcb = lwp_getpcb(ci->ci_data.cpu_idlelwp);
	primary_pcsp = LOCATE_PCS(hwrpb, hwrpb->rpb_primary_cpu_id);
	pcsp = LOCATE_PCS(hwrpb, ci->ci_cpuid);
	cpumask = (1UL << ci->ci_cpuid);

	/*
	 * Set up the PCS's HWPCB to match ours.
	 */
	memcpy(pcsp->pcs_hwpcb, &pcb->pcb_hw, sizeof(pcb->pcb_hw));

	/*
	 * Set up the HWRPB to restart the secondary processor
	 * with our spin-up trampoline.
	 */
	hwrpb->rpb_restart = (uint64_t) cpu_spinup_trampoline;
	hwrpb->rpb_restart_val = (uint64_t) ci;
	hwrpb->rpb_checksum = hwrpb_checksum();

	/*
	 * Configure the CPU to start in OSF/1 PALcode by copying
	 * the primary CPU's PALcode revision info to the secondary
	 * CPUs PCS.
	 */
	memcpy(&pcsp->pcs_pal_rev, &primary_pcsp->pcs_pal_rev,
	    sizeof(pcsp->pcs_pal_rev));
	pcsp->pcs_flags |= (PCS_CV|PCS_RC);
	pcsp->pcs_flags &= ~PCS_BIP;

	/* Make sure the secondary console sees all this. */
	alpha_mb();

	/* Send a "START" command to the secondary CPU's console. */
	if (cpu_iccb_send(ci->ci_cpuid, "START\r\n")) {
		aprint_error_dev(ci->ci_softc->sc_dev,
		    "unable to issue `START' command\n");
		return;
	}

	/* Wait for the processor to boot. */
	for (timeout = 10000; timeout != 0; timeout--) {
		alpha_mb();
		if (pcsp->pcs_flags & PCS_BIP)
			break;
		delay(1000);
	}
	if (timeout == 0)
		aprint_error_dev(ci->ci_softc->sc_dev,
		    "processor failed to boot\n");

	/*
	 * ...and now wait for verification that it's running kernel
	 * code.
	 */
	for (timeout = 10000; timeout != 0; timeout--) {
		alpha_mb();
		if (cpus_booted & cpumask)
			break;
		delay(1000);
	}
	if (timeout == 0)
		aprint_error_dev(ci->ci_softc->sc_dev,
		    "processor failed to hatch\n");
}

void
cpu_pause_resume(u_long cpu_id, int pause)
{
	u_long cpu_mask = (1UL << cpu_id);

	if (pause) {
		atomic_or_ulong(&cpus_paused, cpu_mask);
		alpha_send_ipi(cpu_id, ALPHA_IPI_PAUSE);
	} else
		atomic_and_ulong(&cpus_paused, ~cpu_mask);
}

void
cpu_pause_resume_all(int pause)
{
	struct cpu_info *ci, *self = curcpu();
	CPU_INFO_ITERATOR cii;

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci == self)
			continue;
		cpu_pause_resume(ci->ci_cpuid, pause);
	}
}

void
cpu_halt(void)
{
	struct cpu_info *ci = curcpu();
	u_long cpu_id = cpu_number();
	struct pcs *pcsp = LOCATE_PCS(hwrpb, cpu_id);

	aprint_normal_dev(ci->ci_softc->sc_dev, "shutting down...\n");

	pcsp->pcs_flags &= ~(PCS_RC | PCS_HALT_REQ);
	pcsp->pcs_flags |= PCS_HALT_STAY_HALTED;

	atomic_and_ulong(&cpus_running, ~(1UL << cpu_id));
	atomic_and_ulong(&cpus_booted, ~(1U << cpu_id));

	alpha_pal_halt();
	/* NOTREACHED */
}

void
cpu_hatch(struct cpu_info *ci)
{
	u_long cpu_id = cpu_number();
	u_long cpumask = (1UL << cpu_id);

	/* pmap initialization for this processor. */
	pmap_init_cpu(ci);

	/* Initialize trap vectors for this processor. */
	trap_init();

	/* Yahoo!  We're running kernel code!  Announce it! */
	cpu_announce_extensions(ci);

	atomic_or_ulong(&cpus_booted, cpumask);

	/*
	 * Spin here until we're told we can start.
	 */
	while ((cpus_running & cpumask) == 0)
		/* spin */ ;

	/*
	 * Invalidate the TLB and sync the I-stream before we
	 * jump into the kernel proper.  We have to do this
	 * beacause we haven't been getting IPIs while we've
	 * been spinning.
	 */
	ALPHA_TBIA();
	alpha_pal_imb();

	if (alpha_use_cctr) {
		cc_init_secondary(ci);
	}

	cpu_initclocks_secondary();
}

int
cpu_iccb_send(long cpu_id, const char *msg)
{
	struct pcs *pcsp = LOCATE_PCS(hwrpb, cpu_id);
	int timeout;
	u_long cpumask = (1UL << cpu_id);

	/* Wait for the ICCB to become available. */
	for (timeout = 10000; timeout != 0; timeout--) {
		alpha_mb();
		if ((hwrpb->rpb_rxrdy & cpumask) == 0)
			break;
		delay(1000);
	}
	if (timeout == 0)
		return (EIO);

	/*
	 * Copy the message into the ICCB, and tell the secondary console
	 * that it's there.  Ensure the buffer is initialized before we
	 * set the rxrdy bits, as a store-release.
	 */
	strcpy(pcsp->pcs_iccb.iccb_rxbuf, msg);
	pcsp->pcs_iccb.iccb_rxlen = strlen(msg);
	membar_release();
	atomic_or_ulong(&hwrpb->rpb_rxrdy, cpumask);

	/* Wait for the message to be received. */
	for (timeout = 10000; timeout != 0; timeout--) {
		alpha_mb();
		if ((hwrpb->rpb_rxrdy & cpumask) == 0)
			break;
		delay(1000);
	}
	if (timeout == 0)
		return (EIO);

	return (0);
}

void
cpu_iccb_receive(void)
{
#if 0	/* Don't bother... we don't get any important messages anyhow. */
	uint64_t txrdy;
	char *cp1, *cp2, buf[80];
	struct pcs *pcsp;
	u_int cnt;
	long cpu_id;

	txrdy = hwrpb->rpb_txrdy;

	for (cpu_id = 0; cpu_id < hwrpb->rpb_pcs_cnt; cpu_id++) {
		if (txrdy & (1UL << cpu_id)) {
			pcsp = LOCATE_PCS(hwrpb, cpu_id);
			printf("Inter-console message from CPU %lu "
			    "HALT REASON = 0x%lx, FLAGS = 0x%lx\n",
			    cpu_id, pcsp->pcs_halt_reason, pcsp->pcs_flags);
			
			cnt = pcsp->pcs_iccb.iccb_txlen;
			if (cnt >= 80) {
				printf("Malformed inter-console message\n");
				continue;
			}
			cp1 = pcsp->pcs_iccb.iccb_txbuf;
			cp2 = buf;
			while (cnt--) {
				if (*cp1 != '\r' && *cp1 != '\n')
					*cp2++ = *cp1;
				cp1++;
			}
			*cp2 = '\0';
			printf("Message from CPU %lu: %s\n", cpu_id, buf);
		}
	}
#endif /* 0 */
	hwrpb->rpb_txrdy = 0;
	alpha_mb();
}

#if defined(DDB)

#include <ddb/db_output.h>
#include <machine/db_machdep.h>

/*
 * Dump CPU information from DDB.
 */
void
cpu_debug_dump(void)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	db_printf("addr		dev	id	flags	ipis	curproc\n");
	for (CPU_INFO_FOREACH(cii, ci)) {
		db_printf("%p	%s	%lu	%lx	%lx	%p\n",
		    ci,
		    device_xname(ci->ci_softc->sc_dev),
		    ci->ci_cpuid,
		    ci->ci_flags,
		    ci->ci_ipis,
		    ci->ci_curlwp);
	}
}

#endif /* DDB */

#endif /* MULTIPROCESSOR */
