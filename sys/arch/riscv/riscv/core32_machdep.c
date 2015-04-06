/*	$NetBSD: core32_machdep.c,v 1.1.2.2 2015/04/06 15:18:01 skrll Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: core32_machdep.c,v 1.1.2.2 2015/04/06 15:18:01 skrll Exp $");

#define	CORENAME(x)	__CONCAT(x,32)
#define	COREINC		<compat/netbsd32/netbsd32.h>

#include "core_machdep.c"
