/* $NetBSD: ucontext.h,v 1.2 2018/08/05 18:42:48 reinoud Exp $ */

#ifndef _USERMODE_UCONTEXT_H
#define _USERMODE_UCONTEXT_H

#include <sys/ucontext.h>
#include <machine/trap.h>
#include <machine/psl.h>

#if defined(__i386__)

#define _UC_MACHINE_EFLAGS(uc) ((uc)->uc_mcontext.__gregs[_REG_EFL])

#elif defined(__x86_64__)

#define _UC_MACHINE_RFLAGS(uc) ((uc)->uc_mcontext.__gregs[26])

#elif defined(__arm__)
#error port me
#else
#error port me
#endif

#endif /* _USERMODE_UCONTEXT_H */

