/* $NetBSD: cpu.c,v 1.83.2.1 2009/05/13 17:16:04 jym Exp $ */

/*-
 * Copyright (c) 1998, 1999, 2000, 2001 The NetBSD Foundation, Inc.
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

__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.83.2.1 2009/05/13 17:16:04 jym Exp $");

#include "opt_ddb.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/atomic.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/cpuvar.h>
#include <machine/rpb.h>
#include <machine/prom.h>
#include <machine/alpha.h>

struct cpu_info cpu_info_primary = {
	.ci_curlwp = &lwp0
};
struct cpu_info *cpu_info_list = &cpu_info_primary;

#if defined(MULTIPROCESSOR)
/*
 * Array of CPU info structures.  Must be statically-allocated because
 * curproc, etc. are used early.
 */
struct cpu_info *cpu_info[ALPHA_MAXPROCS];

/* Bitmask of CPUs booted, currently running, and paused. */
volatile u_long cpus_booted;
volatile u_long cpus_running;
volatile u_long cpus_paused;

void	cpu_boot_secondary(struct cpu_info *);
#endif /* MULTIPROCESSOR */

/*
 * The Implementation Version and the Architecture Mask must be
 * consistent across all CPUs in the system, so we set it for the
 * primary and announce the AMASK extensions if they exist.
 *
 * Note, we invert the AMASK so that if a bit is set, it means "has
 * extension".
 */
u_long	cpu_implver, cpu_amask;

/* Definition of the driver for autoconfig. */
static int	cpumatch(struct device *, struct cfdata *, void *);
static void	cpuattach(struct device *, struct device *, void *);

CFATTACH_DECL(cpu, sizeof(struct cpu_softc),
    cpumatch, cpuattach, NULL, NULL);

static void	cpu_announce_extensions(struct cpu_info *);

extern struct cfdriver cpu_cd;

static const char *lcaminor[] = {
	"",
	"21066", "21066",
	"21068", "21068",
	"21066A", "21068A", 0
};

struct cputable_struct {
	int	cpu_major_code;
	const char *cpu_major_name;
	const char **cpu_minor_names;
} cpunametable[] = {
	{ PCS_PROC_EV3,		"EV3",		NULL		},
	{ PCS_PROC_EV4,		"21064",	NULL		},
	{ PCS_PROC_SIMULATION,	"Sim",		NULL		},
	{ PCS_PROC_LCA4,	"LCA",		lcaminor	},
	{ PCS_PROC_EV5,		"21164",	NULL		},
	{ PCS_PROC_EV45,	"21064A",	NULL		},
	{ PCS_PROC_EV56,	"21164A",	NULL		},
	{ PCS_PROC_EV6,		"21264",	NULL		},
	{ PCS_PROC_PCA56,	"PCA56",	NULL		},
	{ PCS_PROC_PCA57,	"PCA57",	NULL		},
	{ PCS_PROC_EV67,	"21264A",	NULL		},
	{ PCS_PROC_EV68CB,	"21264C",	NULL		},
	{ PCS_PROC_EV68AL,	"21264B",	NULL		},
	{ PCS_PROC_EV68CX,	"21264D",	NULL		},
};

