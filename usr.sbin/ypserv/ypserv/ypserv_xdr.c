/*	$NetBSD: ypserv_xdr.c,v 1.2 1997/10/15 05:01:52 lukem Exp $	*/

/*
 * This file was originally generated with rpcgen, then modified
 * by Jason R. Thorpe <thorpej@NetBSD.ORG>. 
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ypserv_xdr.c,v 1.2 1997/10/15 05:01:52 lukem Exp $");
#endif

#include <sys/types.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>

#include "ypserv.h"

bool_t
xdr_ypresp_all(xdrs, objp)
	XDR *xdrs;
	struct ypresp_all *objp;
{

	if (!xdr_bool(xdrs, &objp->more))
		return FALSE;

	switch (objp->more) {
	case TRUE:
		if (!xdr_ypresp_key_val(xdrs, &objp->ypresp_all_u.val))
			return FALSE;

		break;

	case FALSE:
		break;

	default:
		return FALSE;
	}

	return TRUE;
}
