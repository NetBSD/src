/*	$NetBSD: xscale_pmc.c,v 1.9.8.1 2006/05/24 10:56:38 yamt Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Allen Briggs for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xscale_pmc.c,v 1.9.8.1 2006/05/24 10:56:38 yamt Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/pmc.h>
#include <sys/systm.h>
#include <sys/types.h>

#include <machine/pmc.h>

#include <arm/xscale/xscalereg.h>
#include <arm/xscale/xscalevar.h>

extern int	profsrc;

struct xscale_pmc_state {
	uint32_t	pmnc;		/* performance monitor ctrl */
	uint32_t	pmcr[3];	/* array of counter reset values */
	uint64_t	pmcv[3];	/* array of counter values */
	uint64_t	pmc_accv[3];	/* accumulated ctr values of children */
};

#define __PMC_CCNT_I	(0)
#define __PMC0_I	(1)
#define __PMC1_I	(2)
#define __PMC_CCNT	(1 << __PMC_CCNT_I)
#define __PMC0		(1 << __PMC0_I)
#define __PMC1		(1 << __PMC1_I)
#define __PMC_NCTRS	3

uint32_t	pmc_kernel_mask = 0;
uint32_t	pmc_kernel_bits = 0;
uint32_t	pmc_kernel_enabled = 0;
uint32_t	pmc_profiling_enabled = 0;
uint32_t	pmc_reset_vals[3] = {0x80000000, 0x80000000, 0x80000000};

int		pmc_usecount[3] = {0, 0, 0};

static inline uint32_t
xscale_pmnc_read(void)
{
	uint32_t pmnc;

	__asm volatile("mrc p14, 0, %0, c0, c0, 0"
		: "=r" (pmnc));

	return pmnc;
}

static inline void
xscale_pmnc_write(uint32_t val)
{

	__asm volatile("mcr p14, 0, %0, c0, c0, 0"
		: : "r" (val));
}

int
xscale_pmc_dispatch(void *arg)
{
	struct clockframe *frame = arg;
	struct xscale_pmc_state *pmcs;
	struct proc *p;
	uint32_t pmnc;
	int s;

	s = splstatclock();

	pmnc = xscale_pmnc_read() & ~(PMNC_C | PMNC_P);

	/*
	 * Disable interrupts -- ensure we don't reset anything.
	 */
	xscale_pmnc_write(pmnc &
	    ~(PMNC_PMN0_IF | PMNC_PMN1_IF | PMNC_CC_IF | PMNC_E));

	/*
	 * If we have recorded a clock overflow...
	 */
	if (pmnc & PMNC_CC_IF) {
		if (pmc_profiling_enabled & __PMC_CCNT) {
			proftick(frame);
			__asm volatile("mcr p14, 0, %0, c1, c0, 0"
			    : : "r" (pmc_reset_vals[__PMC_CCNT_I]));
		} else if ((p = curproc) != NULL &&
			   (p->p_md.pmc_enabled & __PMC_CCNT) != 0) {
			/*
			 * XXX - It's not quite clear that this is the proper
			 * way to handle this case (here or in the other
			 * counters below).
			 */
			pmcs = p->p_md.pmc_state;
			pmcs->pmcv[__PMC_CCNT_I] += 0x100000000ULL;
			__asm volatile("mcr p14, 0, %0, c1, c0, 0"
			    : : "r" (pmcs->pmcr[__PMC_CCNT_I]));
		}
	}

	/*
	 * If we have recorded an PMC0 overflow...
	 */
	if (pmnc & PMNC_PMN0_IF) {
		if (pmc_profiling_enabled & __PMC0) {
			proftick(frame);
			__asm volatile("mcr p14, 0, %0, c2, c0, 0"
			    : : "r" (pmc_reset_vals[__PMC0_I]));
		} else if ((p = curproc) != NULL &&
			   (p->p_md.pmc_enabled & __PMC0) != 0) {
			/*
			 * XXX - should handle wrapping the counter.
			 */
			pmcs = p->p_md.pmc_state;
			pmcs->pmcv[__PMC0_I] += 0x100000000ULL;
			__asm volatile("mcr p14, 0, %0, c2, c0, 0"
			    : : "r" (pmcs->pmcr[__PMC0_I]));
		}
	}

	/*
	 * If we have recorded an PMC1 overflow...
	 */
	if (pmnc & PMNC_PMN1_IF) {
		if (pmc_profiling_enabled & __PMC1) {
			proftick(frame);
			__asm volatile("mcr p14, 0, %0, c3, c0, 0"
			    : : "r" (pmc_reset_vals[__PMC1_I]));
		} else if ((p = curproc) != NULL &&
			   (p->p_md.pmc_enabled & __PMC1) != 0) {
			pmcs = p->p_md.pmc_state;
			pmcs->pmcv[__PMC1_I] += 0x100000000ULL;
			__asm volatile("mcr p14, 0, %0, c3, c0, 0"
			    : : "r" (pmcs->pmcr[__PMC1_I]));
		}
	}

	/*
	 * If any overflows were set, this will clear them.
	 * It will also re-enable the counters.
	 */
	xscale_pmnc_write(pmnc);

	splx(s);

	return 1;
}

