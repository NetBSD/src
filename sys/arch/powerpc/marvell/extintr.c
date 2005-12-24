/*	$NetBSD: extintr.c,v 1.15 2005/12/24 20:07:28 perry Exp $	*/

/*
 * Copyright (c) 2002 Allegro Networks, Inc., Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Allegro Networks, Inc., and Wasabi Systems, Inc.
 * 4. The name of Allegro Networks, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 * 5. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ALLEGRO NETWORKS, INC. AND
 * WASABI SYSTEMS, INC. ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL EITHER ALLEGRO NETWORKS, INC. OR WASABI SYSTEMS, INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * extintr.c - external interrupt management for discovery
 *
 *	Interrupts are software-prioritized and preempting,
 *	they are only actually masked when pending.
 *	this allows avoiding slow, off-CPU mask reprogramming for spl/splx.
 *	When a lower priority interrupt preempts a high one,
 *	it is pended and masked.  Masks are re-enabled after service.
 *
 *	`ci->ci_cpl'   is a "priority level" not a bitmask.
 *	`irq'   is a bit number in the 128 bit imask_t which reflects
 *		the GT-64260 Main Cause register pair (64 bits), and
 *		GPP Cause register (32 bits) interrupts.
 *
 *	Intra IPL dispatch order is defined in cause_irq()
 *
 *	Summary bits in cause registers are not valid IRQs
 *
 * 	Within a cause register bit vector ISRs are called in
 *	order of IRQ (descending).
 *
 *	When IRQs are shared, ISRs are called in order on the queue
 *	(which is arbitrarily first-established-first-served).
 *
 *	GT64260 GPP setup is for edge-triggered interrupts.
 *	We maintain a mask of level-triggered type IRQs
 *	and gather unlatched level from the GPP Value register.
 *
 *	Software interrupts are just like regular IRQs,
 *	they are established, pended, and dispatched exactly the
 *	same as HW interrupts.
 *	
 *	128 bit imask_t operations could be implemented with Altivec
 *	("vand", "vor", etc however no "vcntlzw" or "vffs"....)
 *
 * creation	Tue Feb  6 17:27:18 PST 2001	cliff
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: extintr.c,v 1.15 2005/12/24 20:07:28 perry Exp $");

#include "opt_marvell.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <machine/psl.h>
#include <machine/bus.h>
#include <machine/intr.h>
#ifdef KGDB
#include <machine/db_machdep.h>
#endif
#include <dev/marvell/gtreg.h>
#include <dev/marvell/gtvar.h>
#include <dev/marvell/gtintrreg.h>

#include <net/netisr.h>

#include <uvm/uvm_extern.h>

#if defined(DEBUG) && defined(DDB)
#endif

#ifdef DEBUG
# define STATIC
int intrdebug = 0;
# define DPRINTF(x)		do { if (intrdebug) printf x ; } while (0)
# define DPRINTFN(n, x)		do { if (intrdebug > (n)) printf x ; } while (0)
#else
# define STATIC static
# define DPRINTF(x)
# define DPRINTFN(n, x)
#endif

#ifdef DIAGNOSTIC
# define DIAGPRF(x)		printf x
#else
# define DIAGPRF(x)
#endif

#define ILLEGAL_IRQ(x) (((x) < 0) || ((x) >= NIRQ) || \
		 ((1<<((x)&IMASK_BITMASK)) & imres.bits[(x)>>IMASK_WORDSHIFT]))

extern struct powerpc_bus_space gt_mem_bs_tag; 
extern bus_space_handle_t gt_memh; 
extern int safepri;

struct intrsource {
	struct intrhand *is_hand;
	struct evcnt is_evcnt;
	int is_level;
	int is_type;

	const char *is_name;
};

/*
 * Interrupt handler chains.  intr_establish() inserts a handler into
 * the list.  The handler is called with its (single) argument.
 */
struct intrhand {
	int	(*ih_fun)(void *);
	void	*ih_arg;
	struct	intrhand *ih_next;
	struct	intrsource *ih_source;
	int	ih_flags;
	int	ih_level;	/* sanity only */
	u_long	ih_count;
	int	ih_softimask;
};
#define	IH_ACTIVE	0x01

struct intrsource intr_sources[NIRQ];

struct intrhand *softnet_handlers[32];

