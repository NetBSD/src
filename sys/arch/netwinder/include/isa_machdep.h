/*	$NetBSD: isa_machdep.h,v 1.4 2002/10/09 00:33:39 thorpej Exp $	*/

#ifndef _NETWINDER_ISA_MACHDEP_H_
#define	_NETWINDER_ISA_MACHDEP_H_

#include <arm/isa_machdep.h>

#ifdef _KERNEL
void	isa_netwinder_init(u_int, u_int);
#endif /* _KERNEL */

#endif /* _NETWINDER_ISA_MACHDEP_H_ */
