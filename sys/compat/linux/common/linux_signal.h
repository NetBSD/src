/* 	$NetBSD: linux_signal.h,v 1.30 2009/05/29 14:19:13 njoly Exp $	*/

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
#elif defined(__arm__)
#include <compat/linux/arch/arm/linux_signal.h>
#elif defined(__amd64__)
#include <compat/linux/arch/amd64/linux_signal.h>
#endif

#ifdef _KERNEL
extern const int native_to_linux_signo[];
extern const int linux_to_native_signo[];
__BEGIN_DECLS
int linux_sigprocmask1(struct lwp *, int, const linux_old_sigset_t *,
						linux_old_sigset_t *);

#if LINUX__NSIG_WORDS > 1
void linux_old_extra_to_native_sigset(sigset_t *,
    const linux_old_sigset_t *, const unsigned long *);
void native_to_linux_old_extra_sigset(linux_old_sigset_t *,
    unsigned long *, const sigset_t *);
#define linux_old_to_native_sigset(x,y) \
    linux_old_extra_to_native_sigset(x, y, (const unsigned long *)0)
#define native_to_linux_old_sigset(x,y) \
    native_to_linux_old_extra_sigset(x, (unsigned long *)0, y)

#else	/* LINUX__NSIG_WORDS == 1 */

/* XXXmanu (const linux_sigset_t *)(void *) temporary hack to get it building */
#define linux_old_to_native_sigset(x,y) \
    linux_to_native_sigset(x, (const linux_sigset_t *)(const void *)y)
#define native_to_linux_old_sigset(x,y) \
    native_to_linux_sigset((linux_sigset_t *)(void *)x, y)
#endif

void linux_to_native_sigset(sigset_t *, const linux_sigset_t *);
void native_to_linux_sigset(linux_sigset_t *, const sigset_t *);
int linux_to_native_sigflags(const unsigned long);
unsigned int native_to_linux_sigflags(const int);

void linux_old_to_native_sigaction(struct sigaction *,
    const struct linux_old_sigaction *);
void native_to_linux_old_sigaction(struct linux_old_sigaction *,
    const struct sigaction *);

void linux_to_native_sigaction(struct sigaction *,
    const struct linux_sigaction *);
void native_to_linux_sigaction(struct linux_sigaction *,
    const struct sigaction *);

void native_to_linux_sigaltstack(struct linux_sigaltstack *,
    const struct sigaltstack *);

int native_to_linux_si_code(int);
int native_to_linux_si_status(int, int);

__END_DECLS
#endif /* !_KERNEL */

#endif /* !_LINUX_SIGNAL_H */
