/*	$NetBSD: m68k_intr.c,v 1.4 2024/01/15 02:13:16 thorpej Exp $	*/

/*-
 * Copyright (c) 1996, 2023, 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, Gordon W. Ross, and Jason R. Thorpe.
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
 * Common interrupt handling for m68k platforms.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: m68k_intr.c,v 1.4 2024/01/15 02:13:16 thorpej Exp $");

#define	_M68K_INTR_PRIVATE

#include <sys/param.h> 
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/vmmeter.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/bus.h>
#include <sys/once.h>
#include <sys/intr.h>

#include <machine/vectors.h>

#include <uvm/uvm_extern.h>

#ifdef __HAVE_M68K_INTR_VECTORED
#ifndef MACHINE_USERVEC_START
#define	MACHINE_USERVEC_START	VECI_USRVEC_START
#endif

#define	NVECHANDS	(NVECTORS - MACHINE_USERVEC_START)

#ifndef INTR_FREEVEC
#define	INTR_FREEVEC	badtrap
#endif

extern char INTR_FREEVEC[];
extern char intrstub_vectored[];
#endif /* __HAVE_M68K_INTR_VECTORED */

/* A dummy event counter where interrupt stats go to die. */
static struct evcnt bitbucket;

#ifdef __HAVE_LEGACY_INTRCNT
extern u_int intrcnt[];		/* XXX old-style statistics */
#endif

int idepth;

static struct m68k_intrhand_list m68k_intrhands_autovec[NAUTOVECTORS];
#ifdef __HAVE_M68K_INTR_VECTORED
static struct m68k_intrhand *m68k_intrhands_vectored[NVECHANDS];
#endif

const uint16_t ipl2psl_table[NIPL] = {
	[IPL_NONE]	 = PSL_S | PSL_IPL0,
	[IPL_SOFTBIO]	 = PSL_S | MACHINE_PSL_IPL_SOFTBIO,
	[IPL_SOFTCLOCK]	 = PSL_S | MACHINE_PSL_IPL_SOFTCLOCK,
	[IPL_SOFTNET]	 = PSL_S | MACHINE_PSL_IPL_SOFTNET,
	[IPL_SOFTSERIAL] = PSL_S | MACHINE_PSL_IPL_SOFTSERIAL,
	[IPL_VM]	 = PSL_S | MACHINE_PSL_IPL_VM,
	[IPL_SCHED]	 = PSL_S | MACHINE_PSL_IPL_SCHED,
	[IPL_HIGH]	 = PSL_S | PSL_IPL7,
};

static struct m68k_intrhand *
m68k_ih_stdalloc(int km_flag)
{
	return kmem_zalloc(sizeof(struct m68k_intrhand), km_flag);
}

static void
m68k_ih_stdfree(struct m68k_intrhand *ih)
{
	kmem_free(ih, sizeof(*ih));
}

static const struct m68k_ih_allocfuncs m68k_ih_stdallocfuncs = {
	.alloc = m68k_ih_stdalloc,
	.free  = m68k_ih_stdfree,
};

static const struct m68k_ih_allocfuncs *ih_allocfuncs;

static struct m68k_intrhand *
m68k_ih_alloc(void)
{
	KASSERT(ih_allocfuncs != NULL);
	return ih_allocfuncs->alloc(KM_SLEEP);
}

static void
m68k_ih_free(struct m68k_intrhand *ih)
{
	KASSERT(ih_allocfuncs != NULL);
	ih_allocfuncs->free(ih);
}

#ifdef __HAVE_M68K_INTR_VECTORED

#define	INTRVEC_SLOT(vec)	\
	(&m68k_intrhands_vectored[(vec) - MACHINE_USERVEC_START])

#define	m68k_intrvec_handler(vec)	(*INTRVEC_SLOT(vec))

