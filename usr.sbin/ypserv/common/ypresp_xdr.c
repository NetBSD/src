/*	$NetBSD: ypresp_xdr.c,v 1.3 2002/07/06 21:39:26 wiz Exp $	*/

/*
 * This file was originally generated with rpcgen, then modified
 * by Jason R. Thorpe <thorpej@NetBSD.ORG>. 
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ypresp_xdr.c,v 1.3 2002/07/06 21:39:26 wiz Exp $");
#endif

#include <sys/types.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>

#include "ypdef.h"

bool_t
xdr_ypresp_xfr(XDR *xdrs, struct ypresp_xfr *objp)
{

	if (!xdr_u_int(xdrs, &objp->transid))
		return FALSE;

	if (!xdr_enum(xdrs, (enum_t *)&objp->xfrstat))
		return FALSE;

	return TRUE;
}
