/*	$NetBSD: psl.h,v 1.1 2001/06/13 06:01:58 simonb Exp $	*/

#include <powerpc/psl.h>

/* Apparently we get unexplained machine checks, so disable them. */
#undef PSL_USERSET
#define	PSL_USERSET	(PSL_EE | PSL_PR | PSL_IR | PSL_DR | PSL_RI)

/* 
 * We also need to override the PSL_SE bit.  4xx have completely
 * different debug register support.
 *
 * The SE bit is actually the DWE bit.  We want to set the DE bit
 * to enable the debug regs instead of the DWE bit.
 */
#undef	PSL_SE
#define	PSL_SE	0x00000200