const char intr_source_strings[NIRQ][16] = {
	"unknown 0",	"dev",		"dma",		"cpu",
	"idma 01",	"idma 23",	"idma 45",	"idma 67",
	"timer 01",	"timer 23",	"timer 45",	"timer 67",
	"pci0-0",	"pci0-1",	"pci0-2",	"pci0-3",
/*16*/	"pci1-0",	"ecc",		"pci1-1",	"pci1-2",
	"pci1-3",	"pci0-outl",	"pci0-outh",	"pci1-outl",
	"pci1-outh",	"unknown 25",	"pci0-inl",	"pci0-inh",
	"pci1-inl",	"pci1-inh",	"unknown 30",	"unknown 31",
/*32*/	"ether 0",	"ether 1",	"ether 2",	"unknown 35",
	"sdma",		"iic",		"unknown 38",	"brg",
	"mpsc 0",	"unknown 41",	"mpsc 1",	"comm",
	"unknown 44",	"unknown 45",	"unknown 46",	"unknown 47",
/*48*/	"unknown 48",	"unknown 49",	"unknown 50",	"unknown 51",
	"unknown 52",	"unknown 53",	"unknown 54",	"unknown 55",
	"gppsum 0",	"gppsum 1",	"gppsum 2",	"gppsum 3",
	"unknown 60",	"unknown 61",	"unknown 62",	"unknown 63",
/*64*/	"gpp 0",	"gpp 1",	"gpp 2",	"gpp 3",
	"gpp 4",	"gpp 5",	"gpp 6",	"gpp 7",
	"gpp 8",	"gpp 9",	"gpp 10",	"gpp 11",
	"gpp 12",	"gpp 13",	"gpp 14",	"gpp 15",
/*80*/	"gpp 16",	"gpp 17",	"gpp 18",	"gpp 19",
	"gpp 20",	"gpp 21",	"gpp 22",	"gpp 23",
	"gpp 24",	"gpp 25",	"gpp 26",	"gpp 27",
	"gpp 28",	"gpp 29",	"gpp 30",	"gpp 31",
/*96*/	"soft 0",	"soft 1",	"soft 2",	"soft 3",
	"soft 4",	"soft 5",	"soft 6",	"soft 7",
	"soft 8",	"soft 9",	"soft 10",	"soft 11",
	"soft 12",	"soft 13",	"soft 14",	"soft 15",
/*112*/	"soft 16",	"soft 17",	"soft 18",	"soft 19",
	"soft 20",	"soft 21",	"soft 22",	"soft 23",
	"soft 24",	"soft 25",	"soft 26",	"soft clock",
	"soft net",	"soft iic",	"soft serial",	"hwclock",
};

#ifdef EXT_INTR_STATS
#define EXT_INTR_STAT_HISTSZ 4096
ext_intr_stat_t ext_intr_stats[NIRQ] = {{0},};
ext_intr_hist_t ext_intr_hist[NIRQ][EXT_INTR_STAT_HISTSZ];
#endif


#define GPP_RES ~GT_MPP_INTERRUPTS	/* from config */

/*
 * ipending and imen HW IRQs require MSR[EE]=0 protection
 */
volatile imask_t ipending    __attribute__ ((aligned(CACHELINESIZE)));
volatile imask_t imen        __attribute__ ((aligned(CACHELINESIZE)));
imask_t          imask[NIPL] __attribute__ ((aligned(CACHELINESIZE)));
const imask_t    imres       = 
	 { { (IML_RES|IML_SUM), (IMH_RES|IMH_GPP_SUM), GPP_RES, SIR_RES } };

#ifdef DEBUG
struct intrframe *intrframe = 0; 
static int intr_depth;
#define EXTINTR_SANITY_SZ	16
struct {
	unsigned int ipl;
	imask_t oimen;
} extintr_sanity[EXTINTR_SANITY_SZ] = {{ 0, }};

#define EXTINTR_SANITY()  \
	if (intr_depth < EXTINTR_SANITY_SZ) { \
		extintr_sanity[intr_depth].ipl = ci->ci_cpl; \
		imask_dup_v(&extintr_sanity[intr_depth].oimen, &imen); \
	} \
	intr_depth++
#else
#define EXTINTR_SANITY()
#endif	/* DEBUG */

const char * const intr_typename_strings[NIST] = {
	"(none)",
	"pulse",
	"edge",
	"level",
	"soft",
	"clock"
};
u_int32_t gpp_intrtype_level_mask = 0;

STATIC void	softintr_init(void);
STATIC void	write_intr_mask(volatile imask_t *);
STATIC void	intr_calculatemasks(void);
STATIC int	ext_intr_cause(volatile imask_t *, volatile imask_t *,
				const imask_t *);
STATIC int	cause_irq(const imask_t *, const imask_t *);

static inline void
imask_print(char *str, volatile imask_t *imp)
{
	DPRINTF(("%s: %#10x %#10x %#10x %#10x\n",
		str,
		imp->bits[IMASK_ICU_LO],
		imp->bits[IMASK_ICU_HI],
		imp->bits[IMASK_ICU_GPP],
		imp->bits[IMASK_SOFTINT]));
}

/*
 * Count leading zeros.
 */
static inline int
cntlzw(int x)
{
	int a;

	__asm volatile ("cntlzw %0,%1" : "=r"(a) : "r"(x));

	return a;
}

/*
 * softintr_init - establish softclock, softnet; reserve SIR_HWCLOCK
 */
STATIC void
softintr_init(void)
{
	intr_sources[SIR_HWCLOCK].is_type = IST_CLOCK;	/* exclusive */

#define DONETISR(n, f) \
	softnet_handlers[(n)] = \
	    softintr_establish(IPL_SOFTNET, (void (*)(void *))(f), NULL)
#include <net/netisr_dispatch.h>
#undef DONETISR
}

/*
 * initialize interrupt subsystem
 */
void
init_interrupt(void)
{
	safepri = IPL_NONE;
	intr_calculatemasks();
	EXT_INTR_STATS_INIT();
	SPL_STATS_INIT();
	softintr_init();
}

