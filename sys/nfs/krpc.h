/*	$NetBSD: krpc.h,v 1.2 1994/10/26 02:53:36 cgd Exp $	*/

#include <sys/cdefs.h>

int krpc_call __P((struct sockaddr_in *sin, \
	u_long prog, u_long vers, u_long func, \
	struct mbuf **data, struct mbuf **from));

int krpc_portmap __P((struct sockaddr_in *sin, \
	u_long prog, u_long vers, u_short *portp));


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


/*
 * RPC definitions for bootparamd
 */
#define	BOOTPARAM_PROG		100026
#define	BOOTPARAM_VERS		1
#define BOOTPARAM_WHOAMI	1
#define BOOTPARAM_GETFILE	2

