/*	$NetBSD: svc_dg.c,v 1.12.24.1 2013/03/14 22:03:08 riz Exp $	*/

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

/*
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/* #ident	"@(#)svc_dg.c	1.17	94/04/24 SMI" */


/*
 * svc_dg.c, Server side for connectionless RPC.
 *
 * Does some caching in the hopes of achieving execute-at-most-once semantics.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: svc_dg.c,v 1.12.24.1 2013/03/14 22:03:08 riz Exp $");
#endif

#include "namespace.h"
#include "reentrant.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <rpc/rpc.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef RPC_CACHE_DEBUG
#include <netconfig.h>
#include <netdir.h>
#endif
#include <err.h>

#include "rpc_internal.h"
#include "svc_dg.h"

#define	su_data(xprt)	((struct svc_dg_data *)(xprt->xp_p2))
#define	rpc_buffer(xprt) ((xprt)->xp_p1)

#ifdef __weak_alias
__weak_alias(svc_dg_create,_svc_dg_create)
#endif

#ifndef MAX
#define	MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif

static void svc_dg_ops __P((SVCXPRT *));
static enum xprt_stat svc_dg_stat __P((SVCXPRT *));
static bool_t svc_dg_recv __P((SVCXPRT *, struct rpc_msg *));
static bool_t svc_dg_reply __P((SVCXPRT *, struct rpc_msg *));
static bool_t svc_dg_getargs __P((SVCXPRT *, xdrproc_t, caddr_t));
static bool_t svc_dg_freeargs __P((SVCXPRT *, xdrproc_t, caddr_t));
static void svc_dg_destroy __P((SVCXPRT *));
static bool_t svc_dg_control __P((SVCXPRT *, const u_int, void *));
static int cache_get __P((SVCXPRT *, struct rpc_msg *, char **, size_t *));
static void cache_set __P((SVCXPRT *, size_t));

/*
 * Usage:
 *	xprt = svc_dg_create(sock, sendsize, recvsize);
 * Does other connectionless specific initializations.
 * Once *xprt is initialized, it is registered.
 * see (svc.h, xprt_register). If recvsize or sendsize are 0 suitable
 * system defaults are chosen.
 * The routines returns NULL if a problem occurred.
 */
static const char svc_dg_str[] = "svc_dg_create: %s";
static const char svc_dg_err1[] = "could not get transport information";
static const char svc_dg_err2[] = " transport does not support data transfer";
static const char __no_mem_str[] = "out of memory";

SVCXPRT *
svc_dg_create(fd, sendsize, recvsize)
	int fd;
	u_int sendsize;
	u_int recvsize;
{
	SVCXPRT *xprt;
	struct svc_dg_data *su = NULL;
	struct __rpc_sockinfo si;
	struct sockaddr_storage ss;
	socklen_t slen;

	if (!__rpc_fd2sockinfo(fd, &si)) {
		warnx(svc_dg_str, svc_dg_err1);
		return (NULL);
	}
	/*
	 * Find the receive and the send size
	 */
	sendsize = __rpc_get_t_size(si.si_af, si.si_proto, (int)sendsize);
	recvsize = __rpc_get_t_size(si.si_af, si.si_proto, (int)recvsize);
	if ((sendsize == 0) || (recvsize == 0)) {
		warnx(svc_dg_str, svc_dg_err2);
		return (NULL);
	}

	xprt = mem_alloc(sizeof (SVCXPRT));
	if (xprt == NULL)
		goto freedata;
	memset(xprt, 0, sizeof (SVCXPRT));

	su = mem_alloc(sizeof (*su));
	if (su == NULL)
		goto freedata;
	su->su_iosz = ((MAX(sendsize, recvsize) + 3) / 4) * 4;
	if ((rpc_buffer(xprt) = malloc(su->su_iosz)) == NULL)
		goto freedata;
	xdrmem_create(&(su->su_xdrs), rpc_buffer(xprt), su->su_iosz,
		XDR_DECODE);
	su->su_cache = NULL;
	xprt->xp_fd = fd;
	xprt->xp_p2 = (caddr_t)(void *)su;
	xprt->xp_verf.oa_base = su->su_verfbody;
	svc_dg_ops(xprt);
	xprt->xp_rtaddr.maxlen = sizeof (struct sockaddr_storage);

	slen = sizeof ss;
	if (getsockname(fd, (struct sockaddr *)(void *)&ss, &slen) < 0)
		goto freedata;
	xprt->xp_ltaddr.buf = mem_alloc(sizeof (struct sockaddr_storage));
	xprt->xp_ltaddr.maxlen = sizeof (struct sockaddr_storage);
	xprt->xp_ltaddr.len = slen;
	memcpy(xprt->xp_ltaddr.buf, &ss, slen);

	xprt_register(xprt);
	return (xprt);
freedata:
	(void) warnx(svc_dg_str, __no_mem_str);
	if (xprt) {
		if (su)
			(void) mem_free(su, sizeof (*su));
		(void) mem_free(xprt, sizeof (SVCXPRT));
	}
	return (NULL);
}

