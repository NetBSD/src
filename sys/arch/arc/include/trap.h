/*	$NetBSD: trap.h,v 1.4.38.1 2005/04/29 11:28:01 kent Exp $	*/
/*      $OpenBSD: trap.h,v 1.1.1.1 1996/06/24 09:07:18 pefo Exp $	*/

#include <mips/trap.h>

#ifdef _KERNEL
extern int arc_hardware_intr (unsigned mask, unsigned pc,
		unsigned statusReg, unsigned causeReg);
#endif /* _KERNEL */
