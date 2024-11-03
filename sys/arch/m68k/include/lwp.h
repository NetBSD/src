/*	$NetBSD: lwp.h,v 1.2 2024/11/03 22:24:22 christos Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
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

#ifndef _M68K_LWP_H_
#define _M68K_LWP_H_

#define	TLS_TP_OFFSET	0x7000
#define	TLS_DTV_OFFSET	0x8000

#include <sys/tls.h>

__CTASSERT(TLS_TP_OFFSET + sizeof(struct tls_tcb) < 0x8000);
__CTASSERT(TLS_TP_OFFSET % sizeof(struct tls_tcb) == 0);

__BEGIN_DECLS

static __inline struct tls_tcb *
__lwp_gettcb_fast(void)
{
	unsigned int __tcb = (unsigned int)_lwp_getprivate();
	return (struct tls_tcb *)(uintptr_t)
	    (__tcb - TLS_TP_OFFSET - sizeof(struct tls_tcb));
}

static inline void
__lwp_settcb(struct tls_tcb *__tcb)
{
	__tcb += TLS_TP_OFFSET / sizeof(*__tcb) + 1;
	_lwp_setprivate(__tcb);
}
__END_DECLS

#endif	/* !_M68K_LWP_H_ */
