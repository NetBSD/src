/*	$NetBSD: ipsecif.h,v 1.1.2.2 2018/02/11 21:17:34 snj Exp $  */

/*
 * Copyright (c) 2017 Internet Initiative Japan Inc.
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

#ifndef _NETIPSEC_IPSECIF_H_
#define _NETIPSEC_IPSECIF_H_

#include <net/if_ipsec.h>

#define IPSEC_TTL	64
#define IPSEC_HLIM	64

#ifdef _KERNEL
int ipsecif4_encap_func(struct mbuf *, struct ip *, struct ipsec_variant *);
int ipsecif4_attach(struct ipsec_variant *);
int ipsecif4_detach(struct ipsec_variant *);

int ipsecif6_attach(struct ipsec_variant *);
int ipsecif6_detach(struct ipsec_variant *);
void *ipsecif6_ctlinput(int, const struct sockaddr *, void *, void *);
#endif

#endif /*_NETIPSEC_IPSECIF_H_*/
