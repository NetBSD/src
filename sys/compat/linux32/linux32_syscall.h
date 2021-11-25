/*	$NetBSD: linux32_syscall.h,v 1.2 2021/11/25 03:08:05 ryo Exp $ */

#ifndef _LINUX32_SYSCALL_H
#define _LINUX32_SYSCALL_H

#if defined(__aarch64__)
#include <compat/linux32/arch/aarch64/linux32_syscall.h>
#elif defined(__amd64__)
#include <compat/linux32/arch/amd64/linux32_syscall.h>
#else
#define	LINUX32_SYS_MAXSYSCALL	0
#endif

#endif /* !_LINUX32_SYSCALL_H */