static void
xscale_fork(struct proc *p1, struct proc *p2)
{
	struct xscale_pmc_state *pmcs_p1, *pmcs_p2;

	p2->p_md.pmc_enabled = p1->p_md.pmc_enabled;
	p2->p_md.pmc_state = malloc(sizeof(struct xscale_pmc_state),
				    M_PROC, M_NOWAIT);
	if (p2->p_md.pmc_state == NULL) {
		/* XXX
		 * Can't return failure at this point, so just disable
		 * PMC for new process.
		 */
		p2->p_md.pmc_enabled = 0;
		return;
	}
	pmcs_p1 = p1->p_md.pmc_state;
	pmcs_p2 = p2->p_md.pmc_state;
	pmcs_p2->pmnc = pmcs_p1->pmnc;
	pmcs_p2->pmcv[0] = 0;
	pmcs_p2->pmcv[1] = 0;
	pmcs_p2->pmcv[2] = 0;
	pmcs_p2->pmc_accv[0] = 0;
	pmcs_p2->pmc_accv[1] = 0;
	pmcs_p2->pmc_accv[2] = 0;
	if (p2->p_md.pmc_enabled & __PMC_CCNT)
		pmc_usecount[__PMC_CCNT_I]++;
	if (p2->p_md.pmc_enabled & __PMC0)
		pmc_usecount[__PMC0_I]++;
	if (p2->p_md.pmc_enabled & __PMC1)
		pmc_usecount[__PMC1_I]++;
}

static int
xscale_num_counters(void)
{
	return __PMC_NCTRS;
}

