/*	$NetBSD: yp_first.c,v 1.5 1997/05/21 06:55:25 lukem Exp $	 */

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

#if defined(LIBC_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: yp_first.c,v 1.5 1997/05/21 06:55:25 lukem Exp $";
#endif

#include <stdlib.h>
#include <string.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

extern struct timeval _yplib_timeout;
extern int _yplib_nerrs;

int
yp_first(indomain, inmap, outkey, outkeylen, outval, outvallen)
	const char     *indomain;
	const char     *inmap;
	char          **outkey;
	int            *outkeylen;
	char          **outval;
	int            *outvallen;
{
	struct ypresp_key_val yprkv;
	struct ypreq_nokey yprnk;
	struct dom_binding *ysd;
	int r, nerrs = 0;

	if (outkey == NULL || outval == NULL)
		return YPERR_BADARGS;
	*outkey = *outval = NULL;
	*outkeylen = *outvallen = 0;
	if (indomain == NULL || *indomain == '\0'
	    || strlen(indomain) > YPMAXDOMAIN)
		return YPERR_BADARGS;
	if (inmap == NULL || *inmap == '\0'
	    || strlen(inmap) > YPMAXMAP)
		return YPERR_BADARGS;

again:
	if (_yp_dobind(indomain, &ysd) != 0)
		return YPERR_DOMAIN;

	yprnk.domain = indomain;
	yprnk.map = inmap;
	(void)memset(&yprkv, 0, sizeof yprkv);

	r = clnt_call(ysd->dom_client, YPPROC_FIRST,
		   xdr_ypreq_nokey, &yprnk, xdr_ypresp_key_val, &yprkv, 
		   _yplib_timeout);
	if (r != RPC_SUCCESS) {
		if (++nerrs == _yplib_nerrs) {
			clnt_perror(ysd->dom_client, "yp_first: clnt_call");
			nerrs = 0;
		}
		ysd->dom_vers = -1;
		goto again;
	}
	if (!(r = ypprot_err(yprkv.status))) {
		*outkeylen = yprkv.keydat.dsize;
		if ((*outkey = malloc(*outkeylen + 1)) == NULL)
			r = YPERR_RESRC;
		else {
			(void)memcpy(*outkey, yprkv.keydat.dptr, *outkeylen);
			(*outkey)[*outkeylen] = '\0';
		}
		*outvallen = yprkv.valdat.dsize;
		if ((*outval = malloc(*outvallen + 1)) == NULL)
			r = YPERR_RESRC;
		else {
			(void)memcpy(*outval, yprkv.valdat.dptr, *outvallen);
			(*outval)[*outvallen] = '\0';
		}
	}
	xdr_free(xdr_ypresp_key_val, (char *) &yprkv);
	_yp_unbind(ysd);
	if (r != 0) {
		if (*outkey) {
			free(*outkey);
			*outkey = NULL;
		}
		if (*outval) {
			free(*outval);
			*outval = NULL;
		}
	}
	return r;
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
	struct ypresp_key_val yprkv;
	struct ypreq_key yprk;
	struct dom_binding *ysd;
	int r, nerrs = 0;

	if (outkey == NULL || outval == NULL)
		return YPERR_BADARGS;
	*outkey = *outval = NULL;
	*outkeylen = *outvallen = 0;

	if (indomain == NULL || *indomain == '\0'
	    || strlen(indomain) > YPMAXDOMAIN)
		return YPERR_BADARGS;
	if (inmap == NULL || *inmap == '\0'
	    || strlen(inmap) > YPMAXMAP)
		return YPERR_BADARGS;

again:
	if (_yp_dobind(indomain, &ysd) != 0)
		return YPERR_DOMAIN;

	yprk.domain = indomain;
	yprk.map = inmap;
	yprk.keydat.dptr = inkey;
	yprk.keydat.dsize = inkeylen;
	(void)memset(&yprkv, 0, sizeof yprkv);

	r = clnt_call(ysd->dom_client, YPPROC_NEXT,
		      xdr_ypreq_key, &yprk, xdr_ypresp_key_val, &yprkv, 
		      _yplib_timeout);
	if (r != RPC_SUCCESS) {
		if (++nerrs == _yplib_nerrs) {
			clnt_perror(ysd->dom_client, "yp_next: clnt_call");
			nerrs = 0;
		}
		ysd->dom_vers = -1;
		goto again;
	}
	if (!(r = ypprot_err(yprkv.status))) {
		*outkeylen = yprkv.keydat.dsize;
		if ((*outkey = malloc(*outkeylen + 1)) == NULL)
			r = YPERR_RESRC;
		else {
			(void)memcpy(*outkey, yprkv.keydat.dptr, *outkeylen);
			(*outkey)[*outkeylen] = '\0';
		}
		*outvallen = yprkv.valdat.dsize;
		if ((*outval = malloc(*outvallen + 1)) == NULL)
			r = YPERR_RESRC;
		else {
			(void)memcpy(*outval, yprkv.valdat.dptr, *outvallen);
			(*outval)[*outvallen] = '\0';
		}
	}
	xdr_free(xdr_ypresp_key_val, (char *) &yprkv);
	_yp_unbind(ysd);
	if (r != 0) {
		if (*outkey) {
			free(*outkey);
			*outkey = NULL;
		}
		if (*outval) {
			free(*outval);
			*outval = NULL;
		}
	}
	return r;
}
