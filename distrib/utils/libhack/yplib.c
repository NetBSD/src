/*	$NetBSD: yplib.c,v 1.3 1999/09/19 19:51:11 christos Exp $	*/

/*
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

/*
 * This file provides "stubs" for all the YP library functions.
 * It is not needed unless you pull in things that call YP, and
 * if you use all the get* files here then the YP stuff should
 * not get dragged in.  But if it does, one can use this.
 *
 * This was copied from:
 *      lib/libc/yp/yplib.c
 * (and then completely gutted! 8^)
 */
#include <sys/cdefs.h>

#ifdef __weak_alias
#define yp_all			_yp_all
#define yp_bind			_yp_bind
#define yp_first		_yp_first
#define yp_get_default_domain	_yp_get_default_domain
#define yp_maplist		_yp_maplist
#define yp_master		_yp_master
#define yp_match		_yp_match
#define yp_next			_yp_next
#define yp_order		_yp_order
#define yp_unbind		_yp_unbind
#define yperr_string		_yperr_string
#define ypprot_err		_ypprot_err
#endif

#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

struct dom_binding *_ypbindlist;
char _yp_domain[256];

#define YPLIB_TIMEOUT		10
#define YPLIB_RPC_RETRIES	4

struct timeval _yplib_timeout = { YPLIB_TIMEOUT, 0 };
struct timeval _yplib_rpc_timeout = { YPLIB_TIMEOUT / YPLIB_RPC_RETRIES,
	1000000 * (YPLIB_TIMEOUT % YPLIB_RPC_RETRIES) / YPLIB_RPC_RETRIES };
int _yplib_nerrs = 5;


#ifdef __weak_alias
__weak_alias(yp_all,_yp_all);
__weak_alias(yp_bind, _yp_bind);
__weak_alias(yp_first,_yp_first);
__weak_alias(yp_get_default_domain, _yp_get_default_domain);
__weak_alias(yp_maplist,_yp_maplist);
__weak_alias(yp_master,_yp_master);
__weak_alias(yp_match,_yp_match);
__weak_alias(yp_next,_yp_next);
__weak_alias(yp_order,_yp_order);
__weak_alias(yp_unbind, _yp_unbind);
__weak_alias(yperr_string,_yperr_string);
__weak_alias(ypprot_err,_ypprot_err);
#endif

void __yp_unbind __P((struct dom_binding *));
int _yp_invalid_domain __P((const char *));

int
_yp_dobind(dom, ypdb)
	const char *dom;
	struct dom_binding **ypdb;
{
	return YPERR_YPBIND;
}

void
__yp_unbind(ypb)
	struct dom_binding *ypb;
{
}

int
yp_bind(dom)
	const char     *dom;
{
	return _yp_dobind(dom, NULL);
}

void
yp_unbind(dom)
	const char     *dom;
{
}

int
yp_get_default_domain(domp)
	char **domp;
{
	*domp = NULL;
	if (_yp_domain[0] == '\0')
		if (getdomainname(_yp_domain, sizeof(_yp_domain)))
			return YPERR_NODOM;
	*domp = _yp_domain;
	return 0;
}

int
_yp_check(dom)
	char          **dom;
{
	char           *unused;

	if (_yp_domain[0] == '\0')
		if (yp_get_default_domain(&unused))
			return 0;

	if (dom)
		*dom = _yp_domain;

	if (yp_bind(_yp_domain) == 0)
		return 1;
	return 0;
}

int
_yp_invalid_domain(dom)
	const char *dom;
{
	if (dom == NULL || *dom == '\0')
		return 1;

	if (strlen(dom) > YPMAXDOMAIN)
		return 1;

	if (strchr(dom, '/') != NULL)
		return 1;

	return 0;
}

int
yp_match(indomain, inmap, inkey, inkeylen, outval, outvallen)
	const char     *indomain;
	const char     *inmap;
	const char     *inkey;
	int             inkeylen;
	char          **outval;
	int            *outvallen;
{
	*outval = NULL;
	*outvallen = 0;

	return YPERR_DOMAIN;
}

int
yp_first(indomain, inmap, outkey, outkeylen, outval, outvallen)
	const char     *indomain;
	const char     *inmap;
	char          **outkey;
	int            *outkeylen;
	char          **outval;
	int            *outvallen;
{

	*outkey = *outval = NULL;
	*outkeylen = *outvallen = 0;

	return YPERR_DOMAIN;
}

int
yp_next(indomain, inmap, inkey, inkeylen, outkey, outkeylen, outval, outvallen)
	const char     *indomain;
	const char     *inmap;
	const char     *inkey;
	int             inkeylen;
	char          **outkey;
	int            *outkeylen;
	char          **outval;
	int            *outvallen;
{
	*outkey = *outval = NULL;
	*outkeylen = *outvallen = 0;

	return YPERR_DOMAIN;
}

int
yp_all(indomain, inmap, incallback)
	const char     *indomain;
	const char     *inmap;
	struct ypall_callback *incallback;
{
	return YPERR_DOMAIN;
}

int
yp_order(indomain, inmap, outorder)
	const char     *indomain;
	const char     *inmap;
	int            *outorder;
{
	return YPERR_DOMAIN;
}

int
yp_master(indomain, inmap, outname)
	const char     *indomain;
	const char     *inmap;
	char          **outname;
{
	return YPERR_DOMAIN;
}

int
yp_maplist(indomain, outmaplist)
	const char     *indomain;
	struct ypmaplist **outmaplist;
{
	return YPERR_DOMAIN;
}

char *
yperr_string(incode)
	int             incode;
{
	static char     err[80];

	if (incode == 0)
		return "Success";

	sprintf(err, "YP FAKE error %d\n", incode);
	return err;
}

int
ypprot_err(incode)
	unsigned int    incode;
{
	switch (incode) {
	case YP_TRUE:	/* success */
		return 0;
	case YP_FALSE:	/* failure */
		return YPERR_YPBIND;
	}
	return YPERR_YPERR;
}

