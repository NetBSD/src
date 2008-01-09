/*
 * Define the SDP2430 specific information and then include the generic OMAP
 * interrupt header.
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "opt_omap.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap2_icu.c,v 1.1.2.2 2008/01/09 01:45:21 matt Exp $");

#include <sys/param.h>
#include <sys/evcnt.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>
#include <machine/atomic.h>
#include <arm/softintr.h>
#include <arm/omap/omap2430reg.h>
#include <arm/omap/omap2430reg.h>

#include <machine/bus.h>

#ifdef OMAP_2430
#define	NIGROUPS	8

#define	GPIO1_BASE	GPIO1_BASE_2430
#define	GPIO2_BASE	GPIO2_BASE_2430
#define	GPIO3_BASE	GPIO3_BASE_2430
#define	GPIO4_BASE	GPIO4_BASE_2430
#define	GPIO5_BASE	GPIO5_BASE_2430
#elif defined(OMAP_2420)
#define	NIGROUPS	7

#define	GPIO1_BASE	GPIO1_BASE_2420
#define	GPIO2_BASE	GPIO2_BASE_2420
#define	GPIO3_BASE	GPIO3_BASE_2420
#define	GPIO4_BASE	GPIO4_BASE_2420
#endif

#define	IRQ_SOFTSERIAL	M_IRQ_9
#define	IRQ_SOFTCLOCK	M_IRQ_22
#define	IRQ_SOFTNET	M_IRQ_49
#define	IRQ_SOFT	M_IRQ_95

#define	SOFTIPL_MASK	(BIT(IPL_SOFT)|BIT(IPL_SOFTCLOCK)|\
			 BIT(IPL_SOFTNET)|BIT(IPL_SOFTSERIAL))

struct intrsource {
	struct evcnt is_ev;
	uint8_t is_ipl;
	uint8_t is_group;
	int (*is_func)(void *);
	void *is_arg;
	uint64_t is_marked;
};

static struct intrgroup {
	uint32_t ig_irqsbyipl[NIPL];
	uint32_t ig_irqs;
	volatile uint32_t ig_pending_irqs;
	uint32_t ig_enabled_irqs;
	uint32_t ig_edge_rising;
	uint32_t ig_edge_falling;
	uint32_t ig_level_low;
	uint32_t ig_level_high;
	struct intrsource ig_sources[32];
	bus_space_tag_t ig_memt;
	bus_space_handle_t ig_memh;
} intrgroups[NIGROUPS] = {
	[0].ig_sources[ 0 ... 31 ].is_group = 0,
	[1].ig_sources[ 0 ... 31 ].is_group = 1,
	[2].ig_sources[ 0 ... 31 ].is_group = 2,
	[3].ig_sources[ 0 ... 31 ].is_group = 3,
	[4].ig_sources[ 0 ... 31 ].is_group = 4,
	[5].ig_sources[ 0 ... 31 ].is_group = 5,
	[6].ig_sources[ 0 ... 31 ].is_group = 6,
	[7].ig_sources[ 0 ... 31 ].is_group = 7,
	[IRQ_SOFTSERIAL/32].ig_sources[IRQ_SOFTSERIAL&31] = {
		.is_ev = EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL,
					   "soft serial", "intr"),
		.is_ipl = IPL_SOFTSERIAL,
		.is_func = (int (*)(void *)) softintr_dispatch,
		.is_arg = (void *) SI_SOFTSERIAL,
		.is_group = IRQ_SOFTSERIAL/32,
	},
	[IRQ_SOFTCLOCK/32].ig_sources[IRQ_SOFTCLOCK&31] = {
		.is_ev = EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL,
					   "soft clock", "intr"),
		.is_ipl = IPL_SOFTCLOCK,
		.is_func = (int (*)(void *)) softintr_dispatch,
		.is_arg = (void *) SI_SOFTCLOCK,
		.is_group = IRQ_SOFTCLOCK/32,
	},
	[IRQ_SOFTNET/32].ig_sources[IRQ_SOFTNET&31] = {
		.is_ev = EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL,
					   "soft net", "intr"),
		.is_ipl = IPL_SOFTNET,
		.is_func = (int (*)(void *)) softintr_dispatch,
		.is_arg = (void *) SI_SOFTNET,
		.is_group = IRQ_SOFTNET/32,
	},
	[IRQ_SOFT/32].ig_sources[IRQ_SOFT&31] = {
		.is_ev = EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL,
					   "soft", "intr"),
		.is_ipl = IPL_SOFT,
		.is_func = (int (*)(void *)) softintr_dispatch,
		.is_arg = (void *) SI_SOFT,
		.is_group = IRQ_SOFT/32,
	},
};

volatile uint32_t pending_ipls;
volatile uint32_t pending_igroupsbyipl[NIPL];
void omap2430_intr_init(bus_space_tag_t);

#define	INTC_READ(ig, o)		\
	bus_space_read_4((ig)->ig_memt, (ig)->ig_memh, o)
#define	INTC_WRITE(ig, o, v)	\
	bus_space_write_4((ig)->ig_memt, (ig)->ig_memh, o, v)
#define	GPIO_READ(ig, o)		\
	bus_space_read_4((ig)->ig_memt, (ig)->ig_memh, o)
#define	GPIO_WRITE(ig, o, v)	\
	bus_space_write_4((ig)->ig_memt, (ig)->ig_memh, o, v)

static void
unblock_irq(unsigned int group, int irq_mask)
{
	struct intrgroup * const ig = &intrgroups[group];
	KASSERT((irq_mask & ig->ig_enabled_irqs) == 0);
	ig->ig_enabled_irqs |= irq_mask;
	if (group < 3) {
		INTC_WRITE(ig, INTC_MIR_CLEAR, irq_mask);
	} else {
		GPIO_WRITE(ig, GPIO_SETIRQENABLE1, irq_mask);
		/*
		 * Clear IRQSTATUS of level interrupts, if they are still
		 * asserted, IRQSTATUS will become set again and they will
		 * refire.  This avoids one spurious interrupt for every
		 * real interrupt.
		 */
		if (irq_mask & (ig->ig_level_low|ig->ig_level_high))
			GPIO_WRITE(ig, GPIO_IRQSTATUS1,
			    irq_mask & (ig->ig_level_low|ig->ig_level_high));
	}

	/* Force INTC to recompute IRQ availability */
	INTC_WRITE(&intrgroups[0], INTC_CONTROL, INTC_CONTROL_NEWIRQAGR);
}

