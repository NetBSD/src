/*	$NetBSD: linux32_syscalls.c,v 1.2 2021/11/25 03:08:05 ryo Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: linux32_syscalls.c,v 1.2 2021/11/25 03:08:05 ryo Exp $");

#if defined(__aarch64__)
#include "../../sys/compat/linux32/arch/aarch64/linux32_syscalls.c"
#elif defined(__amd64__)
#include "../../sys/compat/linux32/arch/amd64/linux32_syscalls.c"
#else
const char * const linux32_syscallnames[] = { 0 };
#endif