const char *
intr_typename(int type)
{
	if (type >= NIST)
		return "illegal";
	return intr_typename_strings[type];
}

/*
 * intr_establish - register an interrupt handler.
 */
void *
intr_establish(
	int irq,
	int type,
	int level,
	int (*ih_fun) __P((void *)),
	void *ih_arg)
{
	struct intrsource *is = &intr_sources[irq];
	struct intrhand **p, *ih;
	register_t omsr;

	if (ILLEGAL_IRQ(irq)) {
		DIAGPRF(("intr_establish: illegal irq 0x%x", irq));
		return NULL;
	}
	if (level <= IPL_NONE || level >= IPL_HIGH) {
		DIAGPRF(("intr_establish: illegal level %d\n", level));
		return NULL;
	}
	if (type <= IST_NONE || type >= NIST) {
		DIAGPRF(("intr_establish: illegal type 0x%x", type));
		return NULL;
	}
	if (((irq >= SIR_BASE) && (type != IST_SOFT))
	||  ((irq <  SIR_BASE) && (type == IST_SOFT))) {
		DIAGPRF(("intr_establish: irq %d out of range for type %s\n",
			irq, intr_typename_strings[type]));
		return NULL;
	}
	if (is->is_type != IST_NONE && is->is_type != type) {
		DIAGPRF(("intr_establish: irq %d: can't share %s with %s",
			irq, intr_typename_strings[is->is_type],
			intr_typename_strings[type]));
		return NULL;
	}
	if (is->is_level != IPL_NONE && is->is_level != level) {
		DIAGPRF(("intr_establish: irq %d conflicting levels %d, %d\n",
			irq, is->is_level, level));
		return NULL;
	}

	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL) {
		DIAGPRF(("intr_establish: can't malloc handler info"));
		return NULL;
	}

	omsr = extintr_disable();

	if (is->is_hand == NULL) {
		is->is_evcnt.ev_count = 0;
		evcnt_attach_dynamic(&is->is_evcnt, EVCNT_TYPE_INTR,
		    NULL, "discovery", intr_source_strings[irq]);
	}

	is->is_type = type;
	is->is_level = level;

	if ((IRQ_IS_GPP(irq)) && type == IST_LEVEL)
		gpp_intrtype_level_mask |= (1 << (irq - IRQ_GPP_BASE));

	for (p = &is->is_hand; *p != NULL; p = &(*p)->ih_next)
		;
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_level = level;
	ih->ih_source = is;
	ih->ih_next = NULL;
	ih->ih_count = 0;
	if (irq >= SIR_BASE) {
		ih->ih_flags = 0;
		ih->ih_softimask = SIBIT(irq);
	} else {
		ih->ih_flags = IH_ACTIVE;
		ih->ih_softimask = 0;
	}
	*p = ih;

	intr_calculatemasks();

	extintr_restore(omsr);

	return (ih);
}

/*
 * intr_disestablish - deregister an interrupt handler.
 */
void
intr_disestablish(void *arg)
{
	struct intrhand *ih = arg;
	struct intrsource *is = ih->ih_source;
	int irq = is - intr_sources;
	struct intrhand **p;
	register_t omsr;

	if (ILLEGAL_IRQ(irq))
		panic("intr_disestablish: bogus irq %d", irq);

	/*
	 * Remove the handler from the chain.
	 * This is O(n^2), too.
	 */
	omsr = extintr_disable();
	for (p = &is->is_hand; *p != NULL && (*p) != ih; p = &(*p)->ih_next)
		;
	if (*p != NULL)
		*p = (*p)->ih_next;
	else
		panic("intr_disestablish: handler not registered");
	free((void *)ih, M_DEVBUF);

	intr_calculatemasks();

	if (is->is_hand == NULL) {
		evcnt_detach(&is->is_evcnt);
		if ((IRQ_IS_GPP(irq)) && (is->is_type == IST_LEVEL))
			gpp_intrtype_level_mask &= ~(1 << (irq - IRQ_GPP_BASE));
		is->is_level = IPL_NONE;
		is->is_type = IST_NONE;
	}
	extintr_restore(omsr);
}

const char *
intr_string(int irq)
{
	return intr_source_strings[irq];
}

const struct evcnt *
intr_evcnt(int irq)
{
	return &intr_sources[irq].is_evcnt;
}

/*
 * write_intr_mask - update GT-64260 ICU Mask registers
 * 
 *	- IRQs already pending are always disabled
 *	- ICU summary bits are always enabled
 *
 *	always called with interrupts disabled
 */
STATIC void
write_intr_mask(volatile imask_t *imp)
{
	u_int32_t lo;
	u_int32_t hi;
	u_int32_t gpp;

	imask_andnot_icu_vv(imp, &ipending);

	lo  = imp->bits[IMASK_ICU_LO];
	hi  = imp->bits[IMASK_ICU_HI];
	gpp = imp->bits[IMASK_ICU_GPP];

	lo |= IML_SUM;
	hi |= IMH_GPP_SUM;

	bus_space_write_4(&gt_mem_bs_tag, gt_memh, ICR_CIM_LO, lo);
	bus_space_write_4(&gt_mem_bs_tag, gt_memh, ICR_CIM_HI, hi);
	bus_space_write_4(&gt_mem_bs_tag, gt_memh, GT_GPP_Interrupt_Mask, gpp);
	(void)bus_space_read_4(&gt_mem_bs_tag, gt_memh,
	    GT_GPP_Interrupt_Mask);	/* R.A.W. */
}

