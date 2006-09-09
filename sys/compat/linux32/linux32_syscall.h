/*	$NetBSD: linux32_syscall.h,v 1.1.22.2 2006/09/09 02:45:52 rpaulo Exp $ */

#ifndef _LINUX32_SYSCALL_H
#define _LINUX32_SYSCALL_H

#if defined(__amd64__)
#include <compat/linux32/arch/amd64/linux32_syscall.h>
#else
#define	LINUX32_SYS_MAXSYSCALL	0
#endif

#endif /* !_LINUX32_SYSCALL_H */