static void
block_irq(unsigned int group, int irq_mask)
{
	struct intrgroup * const ig = &intrgroups[group];
	ig->ig_enabled_irqs &= ~irq_mask;
	if (group < 3) {
		INTC_WRITE(ig, INTC_MIR_SET, irq_mask);
		return;
	}
	GPIO_WRITE(ig, GPIO_CLEARIRQENABLE1, irq_mask);
	/*
	 * Only clear(reenable) edge interrupts.
	 */
	if (irq_mask & (ig->ig_edge_falling|ig->ig_edge_rising))
		GPIO_WRITE(ig, GPIO_IRQSTATUS1, /* reset int bits */
		    irq_mask & (ig->ig_edge_falling|ig->ig_edge_rising));
}

static void
init_irq(int irq, int spl, int type)
{
	struct intrgroup * const ig = &intrgroups[irq / 32];
	uint32_t irq_mask = BIT(irq & 31);
	uint32_t v;

	KASSERT(irq >= 0 && irq < 256);
	ig->ig_sources[irq & 31].is_ipl = spl;
	if (irq < 96) {
		KASSERT(type == IST_LEVEL);
		return;
	}

	ig->ig_enabled_irqs &= ~irq_mask;
	GPIO_WRITE(ig, GPIO_CLEARIRQENABLE1, irq_mask);

	v = GPIO_READ(ig, GPIO_OE);
	GPIO_WRITE(ig, GPIO_OE, v | irq_mask);	/* set as input */

	ig->ig_edge_rising &= ~irq_mask;
	ig->ig_edge_falling &= ~irq_mask;
	ig->ig_level_low &= ~irq_mask;
	ig->ig_level_high &= ~irq_mask;

	switch (type) {
	case IST_EDGE_BOTH:
		ig->ig_edge_rising |= irq_mask;
		ig->ig_edge_falling |= irq_mask;
		break;
	case IST_EDGE_RISING:
		ig->ig_edge_rising |= irq_mask;
		break;
	case IST_EDGE_FALLING:
		ig->ig_edge_falling |= irq_mask;
		break;
	case IST_LEVEL_LOW:
		ig->ig_level_low |= irq_mask;
		break;
	case IST_LEVEL_HIGH:
		ig->ig_level_high |= irq_mask;
		break;
	}

	GPIO_WRITE(ig, GPIO_LEVELDETECT0, ig->ig_level_low);
	GPIO_WRITE(ig, GPIO_LEVELDETECT1, ig->ig_level_high);
	GPIO_WRITE(ig, GPIO_RISINGDETECT, ig->ig_edge_rising);
	GPIO_WRITE(ig, GPIO_FALLINGDETECT, ig->ig_edge_falling);
}

