/*	$NetBSD: psl.h,v 1.1.8.3 2004/09/21 13:15:07 skrll Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#include <powerpc/psl.h>

#ifdef PPC_IBM4XX
/* 4xx don't have PSL_RI */
#undef PSL_USERSET
#ifdef PPC_IBM403
#define	PSL_USERSET	(PSL_EE | PSL_PR | PSL_ME | PSL_IR | PSL_DR)
#else
/* Apparently we get unexplained machine checks, so disable them. */
#define	PSL_USERSET	(PSL_EE | PSL_PR | PSL_IR | PSL_DR)
#endif

/* 
 * We also need to override the PSL_SE bit.  4xx have completely
 * different debug register support.
 *
 * The SE bit is actually the DWE bit.  We want to set the DE bit
 * to enable the debug regs instead of the DWE bit.
 */
#undef	PSL_SE
#define	PSL_SE	0x00000200
#endif
