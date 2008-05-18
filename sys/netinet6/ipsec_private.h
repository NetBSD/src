/*	$NetBSD: ipsec_private.h,v 1.2.4.2 2008/05/18 12:35:35 yamt Exp $	*/

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

#ifndef _NETINET6_IPSEC_PRIVATE_H_
#define _NETINET6_IPSEC_PRIVATE_H_

#ifdef _KERNEL
#include <net/net_stats.h>

extern	percpu_t *ipsecstat_percpu;
extern	percpu_t *ipsec6stat_percpu;

#define	IPSEC_STAT_GETREF()	_NET_STAT_GETREF(ipsecstat_percpu)
#define	IPSEC_STAT_PUTREF()	_NET_STAT_PUTREF(ipsecstat_percpu)
#define	IPSEC_STATINC(x)	_NET_STATINC(ipsecstat_percpu, x)

#define	IPSEC6_STAT_GETREF()	_NET_STAT_GETREF(ipsec6stat_percpu)
#define	IPSEC6_STAT_PUTREF()	_NET_STAT_PUTREF(ipsec6stat_percpu)
#define	IPSEC6_STATINC(x)	_NET_STATINC(ipsec6stat_percpu, x)
#endif /* _KERNEL */

#endif /* !_NETINET6_IPSEC_PRIVATE_H_ */