static bool
m68k_intrvec_add(struct m68k_intrhand *ih)
{
	if (ih->ih_vec < MACHINE_USERVEC_START || ih->ih_vec >= NVECTORS) {
		aprint_error("%s: vector=0x%x (invalid)\n", __func__,
		    ih->ih_vec);
		return false;
	}

	struct m68k_intrhand **slot =
	    &m68k_intrhands_vectored[ih->ih_vec - MACHINE_USERVEC_START];

	if (*slot != NULL) {
		aprint_error("%s: vector=0x%x (in use)\n", __func__,
		    ih->ih_vec);
		return false;
	}

	if (vec_get_entry(ih->ih_vec) != INTR_FREEVEC) {
		aprint_error("%s: vector=0x%x (unavailable)\n", __func__,
		    ih->ih_vec);
		return false;
	}

	*slot = ih;
	vec_set_entry(ih->ih_vec, intrstub_vectored);
	return true;
}

static void
m68k_intrvec_remove(struct m68k_intrhand *ih)
{
	KASSERT(ih->ih_vec >= MACHINE_USERVEC_START);
	KASSERT(ih->ih_vec < NVECTORS);

	struct m68k_intrhand **slot =
	    &m68k_intrhands_vectored[ih->ih_vec - MACHINE_USERVEC_START];

	KASSERT(*slot == ih);
	KASSERT(vec_get_entry(ih->ih_vec) == intrstub_vectored);

	vec_set_entry(ih->ih_vec, INTR_FREEVEC);
	*slot = NULL;
}

#endif /* __HAVE_M68K_INTR_VECTORED */

/*
 * m68k_intr_init --
 *	Initialize the interrupt subsystem.
 */
void
m68k_intr_init(const struct m68k_ih_allocfuncs *allocfuncs)
{
	int i;

	KASSERT(ih_allocfuncs == NULL);
	if (allocfuncs == NULL) {
		ih_allocfuncs = &m68k_ih_stdallocfuncs;
	} else {
		ih_allocfuncs = allocfuncs;
	}

	for (i = 0; i < NAUTOVECTORS; i++) {
		LIST_INIT(&m68k_intrhands_autovec[i]);
	}
}

/*
 * m68k_intr_establish --
 *	Establish an interrupt handler at the specified vector.
 *	XXX We don't do anything with isrpri yet.
 *	XXX We don't do anything with the flags yet.
 */
void *
m68k_intr_establish(int (*func)(void *), void *arg, struct evcnt *ev,
    int vec, int ipl, int isrpri, int flags __unused)
{
	struct m68k_intrhand *ih;
	int s;

	/*
	 * If a platform doesn't want special behavior, we don't
	 * require them to call m68k_intr_init(); we just handle
	 * it here.
	 *
	 * XXX m68k_intr_init() might be called really early, so
	 * XXX can't use a once control.
	 */
	if (__predict_false(ih_allocfuncs == NULL)) {
		m68k_intr_init(NULL);
	}

	/* These are m68k IPLs, not IPL_* values. */
	if (ipl < 0 || ipl > 7) {
		return NULL;
	}

#ifdef __HAVE_M68K_INTR_VECTORED
	KASSERT(vec >= 0);
	KASSERT(vec < NVECTORS);
#else
	KASSERT(vec == 0);
#endif /* __HAVE_M68K_INTR_VECTORED */

	ih = m68k_ih_alloc();
	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_vec = vec;
	ih->ih_ipl = ipl;
	ih->ih_isrpri = isrpri;
	if ((ih->ih_evcnt = ev) == NULL) {
		ih->ih_evcnt = &bitbucket;
	}

#ifdef __HAVE_M68K_INTR_VECTORED
	if (vec != 0) {
		if (vec_get_entry(vec) != INTR_FREEVEC) {
			m68k_ih_free(ih);
			return NULL;
		}
		if (! m68k_intrvec_add(ih)) {
			m68k_ih_free(ih);
			return NULL;
		}
		return ih;
	}
#endif

	/*
	 * Some devices are particularly sensitive to interrupt
	 * handling latency.  Unbuffered serial ports, for example,
	 * can lose data if their interrupts aren't handled with
	 * reasonable speed.  For this reason, we sort interrupt
	 * handlers by an abstract "ISR" priority, inserting higher-
	 * priority interrupts before lower-priority interrupts.
	 */
	struct m68k_intrhand_list * const list = &m68k_intrhands_autovec[ipl];
	struct m68k_intrhand *curih;

	s = splhigh();
	if (LIST_EMPTY(list)) {
		LIST_INSERT_HEAD(list, ih, ih_link);
		goto done;
	}
	for (curih = LIST_FIRST(list);
	     LIST_NEXT(curih, ih_link) != NULL;
	     curih = LIST_NEXT(curih, ih_link)) {
		if (ih->ih_isrpri > curih->ih_isrpri) {
			LIST_INSERT_BEFORE(curih, ih, ih_link);
			goto done;
		}
	}
	LIST_INSERT_AFTER(curih, ih, ih_link);
 done:
	splx(s);

	return ih;
}

