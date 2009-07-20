/* $NetBSD: interrupt.c,v 1.4 2009/07/20 04:41:37 kiyohara Exp $ */

/*-
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Keith Bostic, Chris G. Demetriou
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
/*-
 * Additional Copyright (c) 1997 by Matthew Jacob for NASA/Ames Research Center.
 * Redistribute and modify at will, leaving only this additional copyright
 * notice.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.4 2009/07/20 04:41:37 kiyohara Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/lwp.h>
#include <sys/malloc.h>
#include <sys/sched.h>

#include <uvm/uvm_extern.h>

#include <machine/clock.h>
#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/fpu.h>
#include <machine/frame.h>
#include <machine/intr.h>
#include <machine/md_var.h>
#include <machine/sapicvar.h>
#include <machine/smp.h>
#include <machine/userret.h>

#ifdef DDB
#include <ddb/ddb.h>
#endif


static void ia64_intr_eoi(void *);
static void ia64_intr_mask(void *);
#if 0
static void ia64_intr_unmask(void *);
#endif
static int ia64_dispatch_intr(void *, u_int);

#ifdef DDB
void db_print_vector(u_int, int);
#endif


int
interrupt(uint64_t vector, struct trapframe *tf)
{
	struct cpu_info *ci = curcpu();
	volatile struct ia64_interrupt_block *ib = IA64_INTERRUPT_BLOCK;
	uint64_t adj, clk, itc;
	int64_t delta;
	uint8_t inta;
	int count, handled = 0;

	ia64_set_fpsr(IA64_FPSR_DEFAULT);

	ci->ci_intrdepth++;
	uvmexp.intrs++;

 next:
	/*
	 * Handle ExtINT interrupts by generating an INTA cycle to
	 * read the vector.
	 */
	if (vector == 0) {
		inta = ib->ib_inta;
		printf("ExtINT interrupt: vector=%u\n", (int)inta);
		if (inta == 15) {
			__asm __volatile("mov cr.eoi = r0;; srlz.d");
			goto stray;
		}
		vector = (int)inta;
	} else if (vector == 15)
		goto stray;

	if (vector == CLOCK_VECTOR) {/* clock interrupt */
		itc = ia64_get_itc();

		adj = ci->ci_clockadj;
		clk = ci->ci_clock;
		delta = itc - clk;
		count = 0;
		while (delta >= ia64_clock_reload) {
			/* Only the BSP runs the real clock */
			if (ci->ci_cpuid == 0)
				hardclock((struct clockframe *)tf);
			else
				panic("CLOCK_VECTOR occur on not cpu0");
			delta -= ia64_clock_reload;
			clk += ia64_clock_reload;
			count++;
			handled = 1;
		}
		ia64_set_itm(ia64_get_itc() + ia64_clock_reload - adj);
		if (count > 0) {
			if (delta > (ia64_clock_reload >> 3)) {
				adj = ia64_clock_reload >> 4;
			} else
				adj = 0;
		} else
			adj = 0;
		ci->ci_clock = clk;
		ci->ci_clockadj = adj;
		ia64_srlz_d();

#ifdef MULTIPROCESSOR
	} else if (vector == ipi_vector[IPI_AST]) {
		asts[PCPU_GET(cpuid)]++;
		CTR1(KTR_SMP, "IPI_AST, cpuid=%d", PCPU_GET(cpuid));
	} else if (vector == ipi_vector[IPI_HIGH_FP]) {
		struct thread *thr = PCPU_GET(fpcurthread);

		if (thr != NULL) {
			mtx_lock_spin(&thr->td_md.md_highfp_mtx);
			save_high_fp(&thr->td_pcb->pcb_high_fp);
			thr->td_pcb->pcb_fpcpu = NULL;
			PCPU_SET(fpcurthread, NULL);
			mtx_unlock_spin(&thr->td_md.md_highfp_mtx);
		}
	} else if (vector == ipi_vector[IPI_RENDEZVOUS]) {
		rdvs[PCPU_GET(cpuid)]++;
		CTR1(KTR_SMP, "IPI_RENDEZVOUS, cpuid=%d", PCPU_GET(cpuid));
		enable_intr();
		smp_rendezvous_action();
		disable_intr();
	} else if (vector == ipi_vector[IPI_STOP]) {
		cpumask_t mybit = PCPU_GET(cpumask);

		savectx(PCPU_PTR(pcb));
		atomic_set_int(&stopped_cpus, mybit);
		while ((started_cpus & mybit) == 0)
			cpu_spinwait();
		atomic_clear_int(&started_cpus, mybit);
		atomic_clear_int(&stopped_cpus, mybit);
	} else if (vector == ipi_vector[IPI_PREEMPT]) {
		CTR1(KTR_SMP, "IPI_PREEMPT, cpuid=%d", PCPU_GET(cpuid));
		__asm __volatile("mov cr.eoi = r0;; srlz.d");
		enable_intr();
		sched_preempt(curthread);
		disable_intr();
		goto stray;
#endif
	} else {
		ci->ci_intrdepth++;
		handled = ia64_dispatch_intr(tf, vector);
		ci->ci_intrdepth--;
	}

	__asm __volatile("mov cr.eoi = r0;; srlz.d");
	vector = ia64_get_ivr();
	if (vector != 15)
		goto next;

