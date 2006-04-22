/*	$NetBSD: linux32_syscalls.c,v 1.1.10.2 2006/04/22 11:38:14 simonb Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: linux32_syscalls.c,v 1.1.10.2 2006/04/22 11:38:14 simonb Exp $");

#if defined(__amd64__)
#include "../../sys/compat/linux32/arch/amd64/linux32_syscalls.c"
#else
const char * const linux32_syscallnames[] = { 0 };
#endif
