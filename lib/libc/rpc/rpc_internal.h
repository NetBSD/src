/*	$NetBSD: rpc_internal.h,v 1.4.24.1 2008/05/18 12:30:18 yamt Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden.
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
/*
 * Private include file for XDR functions only used internally in libc.
 * These are not exported interfaces.
 */

bool_t __xdrrec_getrec __P((XDR *, enum xprt_stat *, bool_t));
bool_t __xdrrec_setnonblock __P((XDR *, int));
void __xprt_unregister_unlocked __P((SVCXPRT *));
bool_t __svc_clean_idle __P((fd_set *, int, bool_t));

bool_t __xdrrec_getrec __P((XDR *, enum xprt_stat *, bool_t));
bool_t __xdrrec_setnonblock __P((XDR *, int));

u_int __rpc_get_a_size __P((int));
int __rpc_dtbsize __P((void));
struct netconfig * __rpcgettp __P((int));
int  __rpc_get_default_domain __P((char **));

char *__rpc_taddr2uaddr_af __P((int, const struct netbuf *));
struct netbuf *__rpc_uaddr2taddr_af __P((int, const char *));
int __rpc_fixup_addr __P((struct netbuf *, const struct netbuf *));
int __rpc_sockinfo2netid __P((struct __rpc_sockinfo *, const char **));
int __rpc_seman2socktype __P((int));
int __rpc_socktype2seman __P((int));
void *rpc_nullproc __P((CLIENT *));
int __rpc_sockisbound __P((int));

struct netbuf *__rpcb_findaddr __P((rpcprog_t, rpcvers_t,
				    const struct netconfig *,
				    const char *, CLIENT **));
bool_t __rpc_control __P((int,void *));

char *_get_next_token __P((char *, int));

u_int32_t __rpc_getxid __P((void));
#define __RPC_GETXID()	(__rpc_getxid())

extern SVCXPRT **__svc_xports;
extern int __svc_maxrec;
