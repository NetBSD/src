/*	$NetBSD: core32_machdep.c,v 1.1 2015/03/28 16:13:56 matt Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: core32_machdep.c,v 1.1 2015/03/28 16:13:56 matt Exp $");

#define	CORENAME(x)	__CONCAT(x,32)
#define	COREINC		<compat/netbsd32/netbsd32.h>

#include "core_machdep.c"
