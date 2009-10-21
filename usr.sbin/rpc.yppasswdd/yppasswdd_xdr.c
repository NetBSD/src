/*	$NetBSD: yppasswdd_xdr.c,v 1.3 2009/10/21 00:01:56 snj Exp $	*/

/*
 * Copyright (c) 1994 Mats O Jansson <moj@stacken.kth.se>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: yppasswdd_xdr.c,v 1.3 2009/10/21 00:01:56 snj Exp $");
#endif /* not lint */

#include <rpc/rpc.h>
#include <rpcsvc/yppasswd.h>

bool_t
xdr_x_passwd(XDR *xdrs, x_passwd *objp)
{

	if (!xdr_string(xdrs, &objp->pw_name, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, &objp->pw_passwd, ~0))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->pw_uid))
		return (FALSE);
	if (!xdr_int(xdrs, &objp->pw_gid))
		return (FALSE);
	if (!xdr_string(xdrs, &objp->pw_gecos, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, &objp->pw_dir, ~0))
		return (FALSE);
	if (!xdr_string(xdrs, &objp->pw_shell, ~0))
		return (FALSE);

	return (TRUE);
}

bool_t
xdr_yppasswd(XDR *xdrs, yppasswd *objp)
{

	if (!xdr_string(xdrs, &objp->oldpass, ~0))
		return (FALSE);
	if (!xdr_x_passwd(xdrs, &objp->newpw))
		return (FALSE);

	return (TRUE);
}
