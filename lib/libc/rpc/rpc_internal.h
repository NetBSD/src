/*	$NetBSD: rpc_internal.h,v 1.3 2003/09/09 00:22:17 itojun Exp $	*/

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
