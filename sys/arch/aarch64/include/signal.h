/* $NetBSD: signal.h,v 1.3 2021/10/27 04:45:42 thorpej Exp $ */

#ifndef _AARCH64_SIGNAL_H_
#define	_AARCH64_SIGNAL_H_

#include <arm/signal.h>

#ifdef _KERNEL
/* This is needed to support COMPAT_NETBSD32. */ 
#define	__HAVE_STRUCT_SIGCONTEXT
#endif /* _KERNEL */

#endif /* ! _AARCH64_SIGNAL_H_ */