/*
 * Called with interrupt disabled
 */
static void
calculate_irq_masks(struct intrgroup *ig)
{
	u_int irq;
	int ipl;
	uint32_t irq_mask;

	memset(ig->ig_irqsbyipl, 0, sizeof(ig->ig_irqsbyipl));
	ig->ig_irqs = 0;

	for (irq_mask = 1, irq = 0; irq < 32; irq_mask <<= 1, irq++) {
		if ((ipl = ig->ig_sources[irq].is_ipl) == IPL_NONE)
			continue;

		ig->ig_irqsbyipl[ipl] |= irq_mask;
		ig->ig_irqs |= irq_mask;
	}
}

/*
 * Called with interrupts disabled
 */
static uint32_t
mark_pending_irqs(int group, uint32_t pending)
{
	struct intrgroup * const ig = &intrgroups[group];
	struct intrsource *is;
	int n;
	int ipl_mask = 0;

	if (pending == 0)
		return ipl_mask;

	KASSERT((ig->ig_enabled_irqs & pending) == pending);
	KASSERT((ig->ig_pending_irqs & pending) == 0);

	ig->ig_pending_irqs |= pending;
	block_irq(group, pending);
	for (;;) {
		n = ffs(pending);
		if (n-- == 0)
			break;
		is = &ig->ig_sources[n];
		KASSERT(ig->ig_irqsbyipl[is->is_ipl] & pending);
		pending &= ~ig->ig_irqsbyipl[is->is_ipl];
		ipl_mask |= BIT(is->is_ipl);
		KASSERT(ipl_mask < BIT(NIPL));
		pending_igroupsbyipl[is->is_ipl] |= BIT(group);
		is->is_marked++;
	}
	KASSERT(ipl_mask < BIT(NIPL));
	return ipl_mask;
}

/*
 * Called with interrupts disabled
 */
