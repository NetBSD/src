/*	$NetBSD: ypresp_xdr.c,v 1.1.1.1 1996/08/09 10:14:52 thorpej Exp $	*/

/*
 * This file was originally generated with rpcgen, then modified
 * by Jason R. Thorpe <thorpej@NetBSD.ORG>. 
 */

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
