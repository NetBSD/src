/*	$NetBSD: gemini_intr.h,v 1.1 2008/10/24 04:23:18 matt Exp $	*/

#ifndef _ARM_GEMINI_INTR_H_
#define _ARM_GEMINI_INTR_H_

#define	ARM_IRQ_HANDLER	_C_LABEL(gemini_irq_handler)

#ifndef _LOCORE

extern void gemini_splx(int);
extern int gemini_splraise(int);
extern int gemini_spllower(int);

int	_splraise(int);
int	_spllower(int);
void	splx(int);
void	gemini_irq_handler(void *);

#if !defined(EVBARM_SPL_NOINLINE)
#define splx(new)		gemini_splx(new)
#define _spllower(ipl)		gemini_spllower(ipl)
#define _splraise(ipl)		gemini_splraise(ipl)
#ifdef __HAVE_FAST_SOFTINTS
# define _setsoftintr(si)	omap_setsoftintr(si)
#endif
#endif  /* !EVBARM_SPL_NOINTR */



#endif	/* _LOCORE */

#endif	/* _ARM_GEMINI_INTR_H_ */
