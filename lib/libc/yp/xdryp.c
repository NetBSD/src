/*	$NetBSD: xdryp.c,v 1.16 1997/07/13 20:28:10 christos Exp $	*/

/*
 * Copyright (c) 1996 Jason R. Thorpe <thorpej@NetBSD.ORG>.
 * All rights reserved.
 *
 * Copyright (c) 1992, 1993 Theo de Raadt <deraadt@fsa.ca>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Theo de Raadt.
 *	This product includes software developed for the NetBSD Project
 *	by Jason R. Thorpe.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: xdryp.c,v 1.16 1997/07/13 20:28:10 christos Exp $");
#endif

/*
 * XDR routines used by the YP protocol.  Note that these routines do
 * not strictly conform to the RPC definition in yp.x.  This file
 * replicates the functions exported by the Sun YP API; reality is
 * often inaccurate.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

/*
 * Functions used only within this file.
 */
static	bool_t xdr_ypbind_binding __P((XDR *, struct ypbind_binding *));
static	bool_t xdr_ypbind_resptype __P((XDR *, enum ypbind_resptype *));
static	bool_t xdr_ypstat __P((XDR *, enum ypbind_resptype *));
static	bool_t xdr_ypmaplist_str __P((XDR *, char *));

__warn_references(xdr_domainname,
    "warning: this program uses xdr_domainname(), which is deprecated and buggy.");

bool_t
xdr_domainname(xdrs, objp)
	XDR *xdrs;
	char *objp;
{
	return xdr_string(xdrs, &objp, YPMAXDOMAIN);
}

__warn_references(xdr_peername,
    "warning: this program uses xdr_peername(), which is deprecated and buggy.");

bool_t
xdr_peername(xdrs, objp)
	XDR *xdrs;
	char *objp;
{
	return xdr_string(xdrs, &objp, YPMAXPEER);
}

__warn_references(xdr_mapname,
    "warning: this program uses xdr_mapname(), which is deprecated and buggy.");

bool_t
xdr_mapname(xdrs, objp)
	XDR *xdrs;
	char *objp;
{
	return xdr_string(xdrs, &objp, YPMAXMAP);
}

bool_t
xdr_ypdomain_wrap_string(xdrs, objp)
	XDR *xdrs;
	char **objp;
{
	return xdr_string(xdrs, objp, YPMAXDOMAIN);
}

bool_t
xdr_ypmap_wrap_string(xdrs, objp)
	XDR *xdrs;
	char **objp;
{
	return xdr_string(xdrs, objp, YPMAXMAP);
}

bool_t
xdr_ypowner_wrap_string(xdrs, objp)
	XDR *xdrs;
	char **objp;
{
	return xdr_string(xdrs, objp, YPMAXPEER);
}

bool_t
xdr_datum(xdrs, objp)
	XDR *xdrs;
	datum *objp;
{
	return xdr_bytes(xdrs, (char **)&objp->dptr,
	    (u_int *)&objp->dsize, YPMAXRECORD);
}

bool_t
xdr_ypreq_key(xdrs, objp)
	XDR *xdrs;
	struct ypreq_key *objp;
{
	if (!xdr_ypdomain_wrap_string(xdrs, (char **)&objp->domain))
		return FALSE;

	if (!xdr_ypmap_wrap_string(xdrs, (char **)&objp->map))
		return FALSE;

	if (!xdr_datum(xdrs, &objp->keydat))
		return FALSE;

	return TRUE;
}

bool_t
xdr_ypreq_nokey(xdrs, objp)
	XDR *xdrs;
	struct ypreq_nokey *objp;
{
	if (!xdr_ypdomain_wrap_string(xdrs, (char **)&objp->domain))
		return FALSE;

	if (!xdr_ypmap_wrap_string(xdrs, (char **)&objp->map))
		return FALSE;

	return TRUE;
}

bool_t
xdr_yp_inaddr(xdrs, objp)
	XDR *xdrs;
	struct in_addr *objp;
{
	return xdr_opaque(xdrs, (caddr_t)&objp->s_addr, sizeof objp->s_addr);
}

static bool_t
xdr_ypbind_binding(xdrs, objp)
	XDR *xdrs;
	struct ypbind_binding *objp;
{
	if (!xdr_yp_inaddr(xdrs, &objp->ypbind_binding_addr))
		return FALSE;

	if (!xdr_opaque(xdrs, (void *)&objp->ypbind_binding_port,
	    sizeof objp->ypbind_binding_port))
		return FALSE;

	return TRUE;
}

static bool_t
xdr_ypbind_resptype(xdrs, objp)
	XDR *xdrs;
	enum ypbind_resptype *objp;
{
	return xdr_enum(xdrs, (enum_t *)objp);
}

static bool_t
xdr_ypstat(xdrs, objp)
	XDR *xdrs;
	enum ypbind_resptype *objp;
{
	return xdr_enum(xdrs, (enum_t *)objp);
}

bool_t
xdr_ypbind_resp(xdrs, objp)
	XDR *xdrs;
	struct ypbind_resp *objp;
{
	if (!xdr_ypbind_resptype(xdrs, &objp->ypbind_status))
		return FALSE;

	switch (objp->ypbind_status) {
	case YPBIND_FAIL_VAL:
		return xdr_u_int(xdrs,
		    (u_int *)&objp->ypbind_respbody.ypbind_error);

	case YPBIND_SUCC_VAL:
		return xdr_ypbind_binding(xdrs,
		    &objp->ypbind_respbody.ypbind_bindinfo);

	default:
		return FALSE;
	}
	/* NOTREACHED */
}

bool_t
xdr_ypresp_val(xdrs, objp)
	XDR *xdrs;
	struct ypresp_val *objp;
{
	if (!xdr_ypstat(xdrs, (enum ypbind_resptype *)&objp->status))
		return FALSE;