/*
 * intr_calculatemasks - generate dispatch tables based on established IRQs
 *
 *	enforces uniform level and intrtype for all handlers of shared IRQ
 *	in case of conflict simply create a new IPL
 *
 *	always called with interrupts disabled
 */
STATIC void
intr_calculatemasks(void)
{
	int irq, level;
	struct intrhand *q;
	struct intrsource *is;

	/*
	 * level to use for each IRQ is defined by first to claim it
	 * if the IRQ is shared, level cannot vary
	 */
	for (irq = 0, is = intr_sources; irq < NIRQ; irq++, is++) {
		for (q = is->is_hand; q; q = q->ih_next)
			if (q->ih_level != is->is_level)
				panic("intr_calculatemasks: irq %d "
					"conflicting levels %d, %d\n",
						irq, q->ih_level, is->is_level);
	}

	/*
	 * imask[] indicates IRQs enabled at each level.
	 */
	for (level = 0; level < NIPL; level++) {
		imask_t *imp = &imask[level];
		imask_zero(imp);
		for (irq = 0, is = intr_sources; irq < NIRQ; irq++, is++) {
			if (level < is->is_level) {
				imask_orbit(imp, irq);
			}
		}
	}

	/* enable IRQs with handlers */
	imask_zero_v(&imen);
	for (irq = 0, is = intr_sources; irq < NIRQ; irq++, is++) {
		if (is->is_hand) {
			imask_orbit_v(&imen, irq);
		}
	}

	/*
	 * enable all live interrupts
	 */
	write_intr_mask(&imen);
}

/*
 * ext_intr_cause - collect, ack (GPP), pend and mask ICU causes
 *
 *	set (OR-in) current ICU causes in impendp
 *
 *	read GT-64260 Main Interrupt Cause and
 *	GPP Interrupt Cause and Value registers
 *
 * 	always called with interrupts disabled
 */
STATIC int
ext_intr_cause(
	volatile imask_t *impendp,
	volatile imask_t *imenp,
	const imask_t *imresp)
{
	u_int32_t lo;
	u_int32_t hi;
	u_int32_t gpp = 0;
	u_int32_t gpp_v;

	lo = bus_space_read_4(&gt_mem_bs_tag, gt_memh, ICR_MIC_LO);
	lo &= ~(imresp->bits[IMASK_ICU_LO] | impendp->bits[IMASK_ICU_LO]);
	imenp->bits[IMASK_ICU_LO] &= ~lo;

	hi = bus_space_read_4(&gt_mem_bs_tag, gt_memh, ICR_MIC_HI);

	if (hi & IMH_GPP_SUM) {
		gpp = bus_space_read_4(&gt_mem_bs_tag, gt_memh, GT_GPP_Interrupt_Cause);
		gpp &= ~imresp->bits[IMASK_ICU_GPP];
		bus_space_write_4(&gt_mem_bs_tag, gt_memh, GT_GPP_Interrupt_Cause, ~gpp);
	}

	gpp_v = bus_space_read_4(&gt_mem_bs_tag, gt_memh, GT_GPP_Value);
	KASSERT((gpp_intrtype_level_mask & imresp->bits[IMASK_ICU_GPP]) == 0);
	gpp_v &= (gpp_intrtype_level_mask & imenp->bits[IMASK_ICU_GPP]);

	gpp |= gpp_v;
	gpp &= ~impendp->bits[IMASK_ICU_GPP];
	imenp->bits[IMASK_ICU_GPP] &= ~gpp;

	hi &= ~(imresp->bits[IMASK_ICU_HI] | impendp->bits[IMASK_ICU_HI]);
	imenp->bits[IMASK_ICU_HI] &= ~hi;

	write_intr_mask(imenp);

	EXT_INTR_STATS_CAUSE(lo, hi, gpp, 0);

	if ((lo | hi | gpp) == 0) {
		return 0;			/* nothing left */
	}

	/*
	 * post pending IRQs in caller's imask
	 */
	impendp->bits[IMASK_ICU_LO] |= lo;
	impendp->bits[IMASK_ICU_HI] |= hi;
	impendp->bits[IMASK_ICU_GPP] |= gpp;
	EXT_INTR_STATS_PEND(lo, hi, gpp, 0);

	return 1;
}

/*
 * cause_irq - find a qualified irq
 *
 *	`cause' indicates pending interrupt causes
 *	`mask'  indicates what interrupts are enabled
 *
 *	dispatch order is: lo, hi, gpp, soft
 *
 * return -1 if no qualified interrupts are pending.
 */
