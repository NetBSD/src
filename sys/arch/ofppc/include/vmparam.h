/*	$NetBSD: vmparam.h,v 1.22.50.1 2011/06/23 14:19:26 cherry Exp $	*/

/*
 * These are based on the segments that are least commonly used, and when
 * used, they are less often used as primary PCI's, or PCI's with video or
 * AGP.  There is no unused value for these, as every different machine
 * uses all, or some of the segments after 0x8.
 */

#if !defined(_MODULE)
#define KERNEL_SR	0xa
#define KERNEL2_SR	0xb
#define USER_SR		0xe
#endif

#include <powerpc/vmparam.h>
