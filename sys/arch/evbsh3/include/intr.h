/*	$NetBSD: intr.h,v 1.1 1999/09/13 10:30:34 itojun Exp $	*/
#ifndef _EVBSH3_INTR_H_
#define _EVBSH3_INTR_H_

#include <sh3/intr.h>

/* Soft interrupt masks. */
#define	SIR_CLOCK	31
#define	SIR_NET		30
#define	SIR_SERIAL	29

/* IRQ */
#define TMU1_IRQ	2
#define SCI_IRQ		6
#define SCIF_IRQ	7

#endif /* _EVBSH3_INTR_H_ */
