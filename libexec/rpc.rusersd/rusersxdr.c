/*
 * This is a *hand* crafted XDR library for the version 2 rusers interface.
 */
/* derived from rnusers.x */
#include <sys/types.h>
#include <utmp.h>
#include <rpc/rpc.h>
#include <rpcsvc/rnusers.h>	/* Old version */

#ifndef lint
/*static char sccsid[] = "from: @(#)rnusers.x 1.2 87/09/20 Copyr 1987 Sun Micro";*/
/*static char sccsid[] = "from: @(#)rnusers.x	2.1 88/08/01 4.0 RPCSRC";*/
static char rcsid[] = "rnusers.x,v 1.1 1993/10/08 05:26:57 cgd Exp";
#endif /* not lint */

bool_t
xdr_utmp(xdrs, objp)
	XDR *xdrs;
	struct ru_utmp *objp;
{
	char *ptr;
	int size;

	ptr  = objp->ut_line;
	size = sizeof(objp->ut_line);
	if (!xdr_bytes(xdrs, &ptr, &size, size)) {
		return (FALSE);
	}
	ptr  = objp->ut_name;
	size = sizeof(objp->ut_line);
	if (!xdr_bytes(xdrs, &ptr, &size, size)) {
		return (FALSE);
	}
	ptr  = objp->ut_host;
	size = sizeof(objp->ut_host);
	if (!xdr_bytes(xdrs, &ptr, &size, size)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->ut_time)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_utmpidle(xdrs, objp)
	XDR *xdrs;
	struct utmpidle *objp;
{
	if (!xdr_utmp(xdrs, &objp->ui_utmp)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->ui_idle)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_utmpidleptr(xdrs, objpp)
	XDR *xdrs;
	struct utmpidle **objpp;
{
	if (!xdr_reference(xdrs, (char **) objpp, sizeof (struct utmpidle), 
			   xdr_utmpidle)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_utmpidlearr(xdrs, objp)
	XDR *xdrs;
	struct utmpidlearr *objp;
{
	if (!xdr_array(xdrs, (char **)&objp->uia_arr, (u_int *)&objp->uia_cnt,
		       MAXUSERS, sizeof(struct utmpidle *), xdr_utmpidleptr)) {
		return (FALSE);
	}
	return (TRUE);
}
