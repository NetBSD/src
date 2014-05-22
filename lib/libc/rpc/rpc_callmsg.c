/*	$NetBSD: rpc_callmsg.c,v 1.18.4.2 2014/05/22 11:36:53 yamt Exp $	*/

/*
 * Copyright (c) 2010, Oracle America, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *     * Neither the name of the "Oracle America, Inc." nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 *   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char *sccsid = "@(#)rpc_callmsg.c 1.4 87/08/11 Copyr 1984 Sun Micro";
static char *sccsid = "@(#)rpc_callmsg.c	2.1 88/07/29 4.0 RPCSRC";
#else
__RCSID("$NetBSD: rpc_callmsg.c,v 1.18.4.2 2014/05/22 11:36:53 yamt Exp $");
#endif
#endif

/*
 * rpc_callmsg.c
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 */

#include "namespace.h"

#include <sys/param.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <rpc/rpc.h>

#ifdef __weak_alias
__weak_alias(xdr_callmsg,_xdr_callmsg)
#endif

/*
 * XDR a call message
 */
bool_t
xdr_callmsg(XDR *xdrs, struct rpc_msg *cmsg)
{
	int32_t *buf;
	struct opaque_auth *oa;

	_DIAGASSERT(xdrs != NULL);
	_DIAGASSERT(cmsg != NULL);

	if (xdrs->x_op == XDR_ENCODE) {
		if (cmsg->rm_call.cb_cred.oa_length > MAX_AUTH_BYTES) {
			return (FALSE);
		}
		if (cmsg->rm_call.cb_verf.oa_length > MAX_AUTH_BYTES) {
			return (FALSE);
		}
		buf = XDR_INLINE(xdrs, 8 * BYTES_PER_XDR_UNIT
			+ RNDUP(cmsg->rm_call.cb_cred.oa_length)
			+ 2 * BYTES_PER_XDR_UNIT
			+ RNDUP(cmsg->rm_call.cb_verf.oa_length));
		if (buf != NULL) {
			IXDR_PUT_INT32(buf, cmsg->rm_xid);
			IXDR_PUT_ENUM(buf, cmsg->rm_direction);
			if (cmsg->rm_direction != CALL) {
				return (FALSE);
			}
			IXDR_PUT_INT32(buf, cmsg->rm_call.cb_rpcvers);
			if (cmsg->rm_call.cb_rpcvers != RPC_MSG_VERSION) {
				return (FALSE);
			}
			IXDR_PUT_INT32(buf, cmsg->rm_call.cb_prog);
			IXDR_PUT_INT32(buf, cmsg->rm_call.cb_vers);
			IXDR_PUT_INT32(buf, cmsg->rm_call.cb_proc);
			oa = &cmsg->rm_call.cb_cred;
			IXDR_PUT_ENUM(buf, oa->oa_flavor);
			IXDR_PUT_INT32(buf, oa->oa_length);
			if (oa->oa_length) {
				memmove(buf, oa->oa_base, oa->oa_length);
				buf += RNDUP(oa->oa_length) / sizeof (int32_t);
			}
			oa = &cmsg->rm_call.cb_verf;
			IXDR_PUT_ENUM(buf, oa->oa_flavor);
			IXDR_PUT_INT32(buf, oa->oa_length);
			if (oa->oa_length) {
				memmove(buf, oa->oa_base, oa->oa_length);
				/* no real need....
				buf += RNDUP(oa->oa_length) / sizeof (int32_t);
				*/
			}
			return (TRUE);
		}
	}
	if (xdrs->x_op == XDR_DECODE) {
		buf = XDR_INLINE(xdrs, 8 * BYTES_PER_XDR_UNIT);
		if (buf != NULL) {
			cmsg->rm_xid = IXDR_GET_U_INT32(buf);
			cmsg->rm_direction = IXDR_GET_ENUM(buf, enum msg_type);
			if (cmsg->rm_direction != CALL) {
				return (FALSE);
			}
			cmsg->rm_call.cb_rpcvers = IXDR_GET_U_INT32(buf);
			if (cmsg->rm_call.cb_rpcvers != RPC_MSG_VERSION) {
				return (FALSE);
			}
			cmsg->rm_call.cb_prog = IXDR_GET_U_INT32(buf);
			cmsg->rm_call.cb_vers = IXDR_GET_U_INT32(buf);
			cmsg->rm_call.cb_proc = IXDR_GET_U_INT32(buf);
			oa = &cmsg->rm_call.cb_cred;
			oa->oa_flavor = IXDR_GET_ENUM(buf, enum_t);
			oa->oa_length = (u_int)IXDR_GET_U_INT32(buf);
			if (oa->oa_length) {
				if (oa->oa_length > MAX_AUTH_BYTES) {
					return (FALSE);
				}
				if (oa->oa_base == NULL) {
					oa->oa_base = mem_alloc(oa->oa_length);
					if (oa->oa_base == NULL)
						return (FALSE);
				}
				buf = XDR_INLINE(xdrs, RNDUP(oa->oa_length));
				if (buf == NULL) {
					if (xdr_opaque(xdrs, oa->oa_base,
					    oa->oa_length) == FALSE) {
						return (FALSE);
					}
				} else {
					memmove(oa->oa_base, buf,
					    oa->oa_length);
					/* no real need....
					buf += RNDUP(oa->oa_length) /
						sizeof (int32_t);
					*/
				}
			}
			oa = &cmsg->rm_call.cb_verf;
			buf = XDR_INLINE(xdrs, 2 * BYTES_PER_XDR_UNIT);
			if (buf == NULL) {
				if (xdr_enum(xdrs, &oa->oa_flavor) == FALSE ||
				    xdr_u_int(xdrs, &oa->oa_length) == FALSE) {
					return (FALSE);
				}
			} else {
				oa->oa_flavor = IXDR_GET_ENUM(buf, enum_t);
				oa->oa_length = (u_int)IXDR_GET_U_INT32(buf);
			}
			if (oa->oa_length) {
				if (oa->oa_length > MAX_AUTH_BYTES) {
					return (FALSE);
				}
				if (oa->oa_base == NULL) {
					oa->oa_base = mem_alloc(oa->oa_length);
					if (oa->oa_base == NULL)
						return (FALSE);
				}
				buf = XDR_INLINE(xdrs, RNDUP(oa->oa_length));
				if (buf == NULL) {
					if (xdr_opaque(xdrs, oa->oa_base,
					    oa->oa_length) == FALSE) {
						return (FALSE);
					}
				} else {
					memmove(oa->oa_base, buf,
					    oa->oa_length);
					/* no real need...
					buf += RNDUP(oa->oa_length) /
						sizeof (int32_t);
					*/
				}
			}
			return (TRUE);
		}
	}
	if (
	    xdr_u_int32_t(xdrs, &(cmsg->rm_xid)) &&
	    xdr_enum(xdrs, (enum_t *)(void *)&(cmsg->rm_direction)) &&
	    (cmsg->rm_direction == CALL) &&
	    xdr_u_int32_t(xdrs, &(cmsg->rm_call.cb_rpcvers)) &&
	    (cmsg->rm_call.cb_rpcvers == RPC_MSG_VERSION) &&
	    xdr_u_int32_t(xdrs, &(cmsg->rm_call.cb_prog)) &&
	    xdr_u_int32_t(xdrs, &(cmsg->rm_call.cb_vers)) &&
	    xdr_u_int32_t(xdrs, &(cmsg->rm_call.cb_proc)) &&
	    xdr_opaque_auth(xdrs, &(cmsg->rm_call.cb_cred)) )
		return (xdr_opaque_auth(xdrs, &(cmsg->rm_call.cb_verf)));
	return (FALSE);
}
