/* $NetBSD: cpu.c,v 1.27 1998/09/24 23:28:17 thorpej Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.27 1998/09/24 23:28:17 thorpej Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <vm/vm.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/rpb.h>
#include <machine/prom.h>

#include <alpha/alpha/cpuvar.h>

#if defined(MULTIPROCESSOR)
TAILQ_HEAD(, cpu_softc) cpu_spinup_queue =
    TAILQ_HEAD_INITIALIZER(cpu_spinup_queue);

/* Map CPU ID to cpu_softc; allocate in cpu_attach(). */
struct cpu_softc **cpus;

/* Quick access to the primary CPU. */
struct cpu_softc *primary_cpu;
#endif /* MULTIPROCESSOR */

/* Definition of the driver for autoconfig. */
static int	cpumatch(struct device *, struct cfdata *, void *);
static void	cpuattach(struct device *, struct device *, void *);

struct cfattach cpu_ca = {
	sizeof(struct cpu_softc), cpumatch, cpuattach
};

extern struct cfdriver cpu_cd;

static char *ev4minor[] = {
	"pass 2 or 2.1", "pass 3", 0
}, *lcaminor[] = {
	"",
	"21066 pass 1 or 1.1", "21066 pass 2",
	"21068 pass 1 or 1.1", "21068 pass 2",
	"21066A pass 1", "21068A pass 1", 0
}, *ev5minor[] = {
	"", "pass 2, rev BA or 2.2, rev CA", "pass 2.3, rev DA or EA",
	"pass 3", "pass 3.2", "pass 4", 0
}, *ev45minor[] = {
	"", "pass 1", "pass 1.1", "pass 2", 0
}, *ev56minor[] = {
	"", "pass 1", "pass 2", 0
}, *ev6minor[] = {
	"", "pass 1", 0
}, *pca56minor[] = {
	"", "pass 1", 0
};

struct cputable_struct {
	int	cpu_major_code;
	char	*cpu_major_name;
	char	**cpu_minor_names;
} cpunametable[] = {
	{ PCS_PROC_EV3,		"EV3",		0		},
	{ PCS_PROC_EV4,		"21064",	ev4minor	},
	{ PCS_PROC_SIMULATION,	"Sim",		0		},
	{ PCS_PROC_LCA4,	"LCA",		lcaminor	},
	{ PCS_PROC_EV5,		"21164",	ev5minor	},
	{ PCS_PROC_EV45,	"21064A",	ev45minor	},
	{ PCS_PROC_EV56,	"21164A",	ev56minor	},
	{ PCS_PROC_EV6,		"21264",	ev6minor	},
	{ PCS_PROC_PCA56,	"PCA56",	pca56minor	}
};

/*
 * The following is an attempt to map out how booting secondary CPUs
 * works.
 *
 * As we find processors during the autoconfiguration sequence, all
 * processors that are available and not the primary are placed on
 * a "spin-up queue".  Once autoconfiguration has completed, this
 * queue is run to spin up the secondary processors.
 *
 * cpu_run_spinup_queue() pulls each processor off the list, switches
 * it to OSF/1 PALcode, sets the entry point to the cpu_spinup_trampoline,
 * and then sends a "START" command to the secondary processor's console.
 *
 * Upon successful processor bootup, the cpu_spinup_trampoline will call
 * cpu_hatch(), which will print a message indicating that the processor
 * is running, and will set the "hatched" flag in its softc.  At the end
 * of cpu_hatch() is a spin-forever loop; we do not yet attempt to schedule
 * anything on secondary CPUs.
 *
 * To summarize:
 *
 *	[primary cpu] cpu_run_spinup_queue -> return
 *
 *	[secondary cpus] cpu_spinup_trampoline -> cpu_hatch -> spin-loop
 */

