/*	bat.h,v 1.5 2003/02/03 17:10:01 matt Exp	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
#include <powerpc/oea/bat.h>
#endif
