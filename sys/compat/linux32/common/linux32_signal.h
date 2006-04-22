/* $NetBSD: linux32_signal.h,v 1.1.10.2 2006/04/22 11:38:14 simonb Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _LINUX32_SIGNAL_H_
#define _LINUX32_SIGNAL_H_

#ifdef __amd64__
#include <compat/linux32/arch/amd64/linux32_signal.h>
#endif

void linux32_to_native_sigset(sigset_t *, const linux32_sigset_t *);
void native_to_linux32_sigset(linux32_sigset_t *, const sigset_t *);
int linux32_to_native_sigflags(const unsigned long);
unsigned int native_to_linux32_sigflags(const int);
void linux32_to_native_sigaction(struct sigaction *, 
	const struct linux32_sigaction *);
void native_to_linux32_sigaction(struct linux32_sigaction *, 
	const struct sigaction *);
void native_to_linux32_sigaltstack __P((struct linux32_sigaltstack *,
    const struct sigaltstack *));
void native_to_linux32_old_sigset(linux32_old_sigset_t *, const sigset_t *);
void linux32_old_extra_to_native_sigset(sigset_t *,
    const linux32_old_sigset_t *, const unsigned long *);
void linux32_old_to_native_sigset(sigset_t *, const linux32_old_sigset_t *);

#endif /* _LINUX32_SIGNAL_H_ */
