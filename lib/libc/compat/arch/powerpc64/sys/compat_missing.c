/*	$NetBSD: compat_missing.c,v 1.2 2021/11/02 06:54:10 thorpej Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: compat_missing.c,v 1.2 2021/11/02 06:54:10 thorpej Exp $");

/*
 * define symbols that autoconf is supposed to find
 * without including the standard headers
 */
#define __LIBC12_SOURCE__
#include <signal.h>

__warn_references(sigaction,
    "warning: reference to compatibility sigaction(); include <signal.h> to generate correct reference")
__warn_references(sigpending,
    "warning: reference to compatibility sigpending(); include <signal.h> to generate correct reference")
__warn_references(sigprocmask,
    "warning: reference to compatibility sigprocmask(); include <signal.h> to generate correct reference")
__warn_references(sigsuspend,
    "warning: reference to compatibility sigsuspend(); include <signal.h> to generate correct reference")

int sigaction(int, const struct sigaction * restrict,
    struct sigaction * restrict);
int __sigaction_siginfo(int, const struct sigaction * restrict,
    struct sigaction * restrict);

int
sigaction(int sig, const struct sigaction * restrict act,
    struct sigaction * restrict oact)
{
	return __sigaction_siginfo(sig, act, oact);
}

int sigpending(sigset_t *);
int __sigpending14(sigset_t *);

int
sigpending(sigset_t *mask)
{
	return __sigpending14(mask);
}

int sigprocmask(int how, const sigset_t * restrict set,
    sigset_t * restrict oset);
int __sigprocmask14(int how, const sigset_t * restrict set,
    sigset_t * restrict oset);

int
sigprocmask(int how, const sigset_t * restrict set, sigset_t * restrict oset)
{
	return __sigprocmask14(how, set, oset);
}

int sigsuspend(const sigset_t *);
int __sigsuspend14(const sigset_t *);

int
sigsuspend(const sigset_t *mask)
{
	return __sigsuspend14(mask);
}
