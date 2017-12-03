/*	$NetBSD: core32_machdep.c,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: core32_machdep.c,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $");

#define	CORENAME(x)	__CONCAT(x,32)
#define	COREINC		<compat/netbsd32/netbsd32.h>

#include "core_machdep.c"