/*
 * The following is an attempt to map out how booting secondary CPUs
 * works.
 *
 * As we find processors during the autoconfiguration sequence, all
 * processors have idle stacks and PCBs created for them, including
 * the primary (although the primary idles on proc0's PCB until its
 * idle PCB is created).
 *
 * Right before calling uvm_scheduler(), main() calls, on proc0's
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
cpumatch(struct device *parent, struct cfdata *cfdata, void *aux)
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
cpuattach(struct device *parent, struct device *self, void *aux)
{
	struct cpu_softc *sc = (void *) self;
	struct mainbus_attach_args *ma = aux;
	int i;
	const char **s;
	struct pcs *p;
#ifdef DEBUG
	int needcomma;
#endif
	u_int32_t major, minor;
	struct cpu_info *ci;

	p = LOCATE_PCS(hwrpb, ma->ma_slot);
	major = PCS_CPU_MAJORTYPE(p);
	minor = PCS_CPU_MINORTYPE(p);

	printf(": ID %d%s, ", ma->ma_slot,
	    ma->ma_slot == hwrpb->rpb_primary_cpu_id ? " (primary)" : "");

	for(i = 0; i < sizeof cpunametable / sizeof cpunametable[0]; ++i) {
		if (cpunametable[i].cpu_major_code == major) {
			printf("%s-%d", cpunametable[i].cpu_major_name, minor);
			s = cpunametable[i].cpu_minor_names;
			for(i = 0; s && s[i]; ++i) {
				if (i == minor && strlen(s[i]) != 0) {
					printf(" (%s)\n", s[i]);
					goto recognized;
				}
			}
			goto recognized;
		}
	}
	printf("UNKNOWN CPU TYPE (%d:%d)", major, minor);

recognized:
	printf("\n");

#ifdef DEBUG
	if (p->pcs_proc_var != 0) {
		printf("%s: ", sc->sc_dev.dv_xname);

		needcomma = 0;
		if (p->pcs_proc_var & PCS_VAR_VAXFP) {
			printf("VAX FP support");
			needcomma = 1;
		}
		if (p->pcs_proc_var & PCS_VAR_IEEEFP) {
			printf("%sIEEE FP support", needcomma ? ", " : "");
			needcomma = 1;
		}
		if (p->pcs_proc_var & PCS_VAR_PE) {
			printf("%sPrimary Eligible", needcomma ? ", " : "");
			needcomma = 1;
		}
		if (p->pcs_proc_var & PCS_VAR_RESERVED)
			printf("%sreserved bits: 0x%lx", needcomma ? ", " : "",
			    p->pcs_proc_var & PCS_VAR_RESERVED);
		printf("\n");
	}
#endif

	if (ma->ma_slot > ALPHA_WHAMI_MAXID) {
		if (ma->ma_slot == hwrpb->rpb_primary_cpu_id)
			panic("cpu_attach: primary CPU ID too large");
		printf("%s: procssor ID too large, ignoring\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	if (ma->ma_slot == hwrpb->rpb_primary_cpu_id)
		ci = &cpu_info_primary;
	else {
		ci = malloc(sizeof(*ci), M_DEVBUF, M_WAITOK);
		memset(ci, 0, sizeof(*ci));
	}
#if defined(MULTIPROCESSOR)
	cpu_info[ma->ma_slot] = ci;
#endif
	ci->ci_cpuid = ma->ma_slot;
	ci->ci_softc = sc;
	ci->ci_pcc_freq = hwrpb->rpb_cc_freq;

	/*
	 * Though we could (should?) attach the LCA cpus' PCI
	 * bus here there is no good reason to do so, and
	 * the bus attachment code is easier to understand
	 * and more compact if done the 'normal' way.
	 */

#if defined(MULTIPROCESSOR)
	/*
	 * Make sure the processor is available for use.
	 */
	if ((p->pcs_flags & PCS_PA) == 0) {
		if (ma->ma_slot == hwrpb->rpb_primary_cpu_id)
			panic("cpu_attach: primary not available?!");
		printf("%s: processor not available for use\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* Make sure the processor has valid PALcode. */
	if ((p->pcs_flags & PCS_PV) == 0) {
		if (ma->ma_slot == hwrpb->rpb_primary_cpu_id)
			panic("cpu_attach: primary has invalid PALcode?!");
		printf("%s: PALcode not valid\n", sc->sc_dev.dv_xname);
		return;
	}
#endif /* MULTIPROCESSOR */

	/*
	 * If we're the primary CPU, no more work to do; we're already
	 * running!
	 */
	if (ma->ma_slot == hwrpb->rpb_primary_cpu_id) {
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
			aprint_error("%s: mi_cpu_attach failed with %d\n",
			    sc->sc_dev.dv_xname, error);
			return;
		}

		/*
		 * Boot the secondary processor.  It will announce its
		 * extensions, and then spin until we tell it to go
		 * on its merry way.
		 */
		cpu_boot_secondary(ci);
#else /* ! MULTIPROCESSOR */
		printf("%s: processor off-line; multiprocessor support "
		    "not present in kernel\n", sc->sc_dev.dv_xname);
#endif /* MULTIPROCESSOR */
	}

	evcnt_attach_dynamic(&sc->sc_evcnt_clock, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "clock");
	evcnt_attach_dynamic(&sc->sc_evcnt_device, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "device");
#if defined(MULTIPROCESSOR)
	alpha_ipi_init(ci);
#endif
}