/*
 * m68k_intr_disestablish --
 *	Remove an interrupt handler.  Returns true if the handler
 *	list for this vector is now empty.
 */
bool
m68k_intr_disestablish(void *v)
{
	struct m68k_intrhand *ih = v;
	int s;
	bool empty;

#ifdef __HAVE_M68K_INTR_VECTORED
	if (ih->ih_vec != 0) {
		KASSERT(vec_get_entry(ih->ih_vec) == intrstub_vectored);
		m68k_intrvec_remove(ih);
		empty = true;
	} else
#endif
	{
		s = splhigh();
		LIST_REMOVE(ih, ih_link);
		empty = LIST_EMPTY(&m68k_intrhands_autovec[ih->ih_ipl]);
		splx(s);
	}

	m68k_ih_free(ih);

	return empty;
}

void	m68k_intr_autovec(struct clockframe);

/*
 * m68k_intr_autovec --
 *	Run the interrupt handlers for an auto-vectored interrupt.
 *	Called from the assembly glue in m68k_intr_stubs.s
 */
void
m68k_intr_autovec(struct clockframe frame)
{
	const int ipl = VECO_TO_VECI(frame.cf_vo & 0xfff) - VECI_INTRAV0;
	struct m68k_intrhand *ih;
	bool rv = false;

	idepth++;

#ifdef __HAVE_LEGACY_INTRCNT
	intrcnt[ipl]++;		/* XXX */
#endif
	curcpu()->ci_data.cpu_nintr++;

	LIST_FOREACH(ih, &m68k_intrhands_autovec[ipl], ih_link) {
		void *arg = ih->ih_arg ? ih->ih_arg : &frame;
		if (ih->ih_func(arg)) {
			ih->ih_evcnt->ev_count++;
			rv = true;
		}
	}
	if (!rv) {
		printf("Spurious interrupt on IPL %d\n", ipl);
	}

	idepth--;

	ATOMIC_CAS_CHECK(&frame);
}

#ifdef __HAVE_M68K_INTR_VECTORED
void	m68k_intr_vectored(struct clockframe);

/*
 * m68k_intr_vectored --
 *	Run a vectored interrupt handler.
 *	Called from the assembly glue in m68k_intr_stubs.s
 */
void
m68k_intr_vectored(struct clockframe frame)
{
	const int vec = VECO_TO_VECI(frame.cf_vo & 0xfff);
	const int ipl = (getsr() >> 8) & 7;
	struct m68k_intrhand *ih;

	idepth++;

#ifdef __HAVE_LEGACY_INTRCNT
	intrcnt[ipl]++;		/* XXX */
#endif
	curcpu()->ci_data.cpu_nintr++;

#ifdef DIAGNOSTIC
	if (vec < MACHINE_USERVEC_START || vec >= NVECTORS) {
		printf("%s: vector=0x%x (invalid)\n", __func__, vec);
		goto out;
	}
#endif
	ih = m68k_intrvec_handler(vec);
	if (ih == NULL) {
		printf("%s: vector=0x%x (no handler?)\n", __func__, vec);
		vec_set_entry(vec, INTR_FREEVEC);
	}

	if ((*ih->ih_func)(ih->ih_arg ? ih->ih_arg : &frame) == 0) {
		printf("Spurious interrupt on vector=0x%0x IPL %d\n",
		    vec, ipl);
	}
#ifdef DIAGNOSTIC
 out:
#endif
	idepth--;

	ATOMIC_CAS_CHECK(&frame);
}
#endif /* __HAVE_M68K_INTR_VECTORED */
