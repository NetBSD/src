/*	$NetBSD: ypresp_xdr.c,v 1.2 1997/10/13 03:42:33 lukem Exp $	*/

/*
 * This file was originally generated with rpcgen, then modified
 * by Jason R. Thorpe <thorpej@NetBSD.ORG>. 
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ypresp_xdr.c,v 1.2 1997/10/13 03:42:33 lukem Exp $");
#endif

#include <sys/types.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>

#include "ypdef.h"

bool_t
xdr_ypresp_xfr(xdrs, objp)
	XDR *xdrs;
	struct ypresp_xfr *objp;
{

	if (!xdr_u_int(xdrs, &objp->transid))
		return FALSE;

	if (!xdr_enum(xdrs, (enum_t *)&objp->xfrstat))
		return FALSE;

	return TRUE;
}
