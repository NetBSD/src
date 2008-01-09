/*	$NetBSD: vmparam.h,v 1.21.80.1 2008/01/09 01:47:37 matt Exp $	*/

/*
 * These are based on the segments that are least commonly used, and when
 * used, they are less often used as primary PCI's, or PCI's with video or
 * AGP.  There is no unused value for these, as every different machine
 * uses all, or some of the segments after 0x8.
 */

#define USER_SR		0xe
#define KERNEL_SR	0xa
#define KERNEL2_SR	0xb

#include <powerpc/oea/vmparam.h>
