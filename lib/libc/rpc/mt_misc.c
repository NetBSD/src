/*	$NetBSD: mt_misc.c,v 1.1.6.2 2001/09/04 02:56:16 nathanw Exp $	*/

/*
 *	Define and initialize MT data for libnsl.
 *	The _libnsl_lock_init() function below is the library's .init handler.
 */

/* #pragma ident	"@(#)mt_misc.c	1.24	93/04/29 SMI" */

#include	"reentrant.h"
#include	<rpc/rpc.h>
#include	<sys/time.h>
#include	<stdlib.h>
#include	<string.h>

#ifdef _REENTRANT

/* protects the services list (svc.c) */
rwlock_t	svc_lock = RWLOCK_INITIALIZER;
/* protects svc_fdset and the xports[] array */
rwlock_t	svc_fd_lock = RWLOCK_INITIALIZER;
/* protects the RPCBIND address cache */
rwlock_t	rpcbaddr_cache_lock = RWLOCK_INITIALIZER;

/* protects authdes cache (svcauth_des.c) */
mutex_t	authdes_lock = MUTEX_INITIALIZER;
/* auth_none.c serialization */
mutex_t	authnone_lock = MUTEX_INITIALIZER;
/* protects the Auths list (svc_auth.c) */
mutex_t	authsvc_lock = MUTEX_INITIALIZER;
/* protects client-side fd lock array */
mutex_t	clnt_fd_lock = MUTEX_INITIALIZER;
/* clnt_raw.c serialization */
mutex_t	clntraw_lock = MUTEX_INITIALIZER;
/* domainname and domain_fd (getdname.c) and default_domain (rpcdname.c) */
mutex_t	dname_lock = MUTEX_INITIALIZER;
/* dupreq variables (svc_dg.c) */
mutex_t	dupreq_lock = MUTEX_INITIALIZER;
/* protects first_time and hostname (key_call.c) */
mutex_t	keyserv_lock = MUTEX_INITIALIZER;
/* serializes rpc_trace() (rpc_trace.c) */
mutex_t	libnsl_trace_lock = MUTEX_INITIALIZER;
/* loopnconf (rpcb_clnt.c) */
mutex_t	loopnconf_lock = MUTEX_INITIALIZER;
/* serializes ops initializations */
mutex_t	ops_lock = MUTEX_INITIALIZER;
/* protects ``port'' static in bindresvport() */
mutex_t	portnum_lock = MUTEX_INITIALIZER;
/* protects proglst list (svc_simple.c) */
mutex_t	proglst_lock = MUTEX_INITIALIZER;
/* serializes clnt_com_create() (rpc_soc.c) */
mutex_t	rpcsoc_lock = MUTEX_INITIALIZER;
/* svc_raw.c serialization */
mutex_t	svcraw_lock = MUTEX_INITIALIZER;
/* xprtlist (svc_generic.c) */
mutex_t	xprtlist_lock = MUTEX_INITIALIZER;
/* serializes calls to public key routines */
mutex_t serialize_pkey = MUTEX_INITIALIZER;

#endif /* _REENTRANT */


#undef	rpc_createerr

struct rpc_createerr rpc_createerr;

#ifdef _REENTRANT
static thread_key_t rce_key;
static once_t rce_once = ONCE_INITIALIZER;

static void 
__rpc_createerr_setup(void)
{

	thr_keycreate(&rce_key, free);
}
#endif /* _REENTRANT */

struct rpc_createerr*
__rpc_createerr()
{
#ifdef _REENTRANT
	struct rpc_createerr *rce_addr = 0;
	extern int __isthreaded;

	if (__isthreaded == 0)
		return (&rpc_createerr);
	thr_once(&rce_once, __rpc_createerr_setup);
	rce_addr = thr_getspecific(rce_key);
	if (rce_addr == NULL) {
		rce_addr = (struct rpc_createerr *)
		    malloc(sizeof (struct rpc_createerr));
		thr_setspecific(rce_key, (void *) rce_addr);
		memset(rce_addr, 0, sizeof (struct rpc_createerr));
	}
		
	return (rce_addr);
#else
	return &rpc_createerr;
#endif
}