/*ARGSUSED*/
static enum xprt_stat
svc_dg_stat(xprt)
	SVCXPRT *xprt;
{
	return (XPRT_IDLE);
}

static bool_t
svc_dg_recv(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	struct svc_dg_data *su;
	XDR *xdrs;
	char *reply;
	struct sockaddr_storage ss;
	socklen_t alen;
	size_t replylen;
	ssize_t rlen;

	_DIAGASSERT(xprt != NULL);
	_DIAGASSERT(msg != NULL);

	su = su_data(xprt);
	xdrs = &(su->su_xdrs);

again:
	alen = sizeof (struct sockaddr_storage);
	rlen = recvfrom(xprt->xp_fd, rpc_buffer(xprt), su->su_iosz, 0,
	    (struct sockaddr *)(void *)&ss, &alen);
	if (rlen == -1 && errno == EINTR)
		goto again;
	if (rlen == -1 || (rlen < (ssize_t)(4 * sizeof (u_int32_t))))
		return (FALSE);
	if (xprt->xp_rtaddr.len < alen) {
		if (xprt->xp_rtaddr.len != 0)
			mem_free(xprt->xp_rtaddr.buf, xprt->xp_rtaddr.len);
		xprt->xp_rtaddr.buf = mem_alloc(alen);
		xprt->xp_rtaddr.len = alen;
	}
	memcpy(xprt->xp_rtaddr.buf, &ss, alen);
#ifdef PORTMAP
	if (ss.ss_family == AF_INET) {
		xprt->xp_raddr = *(struct sockaddr_in *)xprt->xp_rtaddr.buf;
		xprt->xp_addrlen = sizeof (struct sockaddr_in);
	}
#endif
	xdrs->x_op = XDR_DECODE;
	XDR_SETPOS(xdrs, 0);
	if (! xdr_callmsg(xdrs, msg)) {
		return (FALSE);
	}
	su->su_xid = msg->rm_xid;
	if (su->su_cache != NULL) {
		if (cache_get(xprt, msg, &reply, &replylen)) {
			(void)sendto(xprt->xp_fd, reply, replylen, 0,
			    (struct sockaddr *)(void *)&ss, alen);
			return (FALSE);
		}
	}
	return (TRUE);
}

static bool_t
svc_dg_reply(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	struct svc_dg_data *su;
	XDR *xdrs;
	bool_t stat = FALSE;
	size_t slen;

	_DIAGASSERT(xprt != NULL);
	_DIAGASSERT(msg != NULL);

	su = su_data(xprt);
	xdrs = &(su->su_xdrs);

	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, 0);
	msg->rm_xid = su->su_xid;
	if (xdr_replymsg(xdrs, msg)) {
		slen = XDR_GETPOS(xdrs);
		if (sendto(xprt->xp_fd, rpc_buffer(xprt), slen, 0,
		    (struct sockaddr *)xprt->xp_rtaddr.buf,
		    (socklen_t)xprt->xp_rtaddr.len) == (ssize_t) slen) {
			stat = TRUE;
			if (su->su_cache)
				cache_set(xprt, slen);
		}
	}
	return (stat);
}

static bool_t
svc_dg_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	return (*xdr_args)(&(su_data(xprt)->su_xdrs), args_ptr);
}

static bool_t
svc_dg_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	XDR *xdrs;

	_DIAGASSERT(xprt != NULL);

	xdrs = &(su_data(xprt)->su_xdrs);
	xdrs->x_op = XDR_FREE;
	return (*xdr_args)(xdrs, args_ptr);
}

