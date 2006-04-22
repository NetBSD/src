/*	$NetBSD: intrdefs.h,v 1.4.6.1 2006/04/22 11:38:11 simonb Exp $	*/
/*	NetBSD intrdefs.h,v 1.3 2003/06/16 20:01:06 thorpej Exp 	*/

#ifndef _XEN_INTRDEFS_H
#define _XEN_INTRDEFS_H

/*
 * Interrupt priority levels.
 * 
 * There are tty, network and disk drivers that use free() at interrupt
 * time, so imp > (tty | net | bio).
 *
 * Since run queues may be manipulated by both the statclock and tty,
 * network, and disk drivers, clock > imp.
 *
 * IPL_HIGH must block everything that can manipulate a run queue.
 *
 * We need serial drivers to run at the absolute highest priority to
 * avoid overruns, so serial > high.
 * These are also used as index in the struct iplsource[] array, so each
 * soft interrupt needs its own IPL level, not shared with anything else.
 *
 */ 

#define	IPL_NONE	0x0	/* nothing */
#define	IPL_SOFTCLOCK	0x1	/* timeouts */
#define	IPL_SOFTNET	0x2	/* protocol stacks */
#define	IPL_SOFTXENEVT	0x3	/* /dev/xenevt */
#define	IPL_BIO		0x4	/* block I/O */
#define	IPL_NET		0x5	/* network */
#define	IPL_SOFTSERIAL	0x6	/* serial */
#define	IPL_CTRL	0x7	/* control events */
#define	IPL_TTY		0x8	/* terminal */
#define	IPL_LPT		IPL_TTY
#define	IPL_VM		0x9	/* memory allocation */
#define	IPL_AUDIO	0xa	/* audio */
#define	IPL_CLOCK	0xb	/* clock */
#define	IPL_STATCLOCK	IPL_CLOCK
#define	IPL_SCHED	IPL_CLOCK
#define	IPL_HIGH	0xc	/* everything */
#define	IPL_LOCK	IPL_HIGH
#define	IPL_SERIAL	0xc	/* serial */
#define	IPL_IPI		0xd	/* inter-processor interrupts */
#define	IPL_DEBUG	0xe	/* debug events */
#define	IPL_DIE		0xf	/* die events */
#define	NIPL		16

/* Soft interrupt masks. */
#define	SIR_CLOCK	IPL_SOFTCLOCK
#define	SIR_NET		IPL_SOFTNET
#define	SIR_SERIAL	IPL_SOFTSERIAL
#define	SIR_XENEVT	IPL_SOFTXENEVT

#define	IREENT_MAGIC	0x18041969

/* Interrupt sharing types (for ISA) */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */ 

#define NUM_LEGACY_IRQS	16

#endif /* _XEN_INTRDEFS_H */
