/*	NetBSD: krpc.h,v 1.4 1995/12/19 23:07:11 cgd Exp 	*/
/* FreeBSD: head/sys/nfs/krpc.h 221032 2011-04-25 22:22:51Z rmacklem 	*/
/* $NetBSD: krpc.h,v 1.1.1.1.10.3 2017/12/03 11:38:42 jdolecek Exp $	*/

#include <sys/cdefs.h>

struct mbuf;
struct thread;
struct sockaddr;
struct sockaddr_in;

int krpc_call(struct sockaddr_in *_sin,
	u_int prog, u_int vers, u_int func,
	struct mbuf **data, struct sockaddr **from, struct lwp *td);

int krpc_portmap(struct sockaddr_in *_sin,
	u_int prog, u_int vers, u_int16_t *portp, struct lwp *td);

struct mbuf *xdr_string_encode(char *str, int len);

/*
 * RPC definitions for the portmapper
 */
#define	PMAPPORT		111
#define	PMAPPROG		100000
#define	PMAPVERS		2
#define	PMAPPROC_NULL		0
#define	PMAPPROC_SET		1
#define	PMAPPROC_UNSET		2
#define	PMAPPROC_GETPORT	3
#define	PMAPPROC_DUMP		4
#define	PMAPPROC_CALLIT		5