	if (!xdr_datum(xdrs, &objp->valdat))
		return FALSE;

	return TRUE;
}

bool_t
xdr_ypbind_setdom(xdrs, objp)
	XDR *xdrs;
	struct ypbind_setdom *objp;
{
	char *cp = objp->ypsetdom_domain;

	if (!xdr_ypdomain_wrap_string(xdrs, &cp))
		return FALSE;

	if (!xdr_ypbind_binding(xdrs, &objp->ypsetdom_binding))
		return FALSE;

	if (!xdr_u_int(xdrs, &objp->ypsetdom_vers))
		return FALSE;

	return TRUE;
}

bool_t
xdr_ypresp_key_val(xdrs, objp)
	XDR *xdrs;
	struct ypresp_key_val *objp;
{
	if (!xdr_ypstat(xdrs, (enum ypbind_resptype *)&objp->status))
		return FALSE;

	if (!xdr_datum(xdrs, &objp->valdat))
		return FALSE;

	if (!xdr_datum(xdrs, &objp->keydat))
		return FALSE;

	return TRUE;
}

bool_t
xdr_ypall(xdrs, incallback)
	XDR *xdrs;
	struct ypall_callback *incallback;
{
	struct ypresp_key_val out;
	char key[YPMAXRECORD], val[YPMAXRECORD];
	bool_t more, status;

	/*
	 * Set up key/val struct to be used during the transaction.
	 */
	memset(&out, 0, sizeof out);
	out.keydat.dptr = key;
	out.keydat.dsize = sizeof(key);
	out.valdat.dptr = val;
	out.valdat.dsize = sizeof(val);

	for (;;) {
		/* Values pending? */
		if (!xdr_bool(xdrs, &more))
			return FALSE;		/* can't tell! */
		if (!more)
			return TRUE;		/* no more */

		/* Transfer key/value pair. */
		status = xdr_ypresp_key_val(xdrs, &out);

		/*
		 * If we succeeded, call the callback function.
		 * The callback will return TRUE when it wants
		 * no more values.  If we fail, indicate the
		 * error.
		 */
		if (status) {
			if ((*incallback->foreach)(out.status,
			    (char *)out.keydat.dptr, out.keydat.dsize,
			    (char *)out.valdat.dptr, out.valdat.dsize,
			    incallback->data))
				return TRUE;
		} else
			return FALSE;
	}
}

bool_t
xdr_ypresp_master(xdrs, objp)
	XDR *xdrs;
	struct ypresp_master *objp;
{
	if (!xdr_ypstat(xdrs, (enum ypbind_resptype *)&objp->status))
		return FALSE;

	if (!xdr_string(xdrs, &objp->master, YPMAXPEER))
		return FALSE;

	return TRUE;
}

static bool_t
xdr_ypmaplist_str(xdrs, objp)
	XDR *xdrs;
	char *objp;
{
	return xdr_string(xdrs, &objp, YPMAXMAP+1);
}

bool_t
xdr_ypmaplist(xdrs, objp)
	XDR *xdrs;
	struct ypmaplist *objp;
{
	if (!xdr_ypmaplist_str(xdrs, objp->ypml_name))
		return FALSE;

	if (!xdr_pointer(xdrs, (caddr_t *)&objp->ypml_next,
	    sizeof(struct ypmaplist), xdr_ypmaplist))
		return FALSE;

	return TRUE;
}

bool_t
xdr_ypresp_maplist(xdrs, objp)
	XDR *xdrs;
	struct ypresp_maplist *objp;
{
	if (!xdr_ypstat(xdrs, (enum ypbind_resptype *)&objp->status))
		return FALSE;

	if (!xdr_pointer(xdrs, (caddr_t *)&objp->list,
	    sizeof(struct ypmaplist), xdr_ypmaplist))
		return FALSE;

	return TRUE;
}

bool_t
xdr_ypresp_order(xdrs, objp)
	XDR *xdrs;
	struct ypresp_order *objp;
{
	if (!xdr_ypstat(xdrs, (enum ypbind_resptype *)&objp->status))
		return FALSE;

	if (!xdr_u_int(xdrs, &objp->ordernum))
		return FALSE;

	return TRUE;
}

bool_t
xdr_ypreq_xfr(xdrs, objp)
	XDR *xdrs;
	struct ypreq_xfr *objp;
{
	if (!xdr_ypmap_parms(xdrs, &objp->map_parms))
		return FALSE;

	if (!xdr_u_int(xdrs, &objp->transid))
		return FALSE;

	if (!xdr_u_int(xdrs, &objp->proto))
		return FALSE;

	if (!xdr_u_int(xdrs, &objp->port))
		return FALSE;

	return TRUE;
}

bool_t
xdr_ypmap_parms(xdrs, objp)
	XDR *xdrs;
	struct ypmap_parms *objp;
{
	if (!xdr_ypdomain_wrap_string(xdrs, (char **)&objp->domain))
		return FALSE;

	if (!xdr_ypmap_wrap_string(xdrs, (char **)&objp->map))
		return FALSE;

	if (!xdr_u_int(xdrs, &objp->ordernum))
		return FALSE;

	if (!xdr_ypowner_wrap_string(xdrs, &objp->owner))
		return FALSE;

	return TRUE;
}

bool_t
xdr_yppushresp_xfr(xdrs, objp)
	XDR *xdrs;
	struct yppushresp_xfr *objp;
{
	if (!xdr_u_int(xdrs, &objp->transid))
		return FALSE;

	if (!xdr_enum(xdrs, (enum_t *)&objp->status))
		return FALSE;

	return TRUE;
}