static uint32_t
get_pending_irqs(void)
{
	uint32_t pending[3];
	uint32_t ipl_mask = 0;
	uint32_t xpending;

	pending[0] = INTC_READ(&intrgroups[0], INTC_PENDING_IRQ);
	pending[1] = INTC_READ(&intrgroups[1], INTC_PENDING_IRQ);
	pending[2] = INTC_READ(&intrgroups[2], INTC_PENDING_IRQ);

	/* Get interrupt status of GPIO1 */
	if (pending[GPIO1_MPU_IRQ / 32] & BIT(GPIO1_MPU_IRQ & 31)) {
		KASSERT(intrgroups[3].ig_enabled_irqs);
		xpending = GPIO_READ(&intrgroups[3], GPIO_IRQSTATUS1);
		xpending &= intrgroups[3].ig_enabled_irqs;
		ipl_mask |= mark_pending_irqs(3, xpending);
	}

	/* Get interrupt status of GPIO2 */
	if (pending[GPIO2_MPU_IRQ / 32] & BIT(GPIO2_MPU_IRQ & 31)) {
		KASSERT(intrgroups[4].ig_enabled_irqs);
		xpending = GPIO_READ(&intrgroups[4], GPIO_IRQSTATUS1);
		xpending &= intrgroups[4].ig_enabled_irqs;
		ipl_mask |= mark_pending_irqs(4, xpending);
	}

	/* Get interrupt status of GPIO3 */
	if (pending[GPIO3_MPU_IRQ / 32] & BIT(GPIO3_MPU_IRQ & 31)) {
		KASSERT(intrgroups[5].ig_enabled_irqs);
		xpending = GPIO_READ(&intrgroups[5], GPIO_IRQSTATUS1);
		xpending &= intrgroups[5].ig_enabled_irqs;
		ipl_mask |= mark_pending_irqs(5, xpending);
	}

	/* Get interrupt status of GPIO4 */
	if (pending[GPIO4_MPU_IRQ / 32] & BIT(GPIO4_MPU_IRQ & 31)) {
		KASSERT(intrgroups[6].ig_enabled_irqs);
		xpending = GPIO_READ(&intrgroups[6], GPIO_IRQSTATUS1);
		xpending &= intrgroups[6].ig_enabled_irqs;
		ipl_mask |= mark_pending_irqs(6, xpending);
	}

	/* Get interrupt status of GPIO5 */
	if (pending[GPIO5_MPU_IRQ / 32] & BIT(GPIO5_MPU_IRQ & 31)) {
		KASSERT(intrgroups[7].ig_enabled_irqs);
		xpending = GPIO_READ(&intrgroups[7], GPIO_IRQSTATUS1);
		xpending = GPIO_READ(&intrgroups[7], GPIO_IRQSTATUS1);
		xpending &= intrgroups[7].ig_enabled_irqs;
		ipl_mask |= mark_pending_irqs(7, xpending);
	}

	/* Clear GPIO indication from summaries */
	pending[GPIO1_MPU_IRQ / 32] &= ~BIT(GPIO1_MPU_IRQ & 31);
	pending[GPIO2_MPU_IRQ / 32] &= ~BIT(GPIO2_MPU_IRQ & 31);
	pending[GPIO3_MPU_IRQ / 32] &= ~BIT(GPIO3_MPU_IRQ & 31);
	pending[GPIO4_MPU_IRQ / 32] &= ~BIT(GPIO4_MPU_IRQ & 31);
	pending[GPIO5_MPU_IRQ / 32] &= ~BIT(GPIO5_MPU_IRQ & 31);

	/* Now handle the primaries interrupt summaries */
	ipl_mask |= mark_pending_irqs(0, pending[0]);
	ipl_mask |= mark_pending_irqs(1, pending[1]);
	ipl_mask |= mark_pending_irqs(2, pending[2]);

	/* Force INTC to recompute IRQ availability */
	INTC_WRITE(&intrgroups[0], INTC_CONTROL, INTC_CONTROL_NEWIRQAGR);

	return ipl_mask;
}

static int last_delivered_ipl;
static u_long no_pending_irqs[NIPL][NIGROUPS];

