/*	$NetBSD: intr.h,v 1.2 2000/03/21 02:27:50 soren Exp $	*/

#include <mips/cpuregs.h>

#define	IPL_NONE	0	/* Disable only this interrupt. */
#define	IPL_BIO		1	/* Disable block I/O interrupts. */
#define	IPL_NET		2	/* Disable network interrupts. */
#define	IPL_TTY		3	/* Disable terminal interrupts. */
#define	IPL_IMP		4	/* memory allocation */
#define	IPL_CLOCK	4	/* Disable clock interrupts. */
#define	IPL_STATCLOCK	5	/* Disable profiling interrupts. */
#define	IPL_HIGH	6	/* Disable all interrupts. */
#define NIPL		7

/* Interrupt sharing types. */
#define IST_NONE	0	/* none */
#define IST_PULSE	1	/* pulsed */
#define IST_EDGE	2	/* edge-triggered */
#define IST_LEVEL	3	/* level-triggered */

#define SIR_CLOCK	31
#define SIR_NET		30
#define SIR_CLOCKMASK	((1 << SIR_CLOCK))
#define SIR_NETMASK	((1 << SIR_NET) | SIR_CLOCKMASK)
#define SIR_ALLMASK	(SIR_CLOCKMASK | SIR_NETMASK)

#ifndef _LOCORE

int			imask[NIPL];

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
#define splimp()	_splraise(SPLTTY)
#define splclock()      _splraise(SPLCLOCK)
#define splstatclock()  splclock()
#define spllowersoftclock() _spllower(MIPS_SOFT_INT_MASK_0)
#define splsoftclock()	_splraise(MIPS_SOFT_INT_MASK_0)
#define splsoftnet()	_splraise(MIPS_SOFT_INT_MASK_1)

extern unsigned int	intrcnt[];
#define SOFTCLOCK_INTR	0
#define SOFTNET_INTR	1  
#define CLOCK_INTR	2  
#define FPU_INTR	3

/* Handle device interrupts. */
extern int		(*mips_hardware_intr)(unsigned int, unsigned int,
						unsigned int, unsigned int);

/* Handle software interrupts. */
extern void		(*mips_software_intr)(int);

#endif /* _LOCORE */
