/*	$NetBSD: bindresvport.c,v 1.14 1999/01/20 11:37:35 lukem Exp $	*/

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
static char *sccsid = "@(#)bindresvport.c 1.8 88/02/08 SMI";
static char *sccsid = "@(#)bindresvport.c	2.2 88/07/29 4.0 RPCSRC";
#else
__RCSID("$NetBSD: bindresvport.c,v 1.14 1999/01/20 11:37:35 lukem Exp $");
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "namespace.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <rpc/rpc.h>

#ifdef __weak_alias
__weak_alias(bindresvport,_bindresvport);
#endif

/*
 * Bind a socket to a privileged IP port
 */
int
bindresvport(sd, sin)
	int sd;
	struct sockaddr_in *sin;
{
	int res, old;
	struct sockaddr_in myaddr;
	int sinlen = sizeof(struct sockaddr_in);

	if (sin == NULL) {
		sin = &myaddr;
		memset(sin, 0, (size_t)sinlen);
		sin->sin_len = sinlen;
		sin->sin_family = AF_INET;
	} else if (sin->sin_family != AF_INET) {
		errno = EPFNOSUPPORT;
		return (-1);
	}

	if (sin->sin_port == 0) {
		int on, oldlen = sizeof(old);

		res = getsockopt(sd, IPPROTO_IP, IP_PORTRANGE, &old, &oldlen);
		if (res < 0)
			return(res);
		on = IP_PORTRANGE_LOW;
		res = setsockopt(sd, IPPROTO_IP, IP_PORTRANGE, &on, sizeof(on));
		if (res < 0)
			return(res);
	}

	res = bind(sd, (struct sockaddr *)(void *)sin, sinlen);

	if (sin->sin_port == 0) {
		int saved_errno = errno;

		if (res < 0) {
			if (setsockopt(sd, IPPROTO_IP, IP_PORTRANGE,
			    &old, sizeof(old)) < 0)
				errno = saved_errno;
			return (res);
		}

		if (sin != &myaddr) {	/* What did the kernel assign? */
			if (getsockname(sd, (struct sockaddr *)(void *)sin,
			    &sinlen) < 0)
				errno = saved_errno;
			return (res);
		}
	}
	return (res);
}
