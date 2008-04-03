/*	$NetBSD: intr.h,v 1.3.44.1 2008/04/03 12:42:19 mjf Exp $	*/

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

#endif /* ! _IA64_INTR_H_ */
