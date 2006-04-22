/*	$NetBSD: spl.h,v 1.3.6.1 2006/04/22 11:40:19 simonb Exp $	*/

/*-
 * Copyright (c)2005 YAMAMOTO Takashi,
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * this header is intended to be included by MD header.
 */

#if !defined(_KERNEL)
#error not supposed to be exposed to userland.
#endif /* !defined(_KERNEL) */

#define	_SPL_DECL(x, X)	\
	static __inline int spl##x(void) { return splraiseipl(IPL_##X); }

#if defined(IPL_SOFTCLOCK)
_SPL_DECL(softclock, SOFTCLOCK)
#endif /* defined(IPL_SOFTCLOCK) */
#if defined(IPL_SOFTNET)
_SPL_DECL(softnet, SOFTNET)
#endif /* defined(IPL_SOFTNET) */
#if defined(IPL_SOFTSERIAL)
_SPL_DECL(softserial, SOFTSERIAL)
#endif /* defined(IPL_SOFTSERIAL) */

_SPL_DECL(bio, BIO)
_SPL_DECL(net, NET)
_SPL_DECL(tty, TTY)
_SPL_DECL(vm, VM)
_SPL_DECL(statclock, STATCLOCK)
_SPL_DECL(clock, CLOCK)
_SPL_DECL(sched, SCHED)
_SPL_DECL(lock, LOCK)
_SPL_DECL(high, HIGH)

#if defined(IPL_SERIAL)
_SPL_DECL(serial, SERIAL)
#endif /* defined(IPL_SERIAL) */
#if defined(IPL_AUDIO)
_SPL_DECL(audio, AUDIO)
#endif /* defined(IPL_AUDIO) */
#if defined(IPL_LPT)
_SPL_DECL(lpt, LPT)
#endif /* defined(IPL_LPT) */

#if defined(IPL_IPI)
_SPL_DECL(ipi, IPI)
#endif /* defined(IPL_IPI) */

#undef _SPL_DECL
