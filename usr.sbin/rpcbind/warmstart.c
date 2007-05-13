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
/*
 * warmstart.c
 * Allows for gathering of registrations from a earlier dumped file.
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ident	"@(#)warmstart.c	1.7	93/07/05 SMI"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <err.h>
#include <rpc/rpc.h>
#include <rpc/rpcb_prot.h>
#include <rpc/xdr.h>
#ifdef PORTMAP
#include <netinet/in.h>
#include <rpc/pmap_prot.h>
#endif
#include <syslog.h>
#include <unistd.h>

#include "rpcbind.h"

/*
 * XXX this code is unsafe and is not used. It should be made safe.
 */


/* These files keep the pmap_list and rpcb_list in XDR format */
#define	RPCBFILE	"/tmp/rpcbind.file"
#ifdef PORTMAP
#define	PMAPFILE	"/tmp/portmap.file"
#endif

static bool_t write_struct(const char *, xdrproc_t, void *);
static bool_t read_struct(const char *, xdrproc_t, void *);

static bool_t
write_struct(const char *filename, xdrproc_t structproc, void *list)
{
	FILE *fp;
	int fd;
	XDR xdrs;

	fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd == -1 || (fp = fdopen(fd, "r+")) == NULL) {
		syslog(LOG_ERR, "Cannot open `%s' (%m)", filename);
		syslog(LOG_ERR, "Cannot save any registration");
		return FALSE;
	}

	xdrstdio_create(&xdrs, fp, XDR_ENCODE);

	if (structproc(&xdrs, list) == FALSE) {
		syslog(LOG_ERR, "xdr_%s: failed", filename);
		(void)fclose(fp);
		return (FALSE);
	}
	XDR_DESTROY(&xdrs);
	(void)fclose(fp);
	return TRUE;
}

static bool_t
read_struct(const char *filename, xdrproc_t structproc, void *list)
{
	FILE *fp;
	XDR xdrs;
	struct stat sbuf;

	if (stat(filename, &sbuf) != 0) {
		warn("Cannot stat `%s'", filename);
		goto error;
	}
	if ((sbuf.st_uid != 0) || (sbuf.st_mode & S_IRWXG) ||
	    (sbuf.st_mode & S_IRWXO)) {
		warnx("Invalid permissions on `%s'", filename);
		goto error;
	}
	fp = fopen(filename, "r");
	if (fp == NULL) {
		warn("cannot open `%s'", filename);
		goto error;
	}
	xdrstdio_create(&xdrs, fp, XDR_DECODE);

	if (structproc(&xdrs, list) == FALSE) {
		warnx("xdr_%s failed", filename);
		(void)fclose(fp);
		goto error;
	}
	XDR_DESTROY(&xdrs);
	(void)fclose(fp);
	return TRUE;

error:	warnx("Will start from scratch");
	return FALSE;
}

void
write_warmstart(void)
{
	(void)write_struct(RPCBFILE, xdr_rpcblist_ptr, &list_rbl);
#ifdef PORTMAP
	(void)write_struct(PMAPFILE, xdr_pmaplist_ptr, &list_pml);
#endif

}

void
read_warmstart(void)
{
	rpcblist_ptr tmp_rpcbl = NULL;
#ifdef PORTMAP
	struct pmaplist *tmp_pmapl = NULL;
#endif
	int ok1, ok2 = TRUE;

	ok1 = read_struct(RPCBFILE, xdr_rpcblist_ptr, &tmp_rpcbl);
	if (ok1 == FALSE)
		return;
#ifdef PORTMAP
	ok2 = read_struct(PMAPFILE, xdr_pmaplist_ptr, &tmp_pmapl);
#endif
	if (ok2 == FALSE) {
		xdr_free((xdrproc_t) xdr_rpcblist_ptr, (char *)&tmp_rpcbl);
		return;
	}
	xdr_free((xdrproc_t) xdr_rpcblist_ptr, (char *)&list_rbl);
	list_rbl = tmp_rpcbl;
#ifdef PORTMAP
	xdr_free((xdrproc_t) xdr_pmaplist_ptr, (char *)&list_pml);
	list_pml = tmp_pmapl;
#endif
}
