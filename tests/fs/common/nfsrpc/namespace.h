/*	$NetBSD: namespace.h,v 1.2 2010/09/23 09:38:14 he Exp $	*/

/*-
 * Copyright (c) 1997-2004 The NetBSD Foundation, Inc.
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

#ifndef _NAMESPACE_H_
#define _NAMESPACE_H_

#include <sys/cdefs.h>

/* rpc locks */
#define authdes_lock		__rpc_authdes_lock
#define authnone_lock		__rpc_authnone_lock
#define authsvc_lock		__rpc_authsvc_lock
#define clnt_fd_lock		__rpc_clnt_fd_lock
#define clntraw_lock		__rpc_clntraw_lock
#define dname_lock		__rpc_dname_lock
#define dupreq_lock		__rpc_dupreq_lock
#define keyserv_lock		__rpc_keyserv_lock
#define libnsl_trace_lock	__rpc_libnsl_trace_lock
#define loopnconf_lock		__rpc_loopnconf_lock
#define ops_lock		__rpc_ops_lock
#define portnum_lock		__rpc_portnum_lock
#define proglst_lock		__rpc_proglst_lock
#define rpcbaddr_cache_lock	__rpc_rpcbaddr_cache_lock
#define rpcsoc_lock		__rpc_rpcsoc_lock
#define svc_fd_lock		__rpc_svc_fd_lock
#define svc_lock		__rpc_svc_lock
#define svcraw_lock		__rpc_svcraw_lock
#define xprtlist_lock		__rpc_xprtlist_lock

/* NFS server stuff */
#define svc_dg_create		_svc_dg_create

#define rpc_broadcast		_rpc_broadcast
#define rpc_broadcast_exp	_rpc_broadcast_exp

#define svc_fd_create		_svc_fd_create
#define svc_vc_create		_svc_vc_create
#define svc_getreq              _svc_getreq
#define svc_getreqset           _svc_getreqset
#define svc_getreq_common       _svc_getreq_common
#define svc_register            _svc_register
#define svc_reg                 _svc_reg
#define svc_unreg               _svc_unreg
#define svc_sendreply           _svc_sendreply
#define svc_unregister          _svc_unregister
#define svcerr_auth             _svcerr_auth
#define svcerr_decode           _svcerr_decode
#define svcerr_noproc           _svcerr_noproc
#define svcerr_noprog           _svcerr_noprog
#define svcerr_progvers         _svcerr_progvers
#define svcerr_systemerr        _svcerr_systemerr
#define svcerr_weakauth         _svcerr_weakauth
#define xprt_register           _xprt_register
#define xprt_unregister         _xprt_unregister
#define rpc_control             _rpc_control

#define taddr2uaddr		_taddr2uaddr
#define uaddr2taddr		_uaddr2taddr

#define rpcb_set		_rpcb_set
#define rpcb_unset		_rpcb_unset
#define rpcb_getmaps		_rpcb_getmaps
#define rpcb_taddr2uaddr	_rpcb_taddr2uaddr
#define rpcb_uaddr2taddr	_rpcb_uaddr2taddr

#endif /* _NAMESPACE_H_ */
