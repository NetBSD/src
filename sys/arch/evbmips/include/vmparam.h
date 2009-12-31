/*	$NetBSD: vmparam.h,v 1.1.142.3 2009/12/31 00:54:09 matt Exp $	*/

#ifndef _EVBMIPS_VMPARAM_H_
#define _EVBMIPS_VMPARAM_H_

#include <mips/vmparam.h>

#define	VM_PHYSSEG_MAX		32

#undef VMFREELIST_MAX
#define	VMFREELIST_MAX		3
#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
#define	VMFREELIST_FIRST4G	2
#endif
#if !defined(_LP64)
#define	VMFREELIST_FIRST512M	1
#endif
 
#endif	/* !_EVBMIPS_VMPARAM_H_ */