static void
deliver_irqs(register_t psw, int ipl, void *frame)
{
	struct intrgroup *ig;
	struct intrsource *is;
	uint32_t pending_irqs;
	uint32_t irq_mask;
	uint32_t blocked_irqs;
	volatile uint32_t * const pending_igroups = &pending_igroupsbyipl[ipl];
	const uint32_t ipl_mask = BIT(ipl);
	int n;
	int saved_ipl = IPL_NONE;	/* XXX stupid GCC */
	unsigned int group;
	int rv;

	if (frame == NULL) {
		saved_ipl = last_delivered_ipl;
		KASSERT(saved_ipl < ipl);
		last_delivered_ipl = ipl;
	}
	/*
	 * We only must be called is there is this IPL has pending interrupts
	 * and therefore there must be at least one intrgroup with a pending
	 * interrupt.
	 */
	KASSERT(pending_ipls & ipl_mask);
	KASSERT(*pending_igroups);

	/*
	 * We loop until there are no more intrgroups with pending interrupts.
	 */
	do {
		group = 31 - __builtin_clz(*pending_igroups);
		KASSERT(group < NIGROUPS);

		ig = &intrgroups[group];
		irq_mask = ig->ig_irqsbyipl[ipl];
		pending_irqs = ig->ig_pending_irqs & irq_mask;
		blocked_irqs = pending_irqs;
		if ((*pending_igroups &= ~BIT(group)) == 0)
			pending_ipls &= ~ipl_mask;
#if 0
		KASSERT(group < 3 || (GPIO_READ(ig, GPIO_IRQSTATUS1) & blocked_irqs) == 0);
#endif
		/*
		 * We couldn't gotten here unless there was at least one
		 * pending interrupt in this intrgroup.
		 */
		if (pending_irqs == 0) {
			no_pending_irqs[ipl][group]++;
			continue;
		}
#if 0
		KASSERT(pending_irqs != 0);
#endif
		do {
			n = 31 - __builtin_clz(pending_irqs);
			KASSERT(ig->ig_irqs & BIT(n));
			KASSERT(irq_mask & BIT(n));

			/*
			 * If this was the last bit cleared for this IRQ,
			 * we need to clear this group's bit in 
			 * pending_igroupsbyipl[ipl].  Now if that's now 0,
			 * we need to clear pending_ipls for this IPL.
			 */
			ig->ig_pending_irqs &= ~BIT(n);
			if (irq_mask == BIT(n))
				KASSERT((ig->ig_pending_irqs & irq_mask) == 0);
			is = &ig->ig_sources[n];
			if (__predict_false(frame != NULL)) {
				(*is->is_func)(frame);
			} else {
				restore_interrupts(psw);
				rv = (*is->is_func)(is->is_arg);
				disable_interrupts(I32_bit);
			}
#if 0
			if (rv && group >= 3) /* XXX */
				GPIO_WRITE(ig, GPIO_IRQSTATUS1, BIT(n));
#endif
#if 0
			if (ig->ig_irqsbyipl[ipl] == BIT(n))
				KASSERT((ig->ig_pending_irqs & irq_mask) == 0);
#endif
			is->is_ev.ev_count++;
			pending_irqs = ig->ig_pending_irqs & irq_mask;
		} while (pending_irqs);
		/*
		 * We don't block the interrupts individually because even if
		 * one was unblocked it couldn't be delivered since our
		 * current IPL would prevent it.  So we wait until we can do
		 * them all at once.
		 */
#if 0
		KASSERT(group < 3 || (GPIO_READ(ig, GPIO_IRQSTATUS1) & blocked_irqs) == 0);
#endif
		if ((ipl_mask & SOFTIPL_MASK) == 0)
			unblock_irq(group, blocked_irqs);
	} while (*pending_igroups);
	/*
	 * Since there are no more pending interrupts for this IPL,
	 * this IPL must not be present in the pending IPLs.
	 */
	KASSERT((pending_ipls & ipl_mask) == 0);
	KASSERT((intrgroups[0].ig_pending_irqs & intrgroups[0].ig_irqsbyipl[ipl]) == 0);
	KASSERT((intrgroups[1].ig_pending_irqs & intrgroups[1].ig_irqsbyipl[ipl]) == 0);
	KASSERT((intrgroups[2].ig_pending_irqs & intrgroups[2].ig_irqsbyipl[ipl]) == 0);
	KASSERT((intrgroups[3].ig_pending_irqs & intrgroups[3].ig_irqsbyipl[ipl]) == 0);
	KASSERT((intrgroups[4].ig_pending_irqs & intrgroups[4].ig_irqsbyipl[ipl]) == 0);
	KASSERT((intrgroups[5].ig_pending_irqs & intrgroups[5].ig_irqsbyipl[ipl]) == 0);
	KASSERT((intrgroups[6].ig_pending_irqs & intrgroups[6].ig_irqsbyipl[ipl]) == 0);
	KASSERT((intrgroups[7].ig_pending_irqs & intrgroups[7].ig_irqsbyipl[ipl]) == 0);
	if (frame == NULL)
		last_delivered_ipl = saved_ipl;
}

