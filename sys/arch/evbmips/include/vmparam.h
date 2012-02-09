/*	vmparam.h,v 1.1.142.5 2011/11/29 07:48:32 matt Exp	*/

#ifndef _EVBMIPS_VMPARAM_H_
#define _EVBMIPS_VMPARAM_H_

#include <mips/vmparam.h>

#define	VM_PHYSSEG_MAX			32

#undef VM_NFREELIST
#define	VM_NFREELIST			3
#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
#define	VM_FREELIST_FIRST4G		2
#define	VM_FREELIST_FIRST4G_P(pa)	(((pa) >> 32) == 0)
#endif
#if !defined(_LP64)
#define	VM_FREELIST_FIRST512M		1
#define	VM_FREELIST_FIRST512M_P(pa)	(((pa) >> 29) == 0)
#endif

#define	VM_FREELIST_NORMALOK_P(lcv) \
	((lcv) == VM_FREELIST_DEFAULT || (lcv) != mips_poolpage_vmfreelist)
 
#endif	/* !_EVBMIPS_VMPARAM_H_ */
