/*	$NetBSD: intr.h,v 1.5 2009/07/20 06:14:15 kiyohara Exp $	*/

/* XXX: cherry: To Be fixed when we switch on interrupts. */

#ifndef _IA64_INTR_H_
#define _IA64_INTR_H_

#include <machine/intrdefs.h>

static __inline int splraise(int dummy) { return 0; }
static __inline void spllower(int dummy) { }

/*
 * Miscellaneous
 */
#define	splvm()		splraise(IPL_VM)
#define	splhigh()	splraise(IPL_HIGH)
#define	spl0()		spllower(IPL_NONE)
#define	splsched()	splraise(IPL_SCHED)
#define spllock() 	splhigh()
#define	splx(x)		spllower(x)

/*
 * Software interrupt masks
 */

#define	splsoftclock() splraise(IPL_SOFTCLOCK)
#define	splsoftnet()	splraise(IPL_SOFTNET)
#define	splsoftserial()	splraise(IPL_SOFTSERIAL)

typedef int ipl_t;
typedef struct {
	ipl_t _ipl;
} ipl_cookie_t;

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._ipl = ipl};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return splraise(icookie._ipl);
}


/*
 * Layout of the Processor Interrupt Block.
 */
struct ia64_interrupt_block
{
	uint64_t ib_ipi[0x20000];	/* 1Mb of IPI interrupts */
	uint8_t ib_reserved1[0xe0000];
	uint8_t ib_inta;		/* Generate INTA cycle */
	uint8_t ib_reserved2[7];
	uint8_t ib_xtp;			/* XTP cycle */
	uint8_t ib_reserved3[7];
	uint8_t ib_reserved4[0x1fff0];
};

extern uint64_t ia64_lapic_address;

#define IA64_INTERRUPT_BLOCK \
	(struct ia64_interrupt_block *)IA64_PHYS_TO_RR6(ia64_lapic_address)

void *intr_establish(int, int, int, int (*)(void *), void *);
void intr_disestablish(void *);

#endif /* ! _IA64_INTR_H_ */
