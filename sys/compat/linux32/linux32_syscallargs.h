/*	$NetBSD: linux32_syscallargs.h,v 1.1.22.2 2006/09/09 02:45:52 rpaulo Exp $ */

#ifndef _LINUX32_SYSCALLARGS_H
#define _LINUX32_SYSCALLARGS_H

#if defined(__amd64__)
#include <compat/linux32/arch/amd64/linux32_syscallargs.h>
#else
#error Undefined linux32_syscallargs.h machine type.
#endif

#endif /* !_LINUX32_SYSCALLARGS_H */
