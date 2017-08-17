/*	$NetBSD: warmstart.c,v 1.8 2017/08/17 00:08:10 ginsbach Exp $	*/
/* $FreeBSD: head/usr.sbin/rpcbind/warmstart.c 258564 2013-11-25 16:44:02Z hrs $*/

/*-
 * Copyright (c) 2009, Sun Microsystems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Sun Microsystems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * warmstart.c
 * Allows for gathering of registrations from an earlier dumped file.
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ident	"@(#)warmstart.c	1.7	93/07/05 SMI"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <err.h>
#include <paths.h>
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
#ifdef WARMSTART


/* These files keep the pmap_list and rpcb_list in XDR format */
#define	RPCBFILE	_PATH_VARRUN "rpcbind.file"
#ifdef PORTMAP
#define	PMAPFILE	_PATH_VARRUN "portmap.file"
#endif

static bool_t write_struct(const char *, xdrproc_t, void *);
static bool_t read_struct(const char *, xdrproc_t, void *);

static bool_t
write_struct(const char *filename, xdrproc_t structproc, void *list)
{
	FILE *fp;
	int fd;
	XDR xdrs;

	(void)unlink(filename);
	fd = open(filename, O_WRONLY|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
	if (fd == -1 || (fp = fdopen(fd, "w")) == NULL) {
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
	(void)write_struct(RPCBFILE, (xdrproc_t) xdr_rpcblist_ptr, &list_rbl);
#ifdef PORTMAP
	(void)write_struct(PMAPFILE, (xdrproc_t) xdr_pmaplist_ptr, &list_pml);
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

	ok1 = read_struct(RPCBFILE, (xdrproc_t) xdr_rpcblist_ptr, &tmp_rpcbl);
	if (ok1 == FALSE)
		return;
#ifdef PORTMAP
	ok2 = read_struct(PMAPFILE, (xdrproc_t) xdr_pmaplist_ptr, &tmp_pmapl);
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
#endif
