/*	$NetBSD: mt_misc.c,v 1.1 2000/06/02 23:11:11 fvdl Exp $	*/

/*
 *	Define and initialize MT data for libnsl.
 *	The _libnsl_lock_init() function below is the library's .init handler.
 */

/* #pragma ident	"@(#)mt_misc.c	1.24	93/04/29 SMI" */

#include	"reentrant.h"
#include	<rpc/rpc.h>
#include	<sys/time.h>
#include	<stdlib.h>

#ifdef _REENT

/*
 * XXX fvdl - this needs to be done right when implementing a thread-safe
 * libc.
 */

rwlock_t	svc_lock;	/* protects the services list (svc.c) */
rwlock_t	svc_fd_lock;	/* protects svc_fdset and the xports[] array */
rwlock_t	rpcbaddr_cache_lock; /* protects the RPCBIND address cache */
static rwlock_t	*rwlock_table[] = {
	&svc_lock,
	&svc_fd_lock,
	&rpcbaddr_cache_lock
};

mutex_t	authdes_lock;		/* protects authdes cache (svcauth_des.c) */
mutex_t	authnone_lock;		/* auth_none.c serialization */
mutex_t	authsvc_lock;		/* protects the Auths list (svc_auth.c) */
mutex_t	clnt_fd_lock;		/* protects client-side fd lock array */
mutex_t	clntraw_lock;		/* clnt_raw.c serialization */
mutex_t	dname_lock;		/* domainname and domain_fd (getdname.c) */
				/*	and default_domain (rpcdname.c) */
mutex_t	dupreq_lock;		/* dupreq variables (svc_dg.c) */
mutex_t	keyserv_lock;		/* protects first_time and hostname */
				/*	(key_call.c) */
mutex_t	libnsl_trace_lock;	/* serializes rpc_trace() (rpc_trace.c) */
mutex_t	loopnconf_lock;		/* loopnconf (rpcb_clnt.c) */
mutex_t	ops_lock;		/* serializes ops initializations */
mutex_t	portnum_lock;		/* protects ``port'' static in bindresvport() */
mutex_t	proglst_lock;		/* protects proglst list (svc_simple.c) */
mutex_t	rpcsoc_lock;		/* serializes clnt_com_create() (rpc_soc.c) */
mutex_t	svcraw_lock;		/* svc_raw.c serialization */
mutex_t	tsd_lock;		/* protects TSD key creation */
mutex_t	xprtlist_lock;		/* xprtlist (svc_generic.c) */
mutex_t serialize_pkey;		/* serializes calls to public key routines */


static mutex_t	*mutex_table[] = {
	&authdes_lock,
	&authnone_lock,
	&authsvc_lock,
	&clnt_fd_lock,
	&clntraw_lock,
	&dname_lock,
	&dupreq_lock,
	&keyserv_lock,
	&libnsl_trace_lock,
	&loopnconf_lock,
	&ops_lock,
	&portnum_lock,
	&proglst_lock,
	&rpcsoc_lock,
	&svcraw_lock,
	&tsd_lock,
	&xprtlist_lock,
	&serialize_pkey
};

int __rpc_lock_value;

#pragma init(_libnsl_lock_init)

void
_libnsl_lock_init()
{
	int	i;

/* _thr_main() returns -1 if libthread no linked in */

	if (_thr_main() == -1)
		lock_value = 0;
	else
		lock_value = 1;

	for (i = 0; i < sizeof (mutex_table) / sizeof (mutex_table[0]); i++)
		mutex_init(mutex_table[i], 0, (void *) 0);

	for (i = 0; i < sizeof (rwlock_table) / sizeof (rwlock_table[0]); i++)
		rwlock_init(rwlock_table[i], 0, (void *) 0);
}

#endif /* _REENT */


#undef	rpc_createerr

struct rpc_createerr rpc_createerr;

struct rpc_createerr *
__rpc_createerr()
{
#ifdef _REENT
	static thread_key_t rce_key = 0;
	struct rpc_createerr *rce_addr = 0;

	if (_thr_main())
		return (&rpc_createerr);
	if (_thr_getspecific(rce_key, (void **) &rce_addr) != 0) {
		mutex_lock(&tsd_lock);
		if (_thr_keycreate(&rce_key, free) != 0) {
			mutex_unlock(&tsd_lock);
			return (&rpc_createerr);
		}
		mutex_unlock(&tsd_lock);
	}
	if (!rce_addr) {
		rce_addr = (struct rpc_createerr *)
			malloc(sizeof (struct rpc_createerr));
		if (_thr_setspecific(rce_key, (void *) rce_addr) != 0) {
			if (rce_addr)
				free(rce_addr);
			return (&rpc_createerr);
		}
		memset(rce_addr, 0, sizeof (struct rpc_createerr));
		return (rce_addr);
	}
	return (rce_addr);
#else
	return &rpc_createerr;
#endif
}
