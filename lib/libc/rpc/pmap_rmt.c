/*	$NetBSD: pmap_rmt.c,v 1.27 2000/05/15 17:11:29 itojun Exp $	*/

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char *sccsid = "@(#)pmap_rmt.c 1.21 87/08/27 Copyr 1984 Sun Micro";
static char *sccsid = "@(#)pmap_rmt.c	2.2 88/08/01 4.0 RPCSRC";
#else
__RCSID("$NetBSD: pmap_rmt.c,v 1.27 2000/05/15 17:11:29 itojun Exp $");
#endif
#endif

/*
 * pmap_rmt.c
 * Client interface to pmap rpc service.
 * remote call and broadcast service
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include "namespace.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_rmt.h>

#include <ifaddrs.h>

#ifdef __weak_alias
__weak_alias(clnt_broadcast,_clnt_broadcast)
__weak_alias(pmap_rmtcall,_pmap_rmtcall)
__weak_alias(xdr_rmtcall_args,_xdr_rmtcall_args)
__weak_alias(xdr_rmtcallres,_xdr_rmtcallres)
#endif

#define MAX_BROADCAST_SIZE 1400

static int getbroadcastnets __P((struct in_addr *, int));

static const struct timeval timeout = { 3, 0 };

/*
 * pmapper remote-call-service interface.
 * This routine is used to call the pmapper remote call service
 * which will look up a service program in the port maps, and then
 * remotely call that routine with the given parameters.  This allows
 * programs to do a lookup and call in one step.
*/
enum clnt_stat
pmap_rmtcall(addr, prog, vers, proc, xdrargs, argsp, xdrres, resp, tout,
    port_ptr)
	struct sockaddr_in *addr;
	u_long prog, vers, proc;
	xdrproc_t xdrargs, xdrres;
	caddr_t argsp, resp;
	struct timeval tout;
	u_long *port_ptr;
{
	int sock = -1;
	CLIENT *client;
	struct rmtcallargs a;
	struct rmtcallres r;
	enum clnt_stat stat;

	_DIAGASSERT(addr != NULL);
	_DIAGASSERT(port_ptr != NULL);

	addr->sin_port = htons(PMAPPORT);
	client = clntudp_create(addr, PMAPPROG, PMAPVERS, timeout, &sock);
	if (client != (CLIENT *)NULL) {
		a.prog = prog;
		a.vers = vers;
		a.proc = proc;
		a.args_ptr = argsp;
		a.xdr_args = xdrargs;
		r.port_ptr = port_ptr;
		r.results_ptr = resp;
		r.xdr_results = xdrres;
		stat = CLNT_CALL(client, PMAPPROC_CALLIT,
		    (xdrproc_t)xdr_rmtcall_args, &a, (xdrproc_t)xdr_rmtcallres,
		    &r, tout);
		CLNT_DESTROY(client);
	} else {
		stat = RPC_FAILED;
	}
	addr->sin_port = 0;
	return (stat);
}


/*
 * XDR remote call arguments
 * written for XDR_ENCODE direction only
 */
bool_t
xdr_rmtcall_args(xdrs, cap)
	XDR *xdrs;
	struct rmtcallargs *cap;
{
	u_int lenposition, argposition, position;

	_DIAGASSERT(xdrs != NULL);
	_DIAGASSERT(cap != NULL);

	if (xdr_u_long(xdrs, &(cap->prog)) &&
	    xdr_u_long(xdrs, &(cap->vers)) &&
	    xdr_u_long(xdrs, &(cap->proc))) {
		lenposition = XDR_GETPOS(xdrs);
		if (! xdr_u_long(xdrs, &(cap->arglen)))
		    return (FALSE);
		argposition = XDR_GETPOS(xdrs);
		if (! (*(cap->xdr_args))(xdrs, cap->args_ptr))
		    return (FALSE);
		position = XDR_GETPOS(xdrs);
		cap->arglen = (u_long)position - (u_long)argposition;
		XDR_SETPOS(xdrs, lenposition);
		if (! xdr_u_long(xdrs, &(cap->arglen)))
		    return (FALSE);
		XDR_SETPOS(xdrs, position);
		return (TRUE);
	}
	return (FALSE);
}

/*
 * XDR remote call results
 * written for XDR_DECODE direction only
 */
