/*	$NetBSD: linux32_errno.h,v 1.2 2021/11/25 03:08:04 ryo Exp $ */

#ifndef _LINUX32_ERRNO_H
#define _LINUX32_ERRNO_H

#if defined(__aarch64__)
#include <compat/linux32/arch/aarch64/linux32_errno.h>
#elif defined(__amd64__)
#include <compat/linux32/arch/amd64/linux32_errno.h>
#else
#error Undefined linux32_errno.h machine type.
#endif

#endif /* !_LINUX32_ERRNO_H */