stray:
	if (TRAPF_USERMODE(tf)) {
		enable_intr();
		userret(curlwp);
		do_ast(tf);
	}
	ci->ci_intrdepth--;
	return handled;
}


/*
 * Hardware irqs have vectors starting at this offset.
 */
#define IA64_HARDWARE_IRQ_BASE	0x20

struct ia64_intrhand {
	int (*ih_func)(void *);
	void *ih_arg;
	LIST_ENTRY(ia64_intrhand) ih_q;
	int ih_level;
	int ih_irq;
};
struct ia64_intr {
	u_int irq;
	struct sapic *sapic;
	int type;

	LIST_HEAD(, ia64_intrhand) intr_q;

	char evname[32];
	struct evcnt evcnt;
};

static struct ia64_intr *ia64_intrs[256];


static void
ia64_intr_eoi(void *arg)
{
	u_int vector = (uintptr_t)arg;
	struct ia64_intr *i;

	i = ia64_intrs[vector];
	if (i != NULL)
		sapic_eoi(i->sapic, vector);
}

static void
ia64_intr_mask(void *arg)
{
	u_int vector = (uintptr_t)arg;
	struct ia64_intr *i;

	i = ia64_intrs[vector];
	if (i != NULL) {
		sapic_mask(i->sapic, i->irq);
		sapic_eoi(i->sapic, vector);
	}
}

#if 0
static void
ia64_intr_unmask(void *arg)
{
	u_int vector = (uintptr_t)arg;
	struct ia64_intr *i;

	i = ia64_intrs[vector];
	if (i != NULL)
		sapic_unmask(i->sapic, i->irq);
}
#endif

void *
intr_establish(int irq, int type, int level, int (*func)(void *), void *arg)
{
	struct ia64_intr *i;
	struct ia64_intrhand *ih;
	struct sapic *sa;
	u_int vector;

	/* Get the I/O SAPIC that corresponds to the IRQ. */
	sa = sapic_lookup(irq);
	if (sa == NULL)
		return NULL;

	switch (type) {
	case IST_EDGE:
	case IST_LEVEL:
		break;

	default:
		return NULL;
	}

	/*
	 * XXX - There's a priority implied by the choice of vector.
	 * We should therefore relate the vector to the interrupt type.
	 */
	vector = irq + IA64_HARDWARE_IRQ_BASE;

	i = ia64_intrs[vector];
	if (i == NULL) {
		i = malloc(sizeof(struct ia64_intr), M_DEVBUF, M_NOWAIT);
		if (i == NULL)
			return NULL;
		i->irq = irq;
		i->sapic = sa;
		i->type = type;
		LIST_INIT(&i->intr_q);
		snprintf(i->evname, sizeof(i->evname), "irq %d", irq);
		evcnt_attach_dynamic(&i->evcnt, EVCNT_TYPE_INTR, NULL,
		    "iosapic", i->evname);
		ia64_intrs[vector] = i;

		sapic_config_intr(irq, type);
		sapic_enable(i->sapic, irq, vector);
	} else
		if (i->type != type)
			return NULL;

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return NULL;
	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_level = level;
	ih->ih_irq = irq;
	LIST_INSERT_HEAD(&i->intr_q, ih, ih_q);

	return ih;
}

void
intr_disestablish(void *cookie)
{
	struct ia64_intr *i;
	struct ia64_intrhand *ih = cookie;
	u_int vector = ih->ih_irq + IA64_HARDWARE_IRQ_BASE;

	i = ia64_intrs[vector];

	LIST_REMOVE(ih, ih_q);
	if (LIST_FIRST(&i->intr_q) == NULL) {
		ia64_intr_mask((void *)(uintptr_t)vector);

		ia64_intrs[vector] = NULL;
		evcnt_detach(&i->evcnt);
		free(i, M_DEVBUF);
	}

	free(ih, M_DEVBUF);
}

static int
ia64_dispatch_intr(void *frame, u_int vector)
{
	struct ia64_intr *i;
	struct ia64_intrhand *ih;
	int handled = 0;

	/*
	 * Find the interrupt thread for this vector.
	 */
	i = ia64_intrs[vector];
	KASSERT(i != NULL);

	i->evcnt.ev_count++;

	LIST_FOREACH(ih, &i->intr_q, ih_q) {
		if (__predict_false(ih->ih_func == NULL))
			printf("%s: spurious interrupt (irq = %d)\n",
			    __func__, ih->ih_irq);
		else if (__predict_true((*ih->ih_func)(ih->ih_arg)))
			handled = 1;
	}
	ia64_intr_eoi((void *)(uintptr_t)vector);

	return handled;
}

#ifdef DDB
void
db_print_vector(u_int vector, int always)
{
	struct ia64_intr *i;

	i = ia64_intrs[vector];
	if (i != NULL) {
		db_printf("vector %u (%p): ", vector, i);
		sapic_print(i->sapic, i->irq);
	} else if (always)
		db_printf("vector %u: unassigned\n", vector);
}
#endif
