/*	$NetBSD: intr.h,v 1.5 2005/12/11 12:19:16 christos Exp $	*/

/* These control the software interrupt register. */
extern void isr_soft_request(int);
extern void isr_soft_clear(int);

#include <sun68k/intr.h>
