/*	$NetBSD: tcp_private.h,v 1.1 2008/04/12 05:58:22 thorpej Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _NETINET_TCP_PRIVATE_H_
#define _NETINET_TCP_PRIVATE_H_

#ifdef _KERNEL
#include <sys/percpu.h>

extern	percpu_t *tcpstat_percpu;

#define	TCP_STAT_GETREF()	percpu_getref(tcpstat_percpu)
#define	TCP_STAT_PUTREF()	percpu_putref(tcpstat_percpu)

#define	TCP_STATINC(x)							\
do {									\
	uint64_t *_tcps_ = TCP_STAT_GETREF();				\
	_tcps_[x]++;							\
	TCP_STAT_PUTREF();						\
} while (/*CONSTCOND*/0)

#define	TCP_STATADD(x, v)						\
do {									\
	uint64_t *_tcps_ = TCP_STAT_GETREF();				\
	_tcps_[x] += (v);						\
	TCP_STAT_PUTREF();						\
} while (/*CONSTCOND*/0)

#ifdef __NO_STRICT_ALIGNMENT
#define	TCP_HDR_ALIGNED_P(th)	1
#else
#define	TCP_HDR_ALIGNED_P(th)	((((vaddr_t)(th)) & 3) == 0)
#endif /* __NO_STRICT_ALIGNMENT */
#endif /* _KERNEL */

#endif /* !_NETINET_TCP_PRIVATE_H_ */
