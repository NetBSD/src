/*	$NetBSD: intr.h,v 1.4 2005/01/22 15:36:09 chs Exp $	*/

/* These control the software interrupt register. */
extern void isr_soft_request(int);
extern void isr_soft_clear(int);

#include <sun68k/intr.h>