bool_t
xdr_rmtcallres(xdrs, crp)
	XDR *xdrs;
	struct rmtcallres *crp;
{
	caddr_t port_ptr;

	_DIAGASSERT(xdrs != NULL);
	_DIAGASSERT(crp != NULL);

	port_ptr = (caddr_t)(void *)crp->port_ptr;
	if (xdr_reference(xdrs, &port_ptr, sizeof (u_long),
	    (xdrproc_t)xdr_u_long) && xdr_u_long(xdrs, &crp->resultslen)) {
		crp->port_ptr = (u_long *)(void *)port_ptr;
		return ((*(crp->xdr_results))(xdrs, crp->results_ptr));
	}
	return (FALSE);
}


/*
 * The following is kludged-up support for simple rpc broadcasts.
 * Someday a large, complicated system will replace these trivial 
 * routines which only support udp/ip .
 */

static int
getbroadcastnets(addrs, naddrs)
	struct in_addr *addrs;
	int naddrs;
{
	struct ifaddrs *ifap, *ifa;
	int i;
	struct sockaddr_in *sin;

	_DIAGASSERT(addrs != NULL);

	if (getifaddrs(&ifap) != 0) {
                warn("getbroadcastnets: getifaddrs");
		return 0;
	}
	i = 0;
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (i >= naddrs)
			break;
		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;
		if ((ifa->ifa_flags & IFF_BROADCAST) &&
		    (ifa->ifa_flags & IFF_UP)) {
			if (ifa->ifa_broadaddr && 
			    ifa->ifa_broadaddr->sa_family == AF_INET) {
				sin = (struct sockaddr_in *)ifa->ifa_broadaddr;
				addrs[i++] = sin->sin_addr;
			}
		}
	}
	freeifaddrs(ifap);
	return i;
}

typedef bool_t (*resultproc_t) __P((caddr_t, struct sockaddr_in *));

enum clnt_stat 
clnt_broadcast(prog, vers, proc, xargs, argsp, xresults, resultsp, eachresult)
	u_long		prog;		/* program number */
	u_long		vers;		/* version number */
	u_long		proc;		/* procedure number */
	xdrproc_t	xargs;		/* xdr routine for args */
	caddr_t		argsp;		/* pointer to args */
	xdrproc_t	xresults;	/* xdr routine for results */
	caddr_t		resultsp;	/* pointer to results */
	resultproc_t	eachresult;	/* call with each result obtained */
{
	enum clnt_stat stat;
	AUTH *unix_auth = authunix_create_default();
	XDR xdr_stream;
	XDR *xdrs = &xdr_stream;
	int inlen, nets;
	socklen_t fromlen;
	size_t outlen;
	int sock;
	int on = 1;
	struct pollfd fd;
	int i;
	bool_t done = FALSE;
	u_int32_t xid;
	u_long port;
	struct in_addr addrs[20];
	struct sockaddr_in baddr, raddr; /* broadcast and response addresses */
	struct rmtcallargs a;
	struct rmtcallres r;
	struct rpc_msg msg;
	struct timeval t; 
	int milliseconds;
	char outbuf[MAX_BROADCAST_SIZE], inbuf[UDPMSGSIZE];

	/*
	 * initialization: create a socket, a broadcast address, and
	 * preserialize the arguments into a send buffer.
	 */
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		warn("Cannot create socket for broadcast rpc");
		stat = RPC_CANTSEND;
		goto done_broad;
	}
#ifdef SO_BROADCAST
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof (on)) < 0) {
		warn("Cannot set socket option SO_BROADCAST");
		stat = RPC_CANTSEND;
		goto done_broad;
	}
