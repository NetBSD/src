/*	$NetBSD: intrdefs.h,v 1.1.2.2 2002/12/11 06:01:01 thorpej Exp $	*/

#ifndef _i386_INTRDEFS_H
#define _i386_INTRDEFS_H

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
 *
 * The level numbers are picked to fit into APIC vector priorities.
 *
 */
#define	IPL_NONE	0x0	/* nothing */
#define	IPL_SOFTCLOCK	0x4	/* timeouts */
#define	IPL_SOFTNET	0x5	/* protocol stacks */
#define	IPL_BIO		0x6	/* block I/O */
#define	IPL_NET		0x7	/* network */
#define	IPL_SOFTSERIAL	0x8	/* serial */
#define	IPL_TTY		0x9	/* terminal */
#define	IPL_VM		0xa	/* memory allocation */
#define IPL_IMP		IPL_VM
#define	IPL_AUDIO	0xb	/* audio */
#define	IPL_CLOCK	0xc	/* clock */
#define IPL_SCHED	IPL_CLOCK
#define	IPL_HIGH	0xd	/* everything */
#define	IPL_SERIAL	0xd	/* serial */
#define IPL_IPI		0xe	/* inter-processor interrupts */
#define	NIPL		16

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

/* Soft interrupt masks. */
#define	SIR_CLOCK	31
#define	SIR_NET	30
#define	SIR_SERIAL	29

/*
 * Local APIC masks. Must not conflict with SIR_* above, and must
 * be >= NUM_LEGACY_IRQs.
 */
#define LIR_TIMER	28
#define LIR_IPI		27

/*
 * Maximum # of interrupt sources per CPU. 32 to fit in one word.
 * ioapics can theoretically produce more, but it's not likely to
 * happen. For multiple ioapics, things can be routed to different
 * CPUs.
 */
#define MAX_INTR_SOURCES	32
#define NUM_LEGACY_IRQS		16

/*
 * Low and high boundaries between which interrupt gates will
 * be allocated in the IDT.
 */
#define IDT_INTR_LOW	(0x20 + NUM_LEGACY_IRQS)
#define IDT_INTR_HIGH	0xef

#define I386_IPI_HALT			0x00000001
#define I386_IPI_MICROSET		0x00000002
#define I386_IPI_FLUSH_FPU		0x00000004
#define I386_IPI_SYNCH_FPU		0x00000008
#define I386_IPI_TLB			0x00000010
#define I386_IPI_MTRR			0x00000020
#define I386_IPI_GDT			0x00000040

#define I386_NIPI		7

#define I386_IPI_NAMES { "halt IPI", "timeset IPI", "FPU flush IPI", \
			 "FPU synch IPI", "TLB shootdown IPI", \
			 "MTRR update IPI", "GDT update IPI" }

#endif /* _I386_INTRDEFS_H */