static void
svc_dg_destroy(xprt)
	SVCXPRT *xprt;
{
	struct svc_dg_data *su;

	_DIAGASSERT(xprt != NULL);

	su = su_data(xprt);

	xprt_unregister(xprt);
	if (xprt->xp_fd != -1)
		(void)close(xprt->xp_fd);
	XDR_DESTROY(&(su->su_xdrs));
	(void) mem_free(rpc_buffer(xprt), su->su_iosz);
	(void) mem_free(su, sizeof (*su));
	if (xprt->xp_rtaddr.buf)
		(void) mem_free(xprt->xp_rtaddr.buf, xprt->xp_rtaddr.maxlen);
	if (xprt->xp_ltaddr.buf)
		(void) mem_free(xprt->xp_ltaddr.buf, xprt->xp_ltaddr.maxlen);
	if (xprt->xp_tp)
		(void) free(xprt->xp_tp);
	(void) mem_free(xprt, sizeof (SVCXPRT));
}

static bool_t
/*ARGSUSED*/
svc_dg_control(xprt, rq, in)
	SVCXPRT *xprt;
	const u_int	rq;
	void		*in;
{
	return (FALSE);
}

static void
svc_dg_ops(xprt)
	SVCXPRT *xprt;
{
	static struct xp_ops ops;
	static struct xp_ops2 ops2;
#ifdef _REENTRANT
	extern mutex_t ops_lock;
#endif

	_DIAGASSERT(xprt != NULL);

/* VARIABLES PROTECTED BY ops_lock: ops */

	mutex_lock(&ops_lock);
	if (ops.xp_recv == NULL) {
		ops.xp_recv = svc_dg_recv;
		ops.xp_stat = svc_dg_stat;
		ops.xp_getargs = svc_dg_getargs;
		ops.xp_reply = svc_dg_reply;
		ops.xp_freeargs = svc_dg_freeargs;
		ops.xp_destroy = svc_dg_destroy;
		ops2.xp_control = svc_dg_control;
	}
	xprt->xp_ops = &ops;
	xprt->xp_ops2 = &ops2;
	mutex_unlock(&ops_lock);
}

/*  The CACHING COMPONENT */

/*
 * Could have been a separate file, but some part of it depends upon the
 * private structure of the client handle.
 *
 * Fifo cache for cl server
 * Copies pointers to reply buffers into fifo cache
 * Buffers are sent again if retransmissions are detected.
 */

#define	SPARSENESS 4	/* 75% sparse */

#define	ALLOC(type, size)	\
	mem_alloc((sizeof (type) * (size)))

#define	MEMZERO(addr, type, size)	 \
	(void) memset((void *) (addr), 0, sizeof (type) * (int) (size))

#define	FREE(addr, type, size)	\
	mem_free((addr), (sizeof (type) * (size)))

/*
 * An entry in the cache
 */
typedef struct cache_node *cache_ptr;
struct cache_node {
	/*
	 * Index into cache is xid, proc, vers, prog and address
	 */
	u_int32_t cache_xid;
	rpcproc_t cache_proc;
	rpcvers_t cache_vers;
	rpcprog_t cache_prog;
	struct netbuf cache_addr;
	/*
	 * The cached reply and length
	 */
	char *cache_reply;
	size_t cache_replylen;
	/*
	 * Next node on the list, if there is a collision
	 */
	cache_ptr cache_next;
};

/*
 * The entire cache
 */
struct cl_cache {
	u_int uc_size;		/* size of cache */
	cache_ptr *uc_entries;	/* hash table of entries in cache */
	cache_ptr *uc_fifo;	/* fifo list of entries in cache */
	u_int uc_nextvictim;	/* points to next victim in fifo list */
	rpcprog_t uc_prog;	/* saved program number */
	rpcvers_t uc_vers;	/* saved version number */
	rpcproc_t uc_proc;	/* saved procedure number */
};


/*
 * the hashing function
 */
#define	CACHE_LOC(transp, xid)	\
	(xid % (SPARSENESS * ((struct cl_cache *) \
		su_data(transp)->su_cache)->uc_size))

#ifdef _REENTRANT
extern mutex_t	dupreq_lock;
#endif

