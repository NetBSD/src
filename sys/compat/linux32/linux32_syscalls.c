/*	$NetBSD: linux32_syscalls.c,v 1.1 2006/02/09 19:18:57 manu Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: linux32_syscalls.c,v 1.1 2006/02/09 19:18:57 manu Exp $");

#if defined(__amd64__)
#include "../../sys/compat/linux32/arch/amd64/linux32_syscalls.c"
#else
const char * const linux32_syscallnames[] = { 0 };
#endif