STATIC int
cause_irq(const imask_t *cause, const imask_t *mask)
{
	u_int32_t clo  = cause->bits[IMASK_ICU_LO];
	u_int32_t chi  = cause->bits[IMASK_ICU_HI];
	u_int32_t cgpp = cause->bits[IMASK_ICU_GPP];
	u_int32_t csft = cause->bits[IMASK_SOFTINT];
	u_int32_t mlo  = mask->bits[IMASK_ICU_LO];
	u_int32_t mhi  = mask->bits[IMASK_ICU_HI];
	u_int32_t mgpp = mask->bits[IMASK_ICU_GPP];
	u_int32_t msft = mask->bits[IMASK_SOFTINT];
	int irq;

	clo  &= mlo;
	chi  &= mhi;
	cgpp &= mgpp;
	csft &= msft;
	if ((clo | chi | cgpp | csft) == 0)
		return -1;
	if ((irq = 31 - cntlzw(clo)) >= 0)
		return irq;
	if ((irq = 31 - cntlzw(chi)) >= 0)
		return (irq + 32);
	if ((irq = 31 - cntlzw(cgpp)) >= 0)
		return irq + IRQ_GPP_BASE;
	if ((irq = 31 - cntlzw(csft)) >= 0)
		return (irq + SIR_BASE);
	return -1;
}

/*
 * ext_intr - external interrupt service routine
 *
 *	collect and post filtered causes, then have a go at dispatch
 *	always called (from locore.S) with interrupts disabled
 */
void
ext_intr(struct intrframe *frame)
{
#ifdef DEBUG
	struct cpu_info * const ci = curcpu();
	struct intrframe *oframe;
#endif
	EXT_INTR_STATS_DECL(tstart);

	EXT_INTR_STATS_DEPTH();

#ifdef DEBUG
	if (extintr_disable() & PSL_EE)
		panic("ext_intr: PSL_EE");
	oframe = intrframe;
	intrframe = frame;
	EXTINTR_SANITY();
#endif

	if (ext_intr_cause(&ipending, &imen, &imres) == 0) {
		DIAGPRF(("ext_intr: stray interrupt\n"));
#ifdef DEBUG
		DPRINTF(("cpl %d\n", ci->ci_cpl));
		imask_print("imask[cpl]", (volatile imask_t *)&imask[ci->ci_cpl]);
		imask_print("ipending  ", &ipending);
		imask_print("imen      ", &imen);
#endif
	} else {
		do {
			intr_dispatch();
		}  while (ext_intr_cause(&ipending, &imen, &imres) != 0);
	}
#ifdef DEBUG
	intrframe = oframe;
	intr_depth--;
#endif
}

/*
 * intr_dispatch - process pending and soft interrupts
 *
 *	always called with interrupts disabled
 *	pending interrupts must be masked off in ICU
 */
void
intr_dispatch(void)
{
	struct cpu_info * const ci = curcpu();
	struct intrhand *ih;
	imask_t *imp;
	int irq;
	int ipl;
        int ocpl;
        imask_t imdisp;  
	EXT_INTR_STATS_DECL(tstart);

	KASSERT((extintr_disable() & PSL_EE) == 0);
	KASSERT(ci->ci_cpl < IPL_HIGH);
	KASSERT(ci->ci_cpl >= IPL_NONE);

	if (ci->ci_cpl >= IPL_HIGH)
		return;
	ocpl = ci->ci_cpl;

        /*
         * process pending irpts that are not masked at ocpl and above
         * copy and clear these from ipending while irpts are disabled
         */
loop:
	imask_dup_v(&imdisp, &ipending);
	imask_and(&imdisp, &imask[ocpl]);

	imdisp.bits[IMASK_SOFTINT] &= imen.bits[IMASK_SOFTINT];
	imen.bits[IMASK_SOFTINT] &= ~imdisp.bits[IMASK_SOFTINT];

	if (imask_empty(&imdisp)) {
		ci->ci_cpl = ocpl;
		SPL_STATS_LOG(ocpl, 0);
		return;
	}

	imask_andnot_v(&ipending, &imdisp);
	EXT_INTR_STATS_COMMIT_M(&imdisp);

	imp = &imask[IPL_HIGH-1];
	for (ipl=IPL_HIGH; ipl > ocpl; ipl--) {
		ci->ci_cpl = ipl;
		SPL_STATS_LOG(ipl, 0);
		while ((irq = cause_irq(&imdisp, imp)) >= 0) {
			struct intrsource *is = &intr_sources[irq];
#ifdef DEBUG
			if (imask_andbit_v(&imen, irq) != 0)
				panic("intr_dispatch(a): irq %d enabled", irq);
#endif
			is->is_evcnt.ev_count++;
			EXT_INTR_STATS_PRE(irq, tstart);
			for (ih = is->is_hand; ih != NULL; ih = ih->ih_next) {
				if (ih->ih_flags & IH_ACTIVE) {
					int rv;

					if (irq >= SIR_BASE)
						ih->ih_flags &= ~IH_ACTIVE;
					(void)extintr_enable();
					rv = (*ih->ih_fun)(ih->ih_arg);
					(void)extintr_disable();
					if (rv != 0)
						ih->ih_count++;
				}

				KASSERT(ci->ci_cpl == ipl);
			}
			EXT_INTR_STATS_POST(irq, tstart);
			if (irq >= ICU_LEN)
				uvmexp.softs++;
#ifdef DEBUG
			if (imask_andbit_v(&imen, irq) != 0)
				panic("intr_dispatch(b): irq %d enabled", irq);
#endif
			imask_clrbit(&imdisp, irq);
			imask_orbit_v(&imen, irq);

			if (irq < ICU_LEN)
				write_intr_mask(&imen);
		}
		imp--;
		if (imask_empty(&imdisp))
			break;
 	}
	goto loop;
}