/*
 * Enable use of the cache. Returns 1 on success, 0 on failure.
 * Note: there is no disable.
 */
static const char cache_enable_str[] = "svc_enablecache: %s %s";
static const char alloc_err[] = "could not allocate cache ";
static const char enable_err[] = "cache already enabled";

int
svc_dg_enablecache(transp, size)
	SVCXPRT *transp;
	u_int size;
{
	struct svc_dg_data *su;
	struct cl_cache *uc;

	_DIAGASSERT(transp != NULL);

	su = su_data(transp);

	mutex_lock(&dupreq_lock);
	if (su->su_cache != NULL) {
		(void) warnx(cache_enable_str, enable_err, " ");
		mutex_unlock(&dupreq_lock);
		return (0);
	}
	uc = ALLOC(struct cl_cache, 1);
	if (uc == NULL) {
		warnx(cache_enable_str, alloc_err, " ");
		mutex_unlock(&dupreq_lock);
		return (0);
	}
	uc->uc_size = size;
	uc->uc_nextvictim = 0;
	uc->uc_entries = ALLOC(cache_ptr, size * SPARSENESS);
	if (uc->uc_entries == NULL) {
		warnx(cache_enable_str, alloc_err, "data");
		FREE(uc, struct cl_cache, 1);
		mutex_unlock(&dupreq_lock);
		return (0);
	}
	MEMZERO(uc->uc_entries, cache_ptr, size * SPARSENESS);
	uc->uc_fifo = ALLOC(cache_ptr, size);
	if (uc->uc_fifo == NULL) {
		warnx(cache_enable_str, alloc_err, "fifo");
		FREE(uc->uc_entries, cache_ptr, size * SPARSENESS);
		FREE(uc, struct cl_cache, 1);
		mutex_unlock(&dupreq_lock);
		return (0);
	}
	MEMZERO(uc->uc_fifo, cache_ptr, size);
	su->su_cache = (char *)(void *)uc;
	mutex_unlock(&dupreq_lock);
	return (1);
}

/*
 * Set an entry in the cache.  It assumes that the uc entry is set from
 * the earlier call to cache_get() for the same procedure.  This will always
 * happen because cache_get() is calle by svc_dg_recv and cache_set() is called
 * by svc_dg_reply().  All this hoopla because the right RPC parameters are
 * not available at svc_dg_reply time.
 */

static const char cache_set_str[] = "cache_set: %s";
static const char cache_set_err1[] = "victim not found";
static const char cache_set_err2[] = "victim alloc failed";
static const char cache_set_err3[] = "could not allocate new rpc buffer";

