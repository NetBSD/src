/*	$NetBSD: linux32_syscallargs.h,v 1.2 2021/11/25 03:08:05 ryo Exp $ */

#ifndef _LINUX32_SYSCALLARGS_H
#define _LINUX32_SYSCALLARGS_H

#if defined(__aarch64__)
#include <compat/linux32/arch/aarch64/linux32_syscallargs.h>
#elif defined(__amd64__)
#include <compat/linux32/arch/amd64/linux32_syscallargs.h>
#else
#error Undefined linux32_syscallargs.h machine type.
#endif

#endif /* !_LINUX32_SYSCALLARGS_H */