#endif /* def SO_BROADCAST */
	fd.fd = sock;
	fd.events = POLLIN;
	nets = getbroadcastnets(addrs, sizeof(addrs) / sizeof(addrs[0]));
	memset(&baddr, 0, sizeof (baddr));
	baddr.sin_len = sizeof(struct sockaddr_in);
	baddr.sin_family = AF_INET;
	baddr.sin_port = htons(PMAPPORT);
	baddr.sin_addr.s_addr = htonl(INADDR_ANY);
	(void)gettimeofday(&t, (struct timezone *)0);
	msg.rm_xid = xid = (u_int32_t)(getpid() ^ t.tv_sec ^ t.tv_usec);
	t.tv_usec = 0;
	msg.rm_direction = CALL;
	msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	msg.rm_call.cb_prog = PMAPPROG;
	msg.rm_call.cb_vers = PMAPVERS;
	msg.rm_call.cb_proc = PMAPPROC_CALLIT;
	msg.rm_call.cb_cred = unix_auth->ah_cred;
	msg.rm_call.cb_verf = unix_auth->ah_verf;
	a.prog = prog;
	a.vers = vers;
	a.proc = proc;
	a.xdr_args = xargs;
	a.args_ptr = argsp;
	r.port_ptr = &port;
	r.xdr_results = xresults;
	r.results_ptr = resultsp;
	xdrmem_create(xdrs, outbuf, MAX_BROADCAST_SIZE, XDR_ENCODE);
	if ((! xdr_callmsg(xdrs, &msg)) || (! xdr_rmtcall_args(xdrs, &a))) {
		stat = RPC_CANTENCODEARGS;
		goto done_broad;
	}
	outlen = xdr_getpos(xdrs);
	xdr_destroy(xdrs);
	/*
	 * Basic loop: broadcast a packet and wait a while for response(s).
	 * The response timeout grows larger per iteration.
	 */
	for (t.tv_sec = 4; t.tv_sec <= 14; t.tv_sec += 2) {
		for (i = 0; i < nets; i++) {
			baddr.sin_addr = addrs[i];
			if (sendto(sock, outbuf, outlen, 0,
				(struct sockaddr *)(void *)&baddr,
				sizeof (struct sockaddr)) != outlen) {
				warn("Cannot send broadcast packet");
				stat = RPC_CANTSEND;
				goto done_broad;
			}
		}
		if (eachresult == NULL) {
			stat = RPC_SUCCESS;
			goto done_broad;
		}
	recv_again:
		milliseconds = (int)(t.tv_sec * 1000 + t.tv_usec / 1000);
		msg.acpted_rply.ar_verf = _null_auth;
		msg.acpted_rply.ar_results.where = (caddr_t)(void *)&r;
                msg.acpted_rply.ar_results.proc = (xdrproc_t)xdr_rmtcallres;
		switch (poll(&fd, 1, milliseconds)) {

		case 0:  /* timed out */
			stat = RPC_TIMEDOUT;
			continue;

		case -1:  /* some kind of error */
			if (errno == EINTR)
				goto recv_again;
			warn("Broadcast poll problem");
			stat = RPC_CANTRECV;
			goto done_broad;

		}  /* end of poll results switch */
	try_again:
		fromlen = sizeof(struct sockaddr);
		inlen = recvfrom(sock, inbuf, UDPMSGSIZE, 0,
			(struct sockaddr *)(void *)&raddr, &fromlen);
		if (inlen < 0) {
			if (errno == EINTR)
				goto try_again;
			warn("Cannot receive reply to broadcast");
			stat = RPC_CANTRECV;
			goto done_broad;
		}
		if (inlen < sizeof(u_int32_t))
			goto recv_again;
		/*
		 * see if reply transaction id matches sent id.
		 * If so, decode the results.
		 */
		xdrmem_create(xdrs, inbuf, (u_int)inlen, XDR_DECODE);
		if (xdr_replymsg(xdrs, &msg)) {
			if ((msg.rm_xid == xid) &&
				(msg.rm_reply.rp_stat == MSG_ACCEPTED) &&
				(msg.acpted_rply.ar_stat == SUCCESS)) {
				raddr.sin_port = htons((u_short)port);
				done = (*eachresult)(resultsp, &raddr);
			}
			/* otherwise, we just ignore the errors ... */
		} else {
#ifdef notdef
			/* some kind of deserialization problem ... */
			if (msg.rm_xid == xid)
				fprintf(stderr,
				    "Broadcast deserialization problem");
			/* otherwise, just random garbage */
#endif
		}
		xdrs->x_op = XDR_FREE;
		msg.acpted_rply.ar_results.proc = (xdrproc_t)xdr_void;
		(void)xdr_replymsg(xdrs, &msg);
		(void)(*xresults)(xdrs, resultsp);
		xdr_destroy(xdrs);
		if (done) {
			stat = RPC_SUCCESS;
			goto done_broad;
		} else {
			goto recv_again;
		}
	}
	stat = RPC_TIMEDOUT;
done_broad:
	if (sock != -1)
		(void)close(sock);
	AUTH_DESTROY(unix_auth);
	return (stat);
}