static inline void
do_pending_ints(register_t psw, int newipl)
{
	struct cpu_info * const ci = curcpu();

	while ((pending_ipls & ~BIT(newipl)) > BIT(newipl)) {
		KASSERT(pending_ipls < BIT(NIPL));
		for (;;) {
			int ipl = 31 - __builtin_clz(pending_ipls);
			KASSERT(ipl < NIPL);
			if (ipl <= newipl)
				break;
		
			ci->ci_cpl = ipl;
			deliver_irqs(psw, ipl, NULL);
		}
	}
	ci->ci_ipl = newipl;
}

int
_splraise(int newipl)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	KASSERT(newipl < NIPL);
	if (newipl > ci->ci_cpl)
		ci->ci_cpl = newipl;
	return oldipl;
}
int
_spllower(int newipl)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	KASSERT(panicstr || newipl <= ci->ci_cpl);
	if (newipl < ci->ci_cpl) {
		register_t psw = disable_interrupts(I32_bit);
		do_pending_ints(psw, newipl);
		restore_interrupts(psw);
	}
	return oldipl;
}

void
splx(int savedipl)
{
	struct cpu_info * const ci = curcpu();
	KASSERT(savedipl < NIPL);
	if (savedipl < ci->ci_cpl) {
		register_t psw = disable_interrupts(I32_bit);
		do_pending_ints(psw, savedipl);
		restore_interrupts(psw);
	}
	ci->ci_cpl = savedipl;
}

static struct intrsource * const si_to_is[4] = {
	[SI_SOFTSERIAL] =
	    &intrgroups[IRQ_SOFTSERIAL / 32].ig_sources[IRQ_SOFTSERIAL & 31],
	[SI_SOFTCLOCK] =
	    &intrgroups[IRQ_SOFTCLOCK / 32].ig_sources[IRQ_SOFTCLOCK & 31],
	[SI_SOFTNET] =
	    &intrgroups[IRQ_SOFTNET / 32].ig_sources[IRQ_SOFTNET & 31],
	[SI_SOFT] =
	    &intrgroups[IRQ_SOFT / 32].ig_sources[IRQ_SOFT & 31],
};

void
_setsoftintr(int si)
{
	struct intrsource * const is = si_to_is[si];
	struct intrgroup * const ig = &intrgroups[is->is_group];

	if (__predict_false(ci->ci_cpl < is->is_ipl)) {
		/*
		 * If we are less than the desired IPL, raise IPL and dispatch
		 * it immediately.  This is improbable.
		 */
		int s = _splraise(is->is_ipl);
		softintr_dispatch(si);
		splx(s);
	} else {
		/*
		 * Mark the software interrupt for delivery.
		 */
		register_t psw = disable_interrupts(I32_bit);
		ig->ig_pending_irqs |= BIT(is - ig->ig_sources);
		pending_igroupsbyipl[is->is_ipl] |= BIT(is->is_group);
		pending_ipls |= BIT(is->is_ipl);
		is->is_marked++;
		restore_interrupts(psw);
	}
}

