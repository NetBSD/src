/*	$NetBSD: isa_machdep.h,v 1.5 2002/10/12 11:53:42 chris Exp $	*/

#ifndef _NETWINDER_ISA_MACHDEP_H_
#define	_NETWINDER_ISA_MACHDEP_H_

#include <arm/isa_machdep.h>

#ifdef _KERNEL
#define ISA_FOOTBRIDGE_IRQ IRQ_IN_L3
void	isa_footbridge_init(u_int, u_int);
#endif /* _KERNEL */

#endif /* _NETWINDER_ISA_MACHDEP_H_ */
