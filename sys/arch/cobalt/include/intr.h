/*	$NetBSD: intr.h,v 1.4 2000/04/03 11:44:21 soda Exp $	*/

#include <mips/cpuregs.h>
#include <mips/intr.h>

#define	IPL_NONE	0	/* Disable only this interrupt. */
#define	IPL_BIO		1	/* Disable block I/O interrupts. */
#define	IPL_NET		2	/* Disable network interrupts. */
#define	IPL_TTY		3	/* Disable terminal interrupts. */
#define	IPL_IMP		4	/* Memory allocation */
#define	IPL_CLOCK	5	/* Disable clock interrupts. */
#define	IPL_STATCLOCK	6	/* Disable profiling interrupts. */
#define	IPL_HIGH	7	/* Disable all interrupts. */
#define NIPL		8

/* Interrupt sharing types. */
#define IST_NONE	0	/* none */
#define IST_PULSE	1	/* pulsed */
#define IST_EDGE	2	/* edge-triggered */
#define IST_LEVEL	3	/* level-triggered */

#ifdef _KERNEL
#ifndef _LOCORE

extern int		_splraise(int);
extern int		_spllower(int);      
extern int		_splset(int);
extern int		_splget(void);
extern void		_splnone(void);
extern void		_setsoftintr(int);
extern void		_clrsoftintr(int);

#define setsoftclock()	_setsoftintr(MIPS_SOFT_INT_MASK_0)
#define setsoftnet()	_setsoftintr(MIPS_SOFT_INT_MASK_1)
#define clearsoftclock() _clrsoftintr(MIPS_SOFT_INT_MASK_0)
#define clearsoftnet()	_clrsoftintr(MIPS_SOFT_INT_MASK_1)

#define splhigh()       _splraise(MIPS_INT_MASK)
#define spl0()          (void)_spllower(0)
#define splx(s)         (void)_splset(s)
#define SPLSOFT		MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1
#define SPLBIO		SPLSOFT | MIPS_INT_MASK_4
#define SPLNET		SPLBIO | MIPS_INT_MASK_1 | MIPS_INT_MASK_2
#define SPLTTY		SPLNET | MIPS_INT_MASK_3
#define SPLCLOCK	SPLTTY | MIPS_INT_MASK_0 | MIPS_INT_MASK_5
#define splbio()        _splraise(SPLBIO)
#define splnet()        _splraise(SPLNET)
#define spltty()        _splraise(SPLTTY)
#define splclock()      _splraise(SPLCLOCK)
#define splimp()	splclock()
#define splstatclock()  splclock()
#define splsoftclock()	_splraise(MIPS_SOFT_INT_MASK_0)
#define splsoftnet()	_splraise(MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1)
#define spllowersoftclock() _spllower(MIPS_SOFT_INT_MASK_0)

extern unsigned int	intrcnt[];
#define SOFTCLOCK_INTR	0
#define SOFTNET_INTR	1  

extern void *	cpu_intr_establish(int, int, int (*)(void *), void *);
extern void *	icu_intr_establish(int, int, int, int (*)(void *), void *);

#endif /* !_LOCORE */
#endif /* _LOCORE */