static int
xscale_counter_type(int ctr)
{
	int	ret;

	switch (ctr) {
	case __PMC_CCNT_I:
		ret = PMC_TYPE_I80200_CCNT;
		break;
	case __PMC0_I:
	case __PMC1_I:
		ret = PMC_TYPE_I80200_PMCx;
		break;
	case -1:
		ret = PMC_CLASS_I80200;
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static void
xscale_save_context(struct proc *p)
{
	struct xscale_pmc_state *pmcs;
	uint32_t pmnc, val;

	if (p && p->p_md.pmc_state) {
		pmcs = (struct xscale_pmc_state *) p->p_md.pmc_state;

		/* disable counters */
		pmnc = xscale_pmnc_read() & ~PMNC_E;
		xscale_pmnc_write(pmnc);

		/* do not save pmnc */

		if (p->p_md.pmc_enabled & __PMC_CCNT) {
			/* save ccnt */
			__asm volatile("mrc p14, 0, %0, c1, c0, 0"
			    : "=r" (val));
			pmcs->pmcv[__PMC_CCNT_I] &= ~0xffffffffULL;
			pmcs->pmcv[__PMC_CCNT_I] |= val;
		}

		if (p->p_md.pmc_enabled & __PMC0) {
			/* save pmc0 */
			__asm volatile("mrc p14, 0, %0, c2, c0, 0"
			    : "=r" (val));
			pmcs->pmcv[__PMC0_I] &= ~0xffffffffULL;
			pmcs->pmcv[__PMC0_I] |= val;
		}

		if (p->p_md.pmc_enabled & __PMC1) {
			/* save pmc1 */
			__asm volatile("mrc p14, 0, %0, c3, c0, 0"
			    : "=r" (val));
			pmcs->pmcv[__PMC1_I] &= ~0xffffffffULL;
			pmcs->pmcv[__PMC1_I] |= val;
		}

		if (pmc_kernel_bits) {
			__asm volatile("mcr p14, 0, %0, c0, c0, 0"
			    : : "r" (pmc_kernel_bits | PMNC_E));
		}
	}
}

static void
xscale_restore_context(struct proc *p)
{
	struct xscale_pmc_state *pmcs;
	register_t r = 0;
	uint32_t val;

	if (p && p->p_md.pmc_state) {
		pmcs = (struct xscale_pmc_state *) p->p_md.pmc_state;

		if (p->p_md.pmc_enabled & __PMC1) {
			/* restore pmc1 */
			val = pmcs->pmcv[__PMC1_I] & 0xffffffffULL;
			__asm volatile("mcr p14, 0, %0, c3, c0, 0" : :
			    "r" (val));
		}

		if (p->p_md.pmc_enabled & __PMC0) {
			/* restore pmc0 */
			val = pmcs->pmcv[__PMC0_I] & 0xffffffffULL;
			__asm volatile("mcr p14, 0, %0, c2, c0, 0" : :
			    "r" (val));
		}

		if (p->p_md.pmc_enabled & __PMC_CCNT) {
			/* restore ccnt */
			val = pmcs->pmcv[__PMC_CCNT_I] & 0xffffffffULL;
			__asm volatile("mcr p14, 0, %0, c1, c0, 0" : :
			    "r" (val));
		}

		if (p->p_md.pmc_enabled)
			r = pmcs->pmnc;
	}

	if (r | pmc_kernel_bits) {
		/* restore pmnc & enable counters */
		__asm volatile("mcr p14, 0, %0, c0, c0, 0"
		    : : "r" (r | pmc_kernel_bits | PMNC_E));
	}
}

static void
xscale_accumulate(struct proc *parent, struct proc *child)
{
	struct xscale_pmc_state *pmcs_parent, *pmcs_child;

	pmcs_parent = parent->p_md.pmc_state;
	pmcs_child = child->p_md.pmc_state;
	if (pmcs_parent && pmcs_child) {
		pmcs_parent->pmc_accv[__PMC_CCNT_I] +=
			pmcs_child->pmcv[__PMC_CCNT_I];
		pmcs_parent->pmc_accv[__PMC0_I] += pmcs_child->pmcv[__PMC0_I];
		pmcs_parent->pmc_accv[__PMC1_I] += pmcs_child->pmcv[__PMC1_I];
	}
}

static void
xscale_process_exit(struct proc *p)
{
	int	i;

	if (!p)
		return;

	for (i=0 ; i<__PMC_NCTRS ; i++) {
		if (p->p_md.pmc_enabled & (1 << i)) {
			pmc_usecount[i]--; 
			p->p_md.pmc_enabled &= ~(1 << i);
		}
	}
	if (p->p_md.pmc_state)
		free(p->p_md.pmc_state, M_PROC);
	p->p_md.pmc_state = NULL;
	p->p_md.pmc_enabled = 0;
}

static void
xscale_enable_counter(struct proc *p, int ctr)
{
	int current = (p == curproc);

	if (ctr < 0 || ctr >= __PMC_NCTRS || !p)
		return;

	if (current)
		pmc_save_context(p);

	if ((p->p_md.pmc_enabled & (1 << ctr)) == 0) {
		pmc_usecount[ctr]++; 
		p->p_md.pmc_enabled |= (1 << ctr);
	}

	if (current)
		pmc_restore_context(p);
}

static void
xscale_disable_counter(struct proc *p, int ctr)
{
	int current = (p == curproc);

	if (ctr < 0 || ctr >= __PMC_NCTRS || !p)
		return;

	if (current)
		pmc_save_context(p);

	if (p->p_md.pmc_enabled & (1 << ctr)) {
		pmc_usecount[ctr]--; 
		p->p_md.pmc_enabled &= ~(1 << ctr);
	}

	if (current)
		pmc_restore_context(p);
}

static int
xscale_counter_isrunning(struct proc *p, int ctr)
{

	if (ctr < 0 || ctr >= __PMC_NCTRS)
		return -1;

	return ((pmc_kernel_enabled | p->p_md.pmc_enabled) & (1 << ctr));
}

static int
xscale_counter_isconfigured(struct proc *p, int ctr)
{

	return ((ctr >= 0) && (ctr < __PMC_NCTRS));
}

static int
xscale_configure_counter(struct proc *p, int ctr, struct pmc_counter_cfg *cfg)
{
	struct xscale_pmc_state *pmcs;
	int current = (p == curproc);

	if (ctr < 0 || ctr >= __PMC_NCTRS || !p)
		return EINVAL;

	if (pmc_kernel_enabled & (1 << ctr))
		return EBUSY;

	if (ctr) {
		if ((cfg->event_id > 0x16) || ((cfg->event_id & 0xe) == 0xe)
		    || (cfg->event_id == 0x13))
			return ENODEV;
	} else {
		if (cfg->event_id != 0x100 && cfg->event_id != 0x101)
			return ENODEV;
	}

	if (current)
		pmc_save_context(p);

	if (p->p_md.pmc_state == NULL) {
		p->p_md.pmc_state = malloc(sizeof(struct xscale_pmc_state),
					   M_PROC, M_WAITOK);
		if (!p->p_md.pmc_state)
			return ENOMEM;
	}
	pmcs = p->p_md.pmc_state;

	switch (ctr) {
	case __PMC_CCNT_I:
		pmcs->pmnc &= ~PMNC_D;
		pmcs->pmnc |= (PMNC_CC_IF | PMNC_CC_IE);
		if (cfg->event_id == 0x101)
			pmcs->pmnc |= PMNC_D;
		break;
	case __PMC0_I:
		pmcs->pmnc &= ~PMNC_EVCNT0_MASK;
		pmcs->pmnc |=   (cfg->event_id << PMNC_EVCNT0_SHIFT)
				| (PMNC_PMN0_IF | PMNC_PMN0_IE);
		break;
	case __PMC1_I:
		pmcs->pmnc &= ~PMNC_EVCNT1_MASK;
		pmcs->pmnc |=   (cfg->event_id << PMNC_EVCNT1_SHIFT)
				| (PMNC_PMN1_IF | PMNC_PMN1_IE);
		break;
	}
	pmcs->pmcr[ctr] = (uint32_t) -((int32_t) cfg->reset_value);
	pmcs->pmcv[ctr] = pmcs->pmcr[ctr];

	if (current)
		pmc_restore_context(p);

	return 0;
}

static int
xscale_get_counter_value(struct proc *p, int ctr, int flags, uint64_t *pval)
{
	struct xscale_pmc_state *pmcs;
	uint32_t val;

	if (ctr < 0 || ctr >= __PMC_NCTRS)
		return EINVAL;

	if (p) {
		pmcs = p->p_md.pmc_state;

		if (flags & PMC_VALUE_FLAGS_CHILDREN) {
			*pval = pmcs->pmc_accv[ctr];
			return 0;
		}
		if (p != curproc) {
			*pval = pmcs->pmcv[ctr];
			return 0;
		}
	}

	switch (ctr) {
	case __PMC_CCNT_I:
		__asm volatile("mrc p14, 0, %0, c1, c0, 0" : "=r" (val));
		break;
	case __PMC0_I:
		__asm volatile("mrc p14, 0, %0, c2, c0, 0" : "=r" (val));
		break;
	case __PMC1_I:
		__asm volatile("mrc p14, 0, %0, c3, c0, 0" : "=r" (val));
		break;
	default:
		val = 0;
		break;
	}

	*pval = val;
	if (p) {
		pmcs = p->p_md.pmc_state;
		*pval += pmcs->pmcv[ctr];
	}

	return 0;
}

static int
xscale_start_profiling(int ctr, struct pmc_counter_cfg *cfg)
{
	struct proc *p = curproc;
	int s;

	if (ctr < 0 || ctr >= __PMC_NCTRS)
		return EINVAL;

	if ((pmc_usecount[ctr] > 0) || (pmc_kernel_enabled & (1 << ctr)))
		return EBUSY;

	if (ctr) {
		if ((cfg->event_id > 0x16) || ((cfg->event_id & 0xe) == 0xe)
		    || (cfg->event_id == 0x13))
			return ENODEV;
	} else {
		if (cfg->event_id != 0x100 && cfg->event_id != 0x101)
			return ENODEV;
	}

	pmc_save_context(p);

	pmc_reset_vals[ctr] = (uint32_t) -((int32_t) cfg->reset_value);

	s = splstatclock();

	switch (ctr) {
	case __PMC_CCNT_I:
		pmc_kernel_bits &= PMNC_D;
		pmc_kernel_bits |= PMNC_CC_IE;
		if (cfg->event_id)
			pmc_kernel_bits |= PMNC_D;
		__asm volatile("mcr p14, 0, %0, c1, c0, 0" : :
		    "r" (pmc_reset_vals[__PMC_CCNT_I]));
		__asm volatile("mcr p14, 0, %0, c0, c0, 0" : :
		    "r" (PMNC_CC_IF));
		break;
	case __PMC0_I:
		pmc_kernel_bits &= ~PMNC_EVCNT0_MASK;
		pmc_kernel_bits |= (cfg->event_id << PMNC_EVCNT0_SHIFT)
		    | PMNC_PMN0_IE;
		__asm volatile("mcr p14, 0, %0, c2, c0, 0" : :
		    "r" (pmc_reset_vals[__PMC0_I]));
		__asm volatile("mcr p14, 0, %0, c0, c0, 0" : :
		    "r" (PMNC_PMN0_IF));
		break;
	case __PMC1_I:
		pmc_kernel_bits &= ~PMNC_EVCNT1_MASK;
		pmc_kernel_bits |= (cfg->event_id << PMNC_EVCNT1_SHIFT)
		    | PMNC_PMN1_IE;
		__asm volatile("mcr p14, 0, %0, c3, c0, 0" : :
		    "r" (pmc_reset_vals[__PMC1_I]));
		__asm volatile("mcr p14, 0, %0, c0, c0, 0" : :
		    "r" (PMNC_PMN1_IF));
		break;
	}

	profsrc |= 1;
	pmc_kernel_enabled |= (1 << ctr);
	pmc_profiling_enabled |= (1 << ctr);

	pmc_restore_context(p);

	splx(s);

	return 0;
}

static int
xscale_stop_profiling(int ctr)
{
	struct proc *p = curproc;
	uint32_t save;

	if (ctr < 0 || ctr >= __PMC_NCTRS)
		return EINVAL;

	if (!(pmc_kernel_enabled & (1 << ctr)))
		return 0;

	save = pmc_kernel_bits;
	pmc_kernel_bits = 0;
	pmc_save_context(p);
	pmc_kernel_bits = save;

	switch (ctr) {
	case __PMC_CCNT_I:
		pmc_kernel_bits &= (PMNC_D | PMNC_CC_IE);
		break;
	case __PMC0_I:
		pmc_kernel_bits &= ~(PMNC_EVCNT0_MASK | PMNC_PMN0_IE);
		break;
	case __PMC1_I:
		pmc_kernel_bits &= ~(PMNC_EVCNT1_MASK | PMNC_PMN1_IE);
		break;
	}

	pmc_kernel_enabled &= ~(1 << ctr);
	pmc_profiling_enabled &= ~(1 << ctr);

	if (pmc_profiling_enabled == 0)
		profsrc &= ~1;

	pmc_restore_context(p);

	return 0;
}

static int
xscale_alloc_kernel_counter(int ctr, struct pmc_counter_cfg *cfg)
{
	struct proc *p = curproc;

	if (ctr < 0 || ctr >= __PMC_NCTRS)
		return EINVAL;

	if ((pmc_usecount[ctr] > 0) || (pmc_kernel_enabled & (1 << ctr)))
		return EBUSY;

	if (ctr) {
		if ((cfg->event_id > 0x16) || ((cfg->event_id & 0xe) == 0xe)
		    || (cfg->event_id == 0x13))
			return ENODEV;
	} else {
		if (cfg->event_id != 0x100 && cfg->event_id != 0x101)
			return ENODEV;
	}

	pmc_save_context(p);

	pmc_reset_vals[ctr] = (uint32_t) -((int32_t) cfg->reset_value);

	switch (ctr) {
	case __PMC_CCNT_I:
		pmc_kernel_bits &= PMNC_D;
		if (cfg->event_id)
			pmc_kernel_bits |= PMNC_D;
		__asm volatile("mcr p14, 0, %0, c1, c0, 0" : :
		    "r" (pmc_reset_vals[__PMC_CCNT_I]));
		break;
	case __PMC0_I:
		pmc_kernel_bits &= ~PMNC_EVCNT0_MASK;
		pmc_kernel_bits |= (cfg->event_id << PMNC_EVCNT0_SHIFT);
		__asm volatile("mcr p14, 0, %0, c2, c0, 0" : :
		    "r" (pmc_reset_vals[__PMC0_I]));
		break;
	case __PMC1_I:
		pmc_kernel_bits &= ~PMNC_EVCNT1_MASK;
		pmc_kernel_bits |= (cfg->event_id << PMNC_EVCNT1_SHIFT);
		__asm volatile("mcr p14, 0, %0, c3, c0, 0" : :
		    "r" (pmc_reset_vals[__PMC1_I]));
		break;
	}

	pmc_kernel_enabled |= (1 << ctr);

	pmc_restore_context(p);

	return 0;
}

static int
xscale_free_kernel_counter(int ctr)
{
	if (ctr < 0 || ctr >= __PMC_NCTRS)
		return EINVAL;

	if (!(pmc_kernel_enabled & (1 << ctr)))
		return 0;

	pmc_kernel_enabled &= ~(1 << ctr);

	return 0;
}

struct arm_pmc_funcs xscale_pmc_funcs = {
	xscale_fork,
	xscale_num_counters,
	xscale_counter_type,
	xscale_save_context,
	xscale_restore_context,
	xscale_enable_counter,
	xscale_disable_counter,
	xscale_accumulate,
	xscale_process_exit,
	xscale_configure_counter,
	xscale_get_counter_value,
	xscale_counter_isconfigured,
	xscale_counter_isrunning,
	xscale_start_profiling,
	xscale_stop_profiling,
	xscale_alloc_kernel_counter,
	xscale_free_kernel_counter
};

void
xscale_pmu_init(void)
{
	arm_pmc = &xscale_pmc_funcs;
}