void *
softintr_establish(int level, void (*fun)(void *), void *arg)
{
	int irq = 200;
	switch (level) {
	case IPL_SOFTNET:	irq = SIR_SOFTNET; break;
	case IPL_SOFTCLOCK:	irq = SIR_SOFTCLOCK; break;
	case IPL_SOFTSERIAL:	irq = SIR_SOFTSERIAL; break;
	case IPL_SOFTI2C:	irq = SIR_SOFTI2C; break;
	default:
		panic("softintr_establish: bad level %d", level);
	}

	return intr_establish(irq, IST_SOFT, level, (int (*)(void *))fun, arg);
}

void
softintr_disestablish(void *ih)
{
	intr_disestablish(ih);
}

void
softintr_schedule(void *vih)
{
	struct intrhand *ih = vih;
	register_t omsr;

	omsr = extintr_disable();
	ih->ih_flags |= IH_ACTIVE;
	ipending.bits[IMASK_SOFTINT] |= ih->ih_softimask;
	extintr_restore(omsr);
}

#ifdef EXT_INTR_STATS
/*
 * ISR counts, pended, prempted, timing stats
 */

int intr_depth_max = 0;
int ext_intr_stats_enb = 1;
ext_intr_stat_t ext_intr_stats[NIRQ] = {{0},};
ext_intr_stat_t *ext_intr_statp = 0;

#define EXT_INTR_STAT_HISTSZ 4096
ext_intr_hist_t ext_intr_hist[NIRQ][EXT_INTR_STAT_HISTSZ];

static inline u_int64_t
_mftb()
{
        u_long scratch;
        u_int64_t tb;

        __asm volatile ("1: mftbu %0; mftb %0+1; mftbu %1; cmpw 0,%0,%1; bne 1b"
		: "=r"(tb), "=r"(scratch));
        return tb;
}

STATIC void
_ext_intr_stats_cause(u_int64_t tcause, u_int32_t cause, u_int32_t irqbase)
{
	int irq;
	u_int32_t bit;
	unsigned int ix;
	ext_intr_stat_t *statp;
	ext_intr_hist_t *histp;

	if (cause == 0)
		return;
	while ((irq = 31 - cntlzw(cause)) >= 0) {
		bit = 1 << irq;
		cause &= ~bit;
		statp = &ext_intr_stats[irq + irqbase];
		ix = statp->histix;
		++ix;
		if (ix >= EXT_INTR_STAT_HISTSZ)
			ix = 0;
		statp->histix = ix;
		histp = &statp->histp[ix];
		memset(histp, 0, sizeof(ext_intr_hist_t));
		histp->tcause = tcause;
	}

}

void 
ext_intr_stats_cause(u_int32_t lo, u_int32_t hi, u_int32_t gpp, u_int32_t soft)
{
	u_int64_t tcause;

	tcause = _mftb();

	_ext_intr_stats_cause(tcause, lo, 0);
	_ext_intr_stats_cause(tcause, hi, 32);
	_ext_intr_stats_cause(tcause, gpp, IRQ_GPP_BASE);
	_ext_intr_stats_cause(tcause, soft, SIR_BASE);
}
 
STATIC void
_ext_intr_stats_pend(u_int64_t tpend, u_int32_t cause, u_int32_t irqbase)
{
	int irq;
	u_int32_t bit;
	ext_intr_stat_t *statp;
	ext_intr_hist_t *histp;

	if (cause == 0)
		return;
	while ((irq = 31 - cntlzw(cause)) >= 0) {
		bit = 1 << irq;
		cause &= ~bit;
		EXT_INTR_STATS_PEND_IRQ(irq);
	}

}

void 
ext_intr_stats_pend(u_int32_t lo, u_int32_t hi, u_int32_t gpp, u_int32_t soft)
{
	u_int64_t tpend;
	ext_intr_stat_t *statp;
	ext_intr_hist_t *histp;
	int irq;
	u_int32_t bit;

	tpend = _mftb();

	_ext_intr_stats_pend(tpend, lo, 0);
	_ext_intr_stats_pend(tpend, hi, 32);
	_ext_intr_stats_pend(tpend, gpp, IRQ_GPP_BASE);
	_ext_intr_stats_pend(tpend, soft, SIR_BASE);
}

STATIC void
_ext_intr_stats_commit(u_int64_t tcommit, u_int32_t cause, u_int32_t irqbase)
{
	int irq;
	u_int32_t bit;
	ext_intr_stat_t *statp;
	ext_intr_hist_t *histp;

	if (cause == 0)
		return;
	while ((irq = 31 - cntlzw(cause)) >= 0) {
		bit = 1 << irq;
		cause &= ~bit;
		statp = &ext_intr_stats[irq + irqbase];
		histp = &statp->histp[statp->histix];
		histp->tcommit = tcommit;
	}

}

