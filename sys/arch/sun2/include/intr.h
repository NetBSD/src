/*	$NetBSD: intr.h,v 1.3 2001/06/14 13:12:50 fredette Exp $	*/

/* These control the software interrupt register. */
extern void isr_soft_request __P((int level));
extern void isr_soft_clear __P((int level));

#include <sun68k/intr.h>