void
omap_irq_handler(void *frame)
{
	const int oldipl = curcpl();
	const uint32_t oldipl_mask = BIT(oldipl);

	/*
	 * When we enter there must be no pending IRQs for IPL greater than
	 * the current IPL.  There might be pending IRQs for the current IPL
	 * if we are servicing interrupts.
	 */
	KASSERT((pending_ipls & ~oldipl_mask) < oldipl_mask);
	pending_ipls |= get_pending_irqs();

	uvmexp.intrs++;
	/*
	 * We assume this isn't a clock intr.  But if it is, deliver it 
	 * unconditionally so it will always have the interrupted frame.
	 * The clock intr will handle being called at IPLs != IPL_CLOCK.
	 */
	if (__predict_false(pending_ipls & BIT(IPL_STATCLOCK))) {
		deliver_irqs(0, IPL_STATCLOCK, frame);
		pending_ipls &= ~BIT(IPL_STATCLOCK);
	}
	if (__predict_false(pending_ipls & BIT(IPL_CLOCK))) {
		deliver_irqs(0, IPL_CLOCK, frame);
		pending_ipls &= ~BIT(IPL_CLOCK);
	}

	/*
	 * Record the pending_ipls and deliver them if we can.
	 */
	if ((pending_ipls & ~oldipl_mask) > oldipl_mask)
		do_pending_ints(I32_bit, oldipl);
}

void *
omap_intr_establish(int irq, int ipl, const char *name,
	int (*func)(void *), void *arg)
{
	struct intrgroup *ig = &intrgroups[irq / 32];
	struct intrsource *is;
	register_t psw;

	KASSERT(irq >= 0 && irq < 256);
	is = &ig->ig_sources[irq & 0x1f];
	KASSERT(irq != GPIO1_MPU_IRQ);
	KASSERT(irq != GPIO2_MPU_IRQ);
	KASSERT(irq != GPIO3_MPU_IRQ);
	KASSERT(irq != GPIO4_MPU_IRQ);
	KASSERT(irq != GPIO5_MPU_IRQ);
	KASSERT(is->is_ipl == IPL_NONE);

	is->is_func = func;
	is->is_arg = arg;
	psw = disable_interrupts(I32_bit);
	evcnt_attach_dynamic(&is->is_ev, EVCNT_TYPE_INTR, NULL, name, "intr");
	init_irq(irq, ipl, IST_LEVEL);

	calculate_irq_masks(ig);
	unblock_irq(is->is_group, BIT(irq & 31));
	restore_interrupts(psw);
	return is;
}

void
omap_intr_disestablish(void *ih)
{
	struct intrsource * const is = ih;
	struct intrgroup *ig;
	register_t psw;
	uint32_t mask;

	KASSERT(ih != NULL);

	ig = &intrgroups[is->is_group];
	psw = disable_interrupts(I32_bit);
	mask = BIT(is - ig->ig_sources);
	block_irq(is->is_group, mask);
	ig->ig_pending_irqs &= ~mask;
	calculate_irq_masks(ig);
	evcnt_detach(&is->is_ev);
	restore_interrupts(psw);
}

#ifdef GPIO5_BASE
static void
gpio5_clkinit(bus_space_tag_t memt)
{
	bus_space_handle_t memh;
	uint32_t r;
	int error;

	error = bus_space_map(memt, OMAP2430_CM_BASE,
	    OMAP2430_CM_SIZE, 0, &memh);
	if (error != 0)
		panic("%s: cannot map OMAP2430_CM_BASE at %#x: %d\n",
			__func__, OMAP2430_CM_BASE, error);

	r = bus_space_read_4(memt, memh, OMAP2430_CM_FCLKEN2_CORE);
	r |= OMAP2430_CM_FCLKEN2_CORE_EN_GPIO5;
	bus_space_write_4(memt, memh, OMAP2430_CM_FCLKEN2_CORE, r);

	r = bus_space_read_4(memt, memh, OMAP2430_CM_ICLKEN2_CORE);
	r |= OMAP2430_CM_ICLKEN2_CORE_EN_GPIO5;
	bus_space_write_4(memt, memh, OMAP2430_CM_ICLKEN2_CORE, r);

	bus_space_unmap(memt, memh, OMAP2430_CM_SIZE);
}
#endif

