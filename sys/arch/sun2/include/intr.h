/*	$NetBSD: intr.h,v 1.3.24.1 2005/01/24 08:34:34 skrll Exp $	*/

/* These control the software interrupt register. */
extern void isr_soft_request(int);
extern void isr_soft_clear(int);

#include <sun68k/intr.h>