void 
ext_intr_stats_commit_m(imask_t *imp)
{
	u_int64_t tcommit;
	ext_intr_stat_t *statp;
	ext_intr_hist_t *histp;
	int irq;
	u_int32_t bit;

	tcommit = _mftb();

	_ext_intr_stats_commit(tcommit, (*imp)[IMASK_ICU_LO], 0);
	_ext_intr_stats_commit(tcommit, (*imp)[IMASK_ICU_HI], 32);
	_ext_intr_stats_commit(tcommit, (*imp)[IMASK_ICU_GPP], IRQ_GPP_BASE);
	_ext_intr_stats_commit(tcommit, (*imp)[IMASK_SOFTINT], SIR_BASE);
}

void 
ext_intr_stats_commit_irq(u_int irq)
{
	u_int64_t tcommit;
	ext_intr_stat_t *statp;
	ext_intr_hist_t *histp;
	u_int32_t bit;

	tcommit = _mftb();

	_ext_intr_stats_commit(tcommit, irq & (32-1), irq & ~(32-1));
}

u_int64_t 
ext_intr_stats_pre(int irq)
{
	ext_intr_stat_t *statp;

	statp = &ext_intr_stats[irq];
#ifdef DEBUG
	if (statp == ext_intr_statp)
		panic("ext_intr_stats_pre: statp == ext_intr_statp");
#endif
	statp->save = ext_intr_statp;
	statp->borrowed = 0;
	ext_intr_statp = statp;
	return _mftb();
}

void
ext_intr_stats_post(int irq, u_int64_t tstart)
{
	u_int64_t dt;
	ext_intr_stat_t *sp;
	ext_intr_stat_t *statp=NULL;
	ext_intr_hist_t *histp=NULL;

	dt = _mftb();

	if (ext_intr_stats_enb <= 0) {
		if (ext_intr_stats_enb == -1)
			ext_intr_stats_enb = 1;
		ext_intr_statp = statp->save;
		statp->save = 0;
		return;
	}

	statp = &ext_intr_stats[irq];
	histp = &statp->histp[statp->histix];
	histp->tstart = tstart;
	histp->tfin = dt;
	dt -= tstart;

	sp = statp->save;
	statp->save = 0;
	if (sp) {
#ifdef DEBUG
		if (statp == sp)
			panic("ext_intr_stats_post: statp == sp");
#endif
		sp->preempted[irq]++;
		sp->borrowed += dt;
	}
	dt -= statp->borrowed;
	statp->cnt++;
	statp->sum += dt;

	if ((dt < statp->min) || (statp->min == 0))
		statp->min = dt;
	if ((dt > statp->max) || (statp->max == 0))
		statp->max = dt;

	ext_intr_statp = sp;
}

void
ext_intr_stats_print()
{
	unsigned int i;
	ext_intr_stat_t *statp;
	int irq;
	unsigned int level;
	u_int64_t avg;
	struct intrhand *ih;
	struct intrsource *is;

	printf("intr_depth_max %d\n", intr_depth_max);
	statp = ext_intr_stats;
	for (irq = 0, is = intr_sources; irq < NIRQ; irq++, is++) {
		ih = is->is_hand;
		if (ih != 0) {
			int once = 0;

			avg = statp->sum / statp->cnt;
			level = is->is_level;
			printf("irq %d ", irq);
			printf("lvl %d ", level);
			printf("cnt %lld ", statp->cnt);
			printf("avg %lld ", avg);
			printf("min %lld ", statp->min);
			printf("max %lld ", statp->max);
			printf("pnd %lld\n", statp->pnd);

			for (i=0; i < NIRQ; i++) {
				if (statp->preempted[i]) {
					if (!once) {
						once = 1;
						printf("preemption: ");
					}
					printf("%d:%ld, ", i,
						statp->preempted[i]);
				}
			}
			if (once)
				printf("\n");

			if (ih != 0) {
				printf("ISRs: ");
				do  {
					printf("%p, ", ih->ih_fun);
					ih = ih->ih_next;
				} while (ih);
				printf("\n");
			}
		}
		statp++;
	}
}

void
ext_intr_hist_prsubr(int irq)
{
	unsigned int cnt;
	ext_intr_stat_t *statp;
	ext_intr_hist_t *histp;
	struct intrhand *ih;

	if (irq >= NIRQ)
		return;
	ih = intr_sources[irq].is_hand;
	statp = &ext_intr_stats[irq];
	histp = statp->histp;
	if (ih != 0) {
		cnt = (statp->cnt < EXT_INTR_STAT_HISTSZ) ?
			statp->cnt : EXT_INTR_STAT_HISTSZ;
		printf("irq %d: %d samples:\n", irq, cnt);
		while (cnt--) {
			printf("%llu %llu %llu %llu\n",
				histp->tcause, histp->tcommit,
				histp->tstart, histp->tfin);
			histp++;
		}
	}
}