static int
cpumatch(parent, cfdata, aux)
	struct device *parent;
	struct cfdata *cfdata;
	void *aux;
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
cpuattach(parent, dev, aux)
	struct device *parent;
	struct device *dev;
	void *aux;
{
	struct cpu_softc *sc = (struct cpu_softc *)dev;
	struct mainbus_attach_args *ma = aux;
	int i;
	char **s;
        struct pcs *p;
#ifdef DEBUG
	int needcomma;
#endif
	u_int32_t major, minor;

	p = LOCATE_PCS(hwrpb, ma->ma_slot);
	major = PCS_CPU_MAJORTYPE(p);
	minor = PCS_CPU_MINORTYPE(p);

	sc->sc_cpuid = ma->ma_slot;

	printf(": ID %d%s, ", ma->ma_slot,
	    ma->ma_slot == hwrpb->rpb_primary_cpu_id ? " (primary)" : "");

	for(i = 0; i < sizeof cpunametable / sizeof cpunametable[0]; ++i) {
		if (cpunametable[i].cpu_major_code == major) {
			printf("%s", cpunametable[i].cpu_major_name);
			s = cpunametable[i].cpu_minor_names;
			for(i = 0; s && s[i]; ++i) {
				if (i == minor) {
					printf(" (%s)\n", s[i]);
					goto recognized;
				}
			}
			printf(" (unknown minor type %d)\n", minor);
			goto recognized;
		}
	}
	printf("UNKNOWN CPU TYPE (%d:%d)", major, minor);

recognized:

#ifdef DEBUG
	/* XXX SHOULD CHECK ARCHITECTURE MASK, TOO */
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

#if defined(MULTIPROCESSOR)
	if (ma->ma_slot > ALPHA_WHAMI_MAXID) {
		printf("%s: procssor ID too large, ignoring\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	if (cpus == NULL) {
		size_t size = sizeof(struct cpu_softc *) * hwrpb->rpb_pcs_cnt;

		cpus = malloc(size, M_DEVBUF, M_NOWAIT);
		if (cpus == NULL)
			panic("cpu_attach: unable to allocate cpus array");
		memset(cpus, 0, size);
	}

	cpus[ma->ma_slot] = sc;
#endif /* MULTIPROCESSOR */

	/*
	 * Though we could (should?) attach the LCA cpus' PCI
	 * bus here there is no good reason to do so, and
	 * the bus attachment code is easier to understand
	 * and more compact if done the 'normal' way.
	 */

#if defined(MULTIPROCESSOR)
	/*
	 * If we're the primary CPU, no more work to do; we're already
	 * running!
	 */
	if (ma->ma_slot == hwrpb->rpb_primary_cpu_id) {
		sc->sc_flags |= (CPUF_PRIMARY|CPUF_HATCHED);
		primary_cpu = sc;
		return;
	}

	/*
	 * Not the primary CPU; need to queue this processor to be
	 * started after autoconfiguration is finished.
	 */

	if ((p->pcs_flags & PCS_PA) == 0) {
		printf("%s: processor not available for use\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Place this processor on the cpu-spinup queue.  No locking is
	 * necessary; only the primary CPU will access this list.
	 */
	TAILQ_INSERT_TAIL(&cpu_spinup_queue, sc, sc_q);
#endif /* MULTIPROCESSOR */
}

#if defined(MULTIPROCESSOR)
void
cpu_run_spinup_queue()
{
	struct cpu_softc *sc;
	long timeout;
	struct pcs *pcsp, *primary_pcsp;
	struct pcb *pcb;
#if 0
	int s;
#endif

	primary_pcsp = LOCATE_PCS(hwrpb, hwrpb->rpb_primary_cpu_id);

	while ((sc = TAILQ_FIRST(&cpu_spinup_queue)) != NULL) {
		TAILQ_REMOVE(&cpu_spinup_queue, sc, sc_q);

		pcsp = LOCATE_PCS(hwrpb, sc->sc_cpuid);

		/* Make sure the processor has valid PALcode. */
		if ((pcsp->pcs_flags & PCS_PV) == 0) {
			printf("%s: PALcode not valid\n", sc->sc_dev.dv_xname);
			continue;
		}

		/*
		 * XXX This really should fork honest idleprocs (a'la proc0)
		 * XXX but we aren't called via MI code yet, so we can't
		 * XXX really fork.
		 */

		/* Allocate the idle stack. */
		sc->sc_idle_stack = malloc(USPACE, M_TEMP, M_NOWAIT);
		if (sc->sc_idle_stack == NULL) {
			printf("%s: unable to allocate idle stack\n",
			    sc->sc_dev.dv_xname);
			continue;
		}
		memset(sc->sc_idle_stack, 0, USPACE);

		/*
		 * Initialize the PCB and copy it to the PCS.
		 * XXX We don't yet use the trapframe.  Will we ever?
		 */
		pcb = (struct pcb *)sc->sc_idle_stack;
		pcb->pcb_hw.apcb_ksp =
		    (u_int64_t)sc->sc_idle_stack + USPACE -
		    sizeof(struct trapframe);
#if 0
		pcb->pcb_hw.apcb_unique = pcb->pcb_hw.apcb_ksp; /* XXX WHY? */
#endif
		pcb->pcb_hw.apcb_asn = proc0.p_addr->u_pcb.pcb_hw.apcb_asn;
		pcb->pcb_hw.apcb_ptbr = proc0.p_addr->u_pcb.pcb_hw.apcb_ptbr;
		memcpy(pcsp->pcs_hwpcb, &pcb->pcb_hw, sizeof(pcb->pcb_hw));

		/*
		 * Set up the HWRPB to restart the secondary processor
		 * with our spin-up trampoline.
		 */
		hwrpb->rpb_restart = (u_int64_t) cpu_spinup_trampoline;
		hwrpb->rpb_restart_val = (u_int64_t) sc;
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

#if 0
		s = prom_enter();	/* XXX XXX XXX */
#endif

		/* Send a "START" command to the secondary CPU's console. */
		if (cpu_iccb_send(sc->sc_cpuid, "START\r\n")) {
			printf("%s: unable to issue `START' command\n",
			    sc->sc_dev.dv_xname);
			continue;
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
			    sc->sc_dev.dv_xname);

		/*
		 * ...and now wait for verification that it's running kernel
		 * code.
		 */
		for (timeout = 10000; timeout != 0; timeout--) {
			alpha_mb();
			if ((volatile u_long)sc->sc_flags & CPUF_HATCHED)
				break;
			delay(1000);
		}
		if (timeout == 0)
			printf("%s: processor failed to hatch\n",
			    sc->sc_dev.dv_xname);
#if 0
		prom_leave(s);		/* XXX XXX XXX */
#endif
	}
}

void
cpu_hatch(sc)
	struct cpu_softc *sc;
{

	/* Initialize trap vectors for this processor. */
	trap_init();

	/* Yahoo!  We're running kernel code!  Announce it! */
	printf("%s: processor ID %lu running\n", sc->sc_dev.dv_xname,
	    alpha_pal_whami());
	alpha_atomic_setbits_q(&sc->sc_flags, CPUF_HATCHED);

	/* Ok, so all we do is spin for now... */
	for (;;)
		/* nothing */ ;
}

int
cpu_iccb_send(cpu_id, msg)
	long cpu_id;
	const char *msg;
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
	 * that it's there.  The atomic operation performs a memory barrier.
	 */
	strcpy(pcsp->pcs_iccb.iccb_rxbuf, msg);
	pcsp->pcs_iccb.iccb_rxlen = strlen(msg);
	(void) alpha_atomic_testset_q(&hwrpb->rpb_rxrdy, cpumask);

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
cpu_iccb_receive()
{
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
	hwrpb->rpb_txrdy = 0;
	alpha_mb();
}
#endif /* MULTIPROCESSOR */
