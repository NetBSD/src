/* 	$NetBSD: linux_signal.h,v 1.13.6.1 2001/10/01 12:43:48 fvdl Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LINUX_SIGNAL_H
#define _LINUX_SIGNAL_H

#if defined(__i386__)
#include <compat/linux/arch/i386/linux_signal.h>
#elif defined(__m68k__)
#include <compat/linux/arch/m68k/linux_signal.h>
#elif defined(__alpha__)
#include <compat/linux/arch/alpha/linux_signal.h>
#elif defined(__powerpc__)
#include <compat/linux/arch/powerpc/linux_signal.h>
#elif defined(__mips__)
#include <compat/linux/arch/mips/linux_signal.h>
#else
#error Undefined linux_signal.h machine type.
#endif

#ifdef _KERNEL
extern const int native_to_linux_sig[];
extern const int linux_to_native_sig[];
__BEGIN_DECLS
int linux_sigprocmask1 __P((struct proc *, int, const linux_old_sigset_t *,
						linux_old_sigset_t *));

#if LINUX__NSIG_WORDS > 1
void linux_old_extra_to_native_sigset __P((const linux_old_sigset_t *,
    const unsigned long *, sigset_t *));
void native_to_linux_old_extra_sigset __P((const sigset_t *,
    linux_old_sigset_t *, unsigned long *));
#define linux_old_to_native_sigset(x,y) \
    linux_old_extra_to_native_sigset(x, (const unsigned long *) 0, y)
#define native_to_linux_old_sigset(x,y) \
    native_to_linux_old_extra_sigset(x, y, (unsigned long *) 0)

#else	/* LINUX__NSIG_WORDS == 1 */

#define linux_old_to_native_sigset(x,y) \
    linux_to_native_sigset((const linux_sigset_t *) x, y)
#define native_to_linux_old_sigset(x,y) \
    native_to_linux_sigset(x, (linux_sigset_t *) y)
#endif

void linux_to_native_sigset __P((const linux_sigset_t *, sigset_t *));
void native_to_linux_sigset __P((const sigset_t *, linux_sigset_t *));

void linux_old_to_native_sigaction __P((struct linux_old_sigaction *,
    struct sigaction *));
void native_to_linux_old_sigaction __P((struct sigaction *,
    struct linux_old_sigaction *));

void linux_to_native_sigaction __P((struct linux_sigaction *,
    struct sigaction *));
void native_to_linux_sigaction __P((struct sigaction *,
    struct linux_sigaction *));

__END_DECLS
#endif /* !_KERNEL */

#endif /* !_LINUX_SIGNAL_H */