void
ext_intr_hist_print(u_int32_t lo, u_int32_t hi, u_int32_t gpp, u_int32_t soft)
{
	int irq;
	u_int32_t bit;
	unsigned int omsr;

	omsr = extintr_disable();

	while ((irq = 31 - cntlzw(lo)) >= 0) {
		bit = 1 << irq;
		lo &= ~bit;
		ext_intr_hist_prsubr(irq);
	}

	while ((irq = 31 - cntlzw(hi)) >= 0) {
		bit = 1 << irq;
		hi &= ~bit;
		ext_intr_hist_prsubr(irq + 32);
	}

	while ((irq = 31 - cntlzw(gpp)) >= 0) {
		bit = 1 << irq;
		gpp &= ~bit;
		ext_intr_hist_prsubr(irq + IRQ_GPP_BASE);
	}

	while ((irq = 31 - cntlzw(soft)) >= 0) {
		bit = 1 << irq;
		soft &= ~bit;
		ext_intr_hist_prsubr(irq + SIR_BASE);
	}

	extintr_restore(omsr);
}

void
ext_intr_stats_init()
{
	int irq;

	memset(ext_intr_stats, 0, sizeof(ext_intr_stats));
	memset(ext_intr_hist, 0, sizeof(ext_intr_hist));
	for (irq=0; irq < NIRQ; irq++) {
		ext_intr_stats[irq].histp = &ext_intr_hist[irq][0];
		ext_intr_stats[irq].histix = EXT_INTR_STAT_HISTSZ-1;
	}
}
#endif	/* EXT_INTR_STATS */

#ifdef SPL_STATS
#define SPL_STATS_HISTSZ	EXT_INTR_STAT_HISTSZ
spl_hist_t spl_stats_hist[SPL_STATS_HISTSZ];
unsigned int spl_stats_histix = 0;
unsigned int spl_stats_enb = 1;


void
spl_stats_init()
{
	spl_stats_histix = 0;
}

void
spl_stats_log(int ipl, int cc)
{
	unsigned int ix = spl_stats_histix;
	spl_hist_t *histp;
	register_t *fp;
	register_t *pc;

	if (spl_stats_enb == 0)
		return;
	histp = &spl_stats_hist[ix];

	if (cc == 0) {
		/* log our calling address */
		__asm volatile ("mflr %0;" : "=r"(pc));
		pc--;
	} else {
		/* log our caller's calling address */
		__asm volatile ("mr %0,1;" : "=r"(fp));
		fp = (register_t *)*fp;
		pc = (register_t *)(fp[1] - sizeof(register_t));
	}

	histp->level = ipl;
	histp->addr = (void *)pc;
	histp->time = _mftb();
	if (++ix >= SPL_STATS_HISTSZ)
		ix = 0;
	spl_stats_histix = ix;
}

void
spl_stats_print()
{
	spl_hist_t *histp;
	int i;

	printf("spl stats:\n");
	histp = &spl_stats_hist[0];
	for (i=SPL_STATS_HISTSZ; --i; ) {
		printf("%llu %d %p\n", histp->time, histp->level, histp->addr);
		histp++;
	}
}

#endif	/* SPL_STATS */

#ifndef SPL_INLINE
int
splraise(int ncpl)
{
	struct cpu_info * const ci = curcpu();
	register_t omsr;
	int ocpl;

	omsr = extintr_disable();
	ocpl = ci->ci_cpl; 
        if (ncpl > ci->ci_cpl) {
		SPL_STATS_LOG(ncpl, 1);
                ci->ci_cpl = ncpl;
		if ((ncpl == IPL_HIGH) && ((omsr & PSL_EE) != 0)) {
			/* leave external interrupts disabled */
			return (ocpl | IPL_EE);
		}
	}
        extintr_restore(omsr);
        return (ocpl);
}

void
splx(int xcpl)
{
	struct cpu_info * const ci = curcpu();
	imask_t *ncplp;
	unsigned int omsr;
	int ncpl = xcpl & IPL_PRIMASK;
#ifdef DIAGNOSTIC
	int ocpl = ci->ci_cpl;
#endif

	omsr = extintr_disable();
if (ci->ci_cpl < ncpl) {
printf("ci->ci_cpl = %d, ncpl = %d\n", ci->ci_cpl, ncpl);
}
	KASSERT(ncpl <= ci->ci_cpl);
	if (ncpl < ci->ci_cpl) {
		ci->ci_cpl = ncpl;
		ncplp = &imask[ncpl];
		SPL_STATS_LOG(ncpl, 1);
		if (imask_test_v(&ipending, ncplp))
			intr_dispatch();
	}
	if (xcpl & IPL_EE) {
		KASSERT(ocpl == IPL_HIGH);
		KASSERT((omsr & PSL_EE) == 0);
		omsr |= PSL_EE;
	}
	extintr_restore(omsr);
}

int
spllower(int ncpl)
{
	struct cpu_info * const ci = curcpu();
	int ocpl;
	imask_t *ncplp;
	unsigned int omsr;

	ncpl &= IPL_PRIMASK;
	ncplp = &imask[ncpl];

	omsr = extintr_disable();
	ocpl = ci->ci_cpl;
	ci->ci_cpl = ncpl;
	SPL_STATS_LOG(ncpl, 1);
#ifdef EXT_INTR_STATS 
        ext_intr_statp = 0;
#endif
	if (imask_test_v(&ipending, ncplp))
		intr_dispatch();

	if (ncpl < IPL_HIGH)
		omsr |= PSL_EE;
	extintr_restore(omsr);

	return (ocpl);
}
#endif