static void
cache_set(xprt, replylen)
	SVCXPRT *xprt;
	size_t replylen;
{
	cache_ptr victim;
	cache_ptr *vicp;
	struct svc_dg_data *su;
	struct cl_cache *uc;
	u_int loc;
	char *newbuf;
#ifdef RPC_CACHE_DEBUG
	struct netconfig *nconf;
	char *uaddr;
#endif

	_DIAGASSERT(xprt != NULL);

	su = su_data(xprt);
	uc = (struct cl_cache *) su->su_cache;

	mutex_lock(&dupreq_lock);
	/*
	 * Find space for the new entry, either by
	 * reusing an old entry, or by mallocing a new one
	 */
	victim = uc->uc_fifo[uc->uc_nextvictim];
	if (victim != NULL) {
		loc = CACHE_LOC(xprt, victim->cache_xid);
		for (vicp = &uc->uc_entries[loc];
			*vicp != NULL && *vicp != victim;
			vicp = &(*vicp)->cache_next)
			;
		if (*vicp == NULL) {
			warnx(cache_set_str, cache_set_err1);
			mutex_unlock(&dupreq_lock);
			return;
		}
		*vicp = victim->cache_next;	/* remove from cache */
		newbuf = victim->cache_reply;
	} else {
		victim = ALLOC(struct cache_node, 1);
		if (victim == NULL) {
			warnx(cache_set_str, cache_set_err2);
			mutex_unlock(&dupreq_lock);
			return;
		}
		newbuf = mem_alloc(su->su_iosz);
		if (newbuf == NULL) {
			warnx(cache_set_str, cache_set_err3);
			FREE(victim, struct cache_node, 1);
			mutex_unlock(&dupreq_lock);
			return;
		}
	}

	/*
	 * Store it away
	 */
#ifdef RPC_CACHE_DEBUG
	if (nconf = getnetconfigent(xprt->xp_netid)) {
		uaddr = taddr2uaddr(nconf, &xprt->xp_rtaddr);
		freenetconfigent(nconf);
		printf(
	"cache set for xid= %x prog=%d vers=%d proc=%d for rmtaddr=%s\n",
			su->su_xid, uc->uc_prog, uc->uc_vers,
			uc->uc_proc, uaddr);
		free(uaddr);
	}
#endif
	victim->cache_replylen = replylen;
	victim->cache_reply = rpc_buffer(xprt);
	rpc_buffer(xprt) = newbuf;
	xdrmem_create(&(su->su_xdrs), rpc_buffer(xprt),
			su->su_iosz, XDR_ENCODE);
	victim->cache_xid = su->su_xid;
	victim->cache_proc = uc->uc_proc;
	victim->cache_vers = uc->uc_vers;
	victim->cache_prog = uc->uc_prog;
	victim->cache_addr = xprt->xp_rtaddr;
	victim->cache_addr.buf = ALLOC(char, xprt->xp_rtaddr.len);
	(void) memcpy(victim->cache_addr.buf, xprt->xp_rtaddr.buf,
	    (size_t)xprt->xp_rtaddr.len);
	loc = CACHE_LOC(xprt, victim->cache_xid);
	victim->cache_next = uc->uc_entries[loc];
	uc->uc_entries[loc] = victim;
	uc->uc_fifo[uc->uc_nextvictim++] = victim;
	uc->uc_nextvictim %= uc->uc_size;
	mutex_unlock(&dupreq_lock);
}

/*
 * Try to get an entry from the cache
 * return 1 if found, 0 if not found and set the stage for cache_set()
 */
static int
cache_get(xprt, msg, replyp, replylenp)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
	char **replyp;
	size_t *replylenp;
{
	u_int loc;
	cache_ptr ent;
	struct svc_dg_data *su;
	struct cl_cache *uc;
#ifdef RPC_CACHE_DEBUG
	struct netconfig *nconf;
	char *uaddr;
#endif

	_DIAGASSERT(xprt != NULL);
	_DIAGASSERT(msg != NULL);
	_DIAGASSERT(replyp != NULL);
	_DIAGASSERT(replylenp != NULL);

	su = su_data(xprt);
	uc = (struct cl_cache *) su->su_cache;

	mutex_lock(&dupreq_lock);
	loc = CACHE_LOC(xprt, su->su_xid);
	for (ent = uc->uc_entries[loc]; ent != NULL; ent = ent->cache_next) {
		if (ent->cache_xid == su->su_xid &&
			ent->cache_proc == msg->rm_call.cb_proc &&
			ent->cache_vers == msg->rm_call.cb_vers &&
			ent->cache_prog == msg->rm_call.cb_prog &&
			ent->cache_addr.len == xprt->xp_rtaddr.len &&
			(memcmp(ent->cache_addr.buf, xprt->xp_rtaddr.buf,
				xprt->xp_rtaddr.len) == 0)) {
#ifdef RPC_CACHE_DEBUG
			if (nconf = getnetconfigent(xprt->xp_netid)) {
				uaddr = taddr2uaddr(nconf, &xprt->xp_rtaddr);
				freenetconfigent(nconf);
				printf(
	"cache entry found for xid=%x prog=%d vers=%d proc=%d for rmtaddr=%s\n",
					su->su_xid, msg->rm_call.cb_prog,
					msg->rm_call.cb_vers,
					msg->rm_call.cb_proc, uaddr);
				free(uaddr);
			}
#endif
			*replyp = ent->cache_reply;
			*replylenp = ent->cache_replylen;
			mutex_unlock(&dupreq_lock);
			return (1);
		}
	}
	/*
	 * Failed to find entry
	 * Remember a few things so we can do a set later
	 */
	uc->uc_proc = msg->rm_call.cb_proc;
	uc->uc_vers = msg->rm_call.cb_vers;
	uc->uc_prog = msg->rm_call.cb_prog;
	mutex_unlock(&dupreq_lock);
	return (0);
}
