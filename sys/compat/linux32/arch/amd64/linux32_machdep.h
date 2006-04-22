/*	$NetBSD: linux32_machdep.h,v 1.1.10.2 2006/04/22 11:38:14 simonb Exp $ */

#ifndef _AMD64_LINUX32_MACHDEP_H
#define _AMD64_LINUX32_MACHDEP_H

#include <compat/linux32/arch/amd64/linux32_missing.h>

#ifdef _KERNEL
__BEGIN_DECLS
void linux32_syscall_intern __P((struct proc *));
__END_DECLS
#endif /* !_KERNEL */

#endif /* _AMD64_LINUX32_MACHDEP_H */