static void
cpu_announce_extensions(struct cpu_info *ci)
{
	u_long implver, amask = 0;
	char bits[64];

	implver = alpha_implver();
	if (implver >= ALPHA_IMPLVER_EV5)
		amask = (~alpha_amask(ALPHA_AMASK_ALL)) & ALPHA_AMASK_ALL;

	if (ci->ci_cpuid == hwrpb->rpb_primary_cpu_id) {
		cpu_implver = implver;
		cpu_amask = amask;
	} else {
		if (implver < cpu_implver)
			printf("%s: WARNING: IMPLVER %lu < %lu\n",
			    ci->ci_softc->sc_dev.dv_xname,
			    implver, cpu_implver);

		/*
		 * Cap the system architecture mask to the intersection
		 * of features supported by all processors in the system.
		 */
		cpu_amask &= amask;
	}

	if (amask) {
		snprintb(bits, sizeof(bits),
		    ALPHA_AMASK_BITS, cpu_amask);
		printf("%s: Architecture extensions: %s\n",
		    ci->ci_softc->sc_dev.dv_xname, bits);
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
		if (ci->ci_flags & CPUF_PRIMARY)
			continue;
		if ((cpus_booted & (1UL << i)) == 0)
			continue;

		/* Patch MP-criticial kernel routines. */
		if (did_patch == false) {
			alpha_patch(true);
			did_patch = true;
		}

		/*
		 * Link the processor into the list, and launch it.
		 */
		ci->ci_next = cpu_info_list->ci_next;
		cpu_info_list->ci_next = ci;
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

	pcb = &ci->ci_data.cpu_idlelwp->l_addr->u_pcb;
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
	hwrpb->rpb_restart = (u_int64_t) cpu_spinup_trampoline;
	hwrpb->rpb_restart_val = (u_int64_t) ci;
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
		printf("%s: unable to issue `START' command\n",
		    ci->ci_softc->sc_dev.dv_xname);
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
		printf("%s: processor failed to boot\n",
		    ci->ci_softc->sc_dev.dv_xname);

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
		printf("%s: processor failed to hatch\n",
		    ci->ci_softc->sc_dev.dv_xname);
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

	printf("%s: shutting down...\n", ci->ci_softc->sc_dev.dv_xname);

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

	/* Mark the kernel pmap active on this processor. */
	atomic_or_ulong(&pmap_kernel()->pm_cpus, cpumask);

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

	cc_calibrate_cpu(ci);
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
	 * that it's there.
	 */
	strcpy(pcsp->pcs_iccb.iccb_rxbuf, msg);
	pcsp->pcs_iccb.iccb_rxlen = strlen(msg);
	atomic_or_ulong(&hwrpb->rpb_rxrdy, cpumask);
	membar_sync();

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
	u_int64_t txrdy;
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

	db_printf("addr		dev	id	flags	ipis	curproc		fpcurproc\n");
	for (CPU_INFO_FOREACH(cii, ci)) {
		db_printf("%p	%s	%lu	%lx	%lx	%p	%p\n",
		    ci,
		    ci->ci_softc->sc_dev.dv_xname,
		    ci->ci_cpuid,
		    ci->ci_flags,
		    ci->ci_ipis,
		    ci->ci_curlwp,
		    ci->ci_fpcurlwp);
	}
}

#endif /* DDB */

#endif /* MULTIPROCESSOR */
