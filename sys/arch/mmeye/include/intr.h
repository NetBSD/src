/*	$NetBSD: intr.h,v 1.1 1999/09/13 10:30:56 itojun Exp $	*/

#include <sh3/intr.h>

/* Soft interrupt masks. */
#define	SIR_CLOCK	31
#define	SIR_NET		30
#define	SIR_SERIAL	29

/* IRQ */

#define TMU1_IRQ 13
