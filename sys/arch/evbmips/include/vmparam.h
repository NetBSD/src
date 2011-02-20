/*	$NetBSD: vmparam.h,v 1.3 2011/02/20 07:48:34 matt Exp $	*/

#ifndef _EVBMIPS_VMPARAM_H_
#define _EVBMIPS_VMPARAM_H_

#include <mips/vmparam.h>

#define	VM_PHYSSEG_MAX		32

#undef VM_FREELIST_MAX
#define	VM_FREELIST_MAX		3
#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
#define	VM_FREELIST_FIRST4G	2
#endif
#if !defined(_LP64)
#define	VM_FREELIST_FIRST512M	1
#endif
 
#endif	/* !_EVBMIPS_VMPARAM_H_ */