void
omap2430_intr_init(bus_space_tag_t memt)
{
	int error;
	int group;

	evcnt_attach_static(&intrgroups[IRQ_SOFTSERIAL/32].ig_sources[IRQ_SOFTSERIAL&31].is_ev);
	evcnt_attach_static(&intrgroups[IRQ_SOFTCLOCK/32].ig_sources[IRQ_SOFTCLOCK&31].is_ev);
	evcnt_attach_static(&intrgroups[IRQ_SOFTNET/32].ig_sources[IRQ_SOFTNET&31].is_ev);
	evcnt_attach_static(&intrgroups[IRQ_SOFT/32].ig_sources[IRQ_SOFT&31].is_ev);

	for (group = 0; group < NIGROUPS; group++)
		intrgroups[group].ig_memt = memt;
	error = bus_space_map(memt, INTC_BASE, 0x1000, 0,
	    &intrgroups[0].ig_memh);
	if (error)
		panic("failed to map interrupt registers: %d", error);
	error = bus_space_subregion(memt, intrgroups[0].ig_memh, 0x20, 0x20,
	    &intrgroups[1].ig_memh);
	if (error)
		panic("failed to region interrupt registers: %d", error);
	error = bus_space_subregion(memt, intrgroups[0].ig_memh, 0x40, 0x20,
	    &intrgroups[2].ig_memh);
	if (error)
		panic("failed to subregion interrupt registers: %d", error);
	error = bus_space_map(memt, GPIO1_BASE, 0x400, 0,
	    &intrgroups[3].ig_memh);
	if (error)
		panic("failed to map gpio #1 registers: %d", error);
	error = bus_space_map(memt, GPIO2_BASE, 0x400, 0,
	    &intrgroups[4].ig_memh);
	if (error)
		panic("failed to map gpio #2 registers: %d", error);
	error = bus_space_map(memt, GPIO3_BASE, 0x400, 0,
	    &intrgroups[5].ig_memh);
	if (error)
		panic("failed to map gpio #3 registers: %d", error);
	error = bus_space_map(memt, GPIO4_BASE, 0x400, 0,
	    &intrgroups[6].ig_memh);
	if (error)
		panic("failed to map gpio #4 registers: %d", error);

#ifdef GPIO5_BASE
	gpio5_clkinit(memt);
	error = bus_space_map(memt, GPIO5_BASE, 0x400, 0,
	    &intrgroups[7].ig_memh);
	if (error)
		panic("failed to map gpio #5 registers: %d", error);
#endif

	INTC_WRITE(&intrgroups[0], INTC_MIR_SET, 0xffffffff);
	INTC_WRITE(&intrgroups[1], INTC_MIR_SET, 0xffffffff);
	INTC_WRITE(&intrgroups[2], INTC_MIR_SET, 0xffffffff);
	INTC_WRITE(&intrgroups[GPIO1_MPU_IRQ / 32], INTC_MIR_CLEAR,
	    BIT(GPIO1_MPU_IRQ & 31));
	INTC_WRITE(&intrgroups[GPIO2_MPU_IRQ / 32], INTC_MIR_CLEAR,
	    BIT(GPIO2_MPU_IRQ & 31));
	INTC_WRITE(&intrgroups[GPIO3_MPU_IRQ / 32], INTC_MIR_CLEAR,
	    BIT(GPIO3_MPU_IRQ & 31));
	INTC_WRITE(&intrgroups[GPIO4_MPU_IRQ / 32], INTC_MIR_CLEAR,
	    BIT(GPIO4_MPU_IRQ & 31));
#ifdef GPIO5_BASE
	INTC_WRITE(&intrgroups[GPIO5_MPU_IRQ / 32], INTC_MIR_CLEAR,
	    BIT(GPIO5_MPU_IRQ & 31));
#endif

	/*
	 * Setup the primary intrgroups.
	 */
	calculate_irq_masks(&intrgroups[0]);
	calculate_irq_masks(&intrgroups[1]);
	calculate_irq_masks(&intrgroups[2]);
}
