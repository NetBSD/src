/*	$NetBSD: intr.h,v 1.3.32.1 2005/04/29 11:28:25 kent Exp $	*/

/* These control the software interrupt register. */
extern void isr_soft_request(int);
extern void isr_soft_clear(int);

#include <sun68k/intr.h>
