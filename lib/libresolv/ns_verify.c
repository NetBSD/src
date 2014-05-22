/*	$NetBSD: ns_verify.c,v 1.2.4.3 2014/05/22 11:36:59 yamt Exp $	*/

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1999 by Internet Software Consortium, Inc.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__RCSID("Id: ns_verify.c,v 1.5 2006/03/09 23:57:56 marka Exp ");

/* Import. */

#include "port_before.h"
#include "fd_setsize.h"

#include <sys/types.h>
#include <sys/param.h>

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>

#include <errno.h>
#include <netdb.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <isc/dst.h>

#include "port_after.h"

/* Private. */

#define BOUNDS_CHECK(ptr, count) \
	do { \
		if ((ptr) + (count) > eom) { \
			return (NS_TSIG_ERROR_FORMERR); \
		} \
	} while (/*CONSTCOND*/0)

/* Public. */

u_char *
ns_find_tsig(u_char *msg, u_char *eom) {
	HEADER *hp = (void *)msg;
	int n, type;
	u_char *cp = msg, *start;

	if (msg == NULL || eom == NULL || msg > eom)
		return (NULL);

	if (cp + HFIXEDSZ >= eom)
		return (NULL);

	if (hp->arcount == 0)
		return (NULL);

	cp += HFIXEDSZ;

	n = ns_skiprr(cp, eom, ns_s_qd, ntohs(hp->qdcount));
	if (n < 0)
		return (NULL);
	cp += n;

	n = ns_skiprr(cp, eom, ns_s_an, ntohs(hp->ancount));
	if (n < 0)
		return (NULL);
	cp += n;

	n = ns_skiprr(cp, eom, ns_s_ns, ntohs(hp->nscount));
	if (n < 0)
		return (NULL);
	cp += n;

	n = ns_skiprr(cp, eom, ns_s_ar, ntohs(hp->arcount) - 1);
	if (n < 0)
		return (NULL);
	cp += n;

	start = cp;
	n = dn_skipname(cp, eom);
	if (n < 0)
		return (NULL);
	cp += n;
	if (cp + INT16SZ >= eom)
		return (NULL);

	GETSHORT(type, cp);
	if (type != ns_t_tsig)
		return (NULL);
	return (start);
}

/* ns_verify
 *
 * Parameters:
 *\li	statp		res stuff
 *\li	msg		received message
 *\li	msglen		length of message
 *\li	key		tsig key used for verifying.
 *\li	querysig	(response), the signature in the query
 *\li	querysiglen	(response), the length of the signature in the query
 *\li	sig		(query), a buffer to hold the signature
 *\li	siglen		(query), input - length of signature buffer
 *				 output - length of signature
 *
 * Errors:
 *\li	- bad input (-1)
 *\li	- invalid dns message (NS_TSIG_ERROR_FORMERR)
 *\li	- TSIG is not present (NS_TSIG_ERROR_NO_TSIG)
 *\li	- key doesn't match (-ns_r_badkey)
 *\li	- TSIG verification fails with BADKEY (-ns_r_badkey)
 *\li	- TSIG verification fails with BADSIG (-ns_r_badsig)
 *\li	- TSIG verification fails with BADTIME (-ns_r_badtime)
 *\li	- TSIG verification succeeds, error set to BAKEY (ns_r_badkey)
 *\li	- TSIG verification succeeds, error set to BADSIG (ns_r_badsig)
 *\li	- TSIG verification succeeds, error set to BADTIME (ns_r_badtime)
 */
int
ns_verify(u_char *msg, int *msglen, void *k,
	  const u_char *querysig, int querysiglen, u_char *sig, int *siglen,
	  time_t *timesigned, int nostrip)
{
	HEADER *hp = (void *)msg;
	DST_KEY *key = (DST_KEY *)k;
	u_char *cp = msg, *eom;
	char name[MAXDNAME], alg[MAXDNAME];
	u_char *recstart, *rdatastart;
	u_char *sigstart, *otherstart;
	int n;
	int error;
	u_int16_t type, length;
	u_int16_t fudge, sigfieldlen, otherfieldlen;

	dst_init();
	if (msg == NULL || msglen == NULL || *msglen < 0)
		return (-1);

	eom = msg + *msglen;

	recstart = ns_find_tsig(msg, eom);
	if (recstart == NULL)
		return (NS_TSIG_ERROR_NO_TSIG);

	cp = recstart;

	/* Read the key name. */
	n = dn_expand(msg, eom, cp, name, MAXDNAME);
	if (n < 0)
		return (NS_TSIG_ERROR_FORMERR);
	cp += n;

	/* Read the type. */
	BOUNDS_CHECK(cp, 2*INT16SZ + INT32SZ + INT16SZ);
	GETSHORT(type, cp);
	if (type != ns_t_tsig)
		return (NS_TSIG_ERROR_NO_TSIG);

	/* Skip the class and TTL, save the length. */
	cp += INT16SZ + INT32SZ;
	GETSHORT(length, cp);
	if (eom - cp != length)
		return (NS_TSIG_ERROR_FORMERR);

	/* Read the algorithm name. */
	rdatastart = cp;
	n = dn_expand(msg, eom, cp, alg, MAXDNAME);
	if (n < 0)
		return (NS_TSIG_ERROR_FORMERR);
	if (ns_samename(alg, NS_TSIG_ALG_HMAC_MD5) != 1)
		return (-ns_r_badkey);
	cp += n;

	/* Read the time signed and fudge. */
	BOUNDS_CHECK(cp, INT16SZ + INT32SZ + INT16SZ);
	cp += INT16SZ;
	GETLONG((*timesigned), cp);
	GETSHORT(fudge, cp);

	/* Read the signature. */
	BOUNDS_CHECK(cp, INT16SZ);
	GETSHORT(sigfieldlen, cp);
	BOUNDS_CHECK(cp, sigfieldlen);
	sigstart = cp;
	cp += sigfieldlen;

	/* Skip id and read error. */
	BOUNDS_CHECK(cp, 2*INT16SZ);
	cp += INT16SZ;
	GETSHORT(error, cp);

	/* Parse the other data. */
	BOUNDS_CHECK(cp, INT16SZ);
	GETSHORT(otherfieldlen, cp);
	BOUNDS_CHECK(cp, otherfieldlen);
	otherstart = cp;
	cp += otherfieldlen;

	if (cp != eom)
		return (NS_TSIG_ERROR_FORMERR);

	/* Verify that the key used is OK. */
	if (key != NULL) {
		if (key->dk_alg != KEY_HMAC_MD5)
			return (-ns_r_badkey);
		if (error != ns_r_badsig && error != ns_r_badkey) {
			if (ns_samename(key->dk_key_name, name) != 1)
				return (-ns_r_badkey);
		}
	}

	hp->arcount = htons(ntohs(hp->arcount) - 1);

	/*
	 * Do the verification.
	 */

	if (key != NULL && error != ns_r_badsig && error != ns_r_badkey) {
		void *ctx;
		u_char buf[MAXDNAME];
		u_char buf2[MAXDNAME];

		/* Digest the query signature, if this is a response. */
		dst_verify_data(SIG_MODE_INIT, key, &ctx, NULL, 0, NULL, 0);
		if (querysiglen > 0 && querysig != NULL) {
			u_int16_t len_n = htons(querysiglen);
			dst_verify_data(SIG_MODE_UPDATE, key, &ctx,
					(void *)&len_n, INT16SZ, NULL, 0);
			dst_verify_data(SIG_MODE_UPDATE, key, &ctx,
					querysig, querysiglen, NULL, 0);
		}
		
 		/* Digest the message. */
		dst_verify_data(SIG_MODE_UPDATE, key, &ctx, msg,
		    (int)(recstart - msg), NULL, 0);

		/* Digest the key name. */
		n = ns_name_pton(name, buf2, sizeof(buf2));
		if (n < 0)
			return (-1);
		n = ns_name_ntol(buf2, buf, sizeof(buf));
		if (n < 0)
			return (-1);
		dst_verify_data(SIG_MODE_UPDATE, key, &ctx, buf, n, NULL, 0);

		/* Digest the class and TTL. */
		dst_verify_data(SIG_MODE_UPDATE, key, &ctx,
				recstart + dn_skipname(recstart, eom) + INT16SZ,
				INT16SZ + INT32SZ, NULL, 0);

		/* Digest the algorithm. */
		n = ns_name_pton(alg, buf2, sizeof(buf2));
		if (n < 0)
			return (-1);
		n = ns_name_ntol(buf2, buf, sizeof(buf));
		if (n < 0)
			return (-1);
		dst_verify_data(SIG_MODE_UPDATE, key, &ctx, buf, n, NULL, 0);

		/* Digest the time signed and fudge. */
		dst_verify_data(SIG_MODE_UPDATE, key, &ctx,
				rdatastart + dn_skipname(rdatastart, eom),
				INT16SZ + INT32SZ + INT16SZ, NULL, 0);

		/* Digest the error and other data. */
		dst_verify_data(SIG_MODE_UPDATE, key, &ctx,
				otherstart - INT16SZ - INT16SZ,
				otherfieldlen + INT16SZ + INT16SZ, NULL, 0);

		n = dst_verify_data(SIG_MODE_FINAL, key, &ctx, NULL, 0,
				    sigstart, sigfieldlen);

		if (n < 0)
			return (-ns_r_badsig);

		if (sig != NULL && siglen != NULL) {
			if (*siglen < sigfieldlen)
				return (NS_TSIG_ERROR_NO_SPACE);
			memcpy(sig, sigstart, sigfieldlen);
			*siglen = sigfieldlen;
		}
	} else {
		if (sigfieldlen > 0)
			return (NS_TSIG_ERROR_FORMERR);
		if (sig != NULL && siglen != NULL)
			*siglen = 0;
	}

	/* Reset the counter, since we still need to check for badtime. */
	hp->arcount = htons(ntohs(hp->arcount) + 1);

	/* Verify the time. */
	if (abs((int)((*timesigned) - time(NULL))) > fudge)
		return (-ns_r_badtime);

	if (nostrip == 0) {
		*msglen = (int)(recstart - msg);
		hp->arcount = htons(ntohs(hp->arcount) - 1);
	}

	if (error != NOERROR)
		return (error);

	return (0);
}

int
ns_verify_tcp_init(void *k, const u_char *querysig, int querysiglen,
		   ns_tcp_tsig_state *state)
{
	dst_init();
	if (state == NULL || k == NULL || querysig == NULL || querysiglen < 0)
		return (-1);
	state->counter = -1;
	state->key = k;
	if (state->key->dk_alg != KEY_HMAC_MD5)
		return (-ns_r_badkey);
	if (querysiglen > (int)sizeof(state->sig))
		return (-1);
	memcpy(state->sig, querysig, querysiglen);
	state->siglen = querysiglen;
	return (0);
}

int
ns_verify_tcp(u_char *msg, int *msglen, ns_tcp_tsig_state *state,
	      int required)
{
	HEADER *hp = (void *)msg;
	u_char *recstart, *sigstart;
	unsigned int sigfieldlen, otherfieldlen;
	u_char *cp, *eom, *cp2;
	char name[MAXDNAME], alg[MAXDNAME];
	u_char buf[MAXDNAME];
	int n, type, length, fudge, error;
	time_t timesigned;

	if (msg == NULL || msglen == NULL || state == NULL)
		return (-1);

	eom = msg + *msglen;

	state->counter++;
	if (state->counter == 0)
		return (ns_verify(msg, msglen, state->key,
				  state->sig, state->siglen,
				  state->sig, &state->siglen, &timesigned, 0));

	if (state->siglen > 0) {
		u_int16_t siglen_n = htons(state->siglen);

		dst_verify_data(SIG_MODE_INIT, state->key, &state->ctx,
				NULL, 0, NULL, 0);
		dst_verify_data(SIG_MODE_UPDATE, state->key, &state->ctx,
				(void *)&siglen_n, INT16SZ, NULL, 0);
		dst_verify_data(SIG_MODE_UPDATE, state->key, &state->ctx,
				state->sig, state->siglen, NULL, 0);
		state->siglen = 0;
	}

	cp = recstart = ns_find_tsig(msg, eom);

	if (recstart == NULL) {
		if (required)
			return (NS_TSIG_ERROR_NO_TSIG);
		dst_verify_data(SIG_MODE_UPDATE, state->key, &state->ctx,
				msg, *msglen, NULL, 0);
		return (0);
	}

	hp->arcount = htons(ntohs(hp->arcount) - 1);
	dst_verify_data(SIG_MODE_UPDATE, state->key, &state->ctx,
			msg, (int)(recstart - msg), NULL, 0);
	
	/* Read the key name. */
	n = dn_expand(msg, eom, cp, name, MAXDNAME);
	if (n < 0)
		return (NS_TSIG_ERROR_FORMERR);
	cp += n;

	/* Read the type. */
	BOUNDS_CHECK(cp, 2*INT16SZ + INT32SZ + INT16SZ);
	GETSHORT(type, cp);
	if (type != ns_t_tsig)
		return (NS_TSIG_ERROR_NO_TSIG);

	/* Skip the class and TTL, save the length. */
	cp += INT16SZ + INT32SZ;
	GETSHORT(length, cp);
	if (eom - cp != length)
		return (NS_TSIG_ERROR_FORMERR);

	/* Read the algorithm name. */
	n = dn_expand(msg, eom, cp, alg, MAXDNAME);
	if (n < 0)
		return (NS_TSIG_ERROR_FORMERR);
	if (ns_samename(alg, NS_TSIG_ALG_HMAC_MD5) != 1)
		return (-ns_r_badkey);
	cp += n;

	/* Verify that the key used is OK. */
	if ((ns_samename(state->key->dk_key_name, name) != 1 ||
	     state->key->dk_alg != KEY_HMAC_MD5))
		return (-ns_r_badkey);

	/* Read the time signed and fudge. */
	BOUNDS_CHECK(cp, INT16SZ + INT32SZ + INT16SZ);
	cp += INT16SZ;
	GETLONG(timesigned, cp);
	GETSHORT(fudge, cp);

	/* Read the signature. */
	BOUNDS_CHECK(cp, INT16SZ);
	GETSHORT(sigfieldlen, cp);
	BOUNDS_CHECK(cp, sigfieldlen);
	sigstart = cp;
	cp += sigfieldlen;

	/* Skip id and read error. */
	BOUNDS_CHECK(cp, 2*INT16SZ);
	cp += INT16SZ;
	GETSHORT(error, cp);

	/* Parse the other data. */
	BOUNDS_CHECK(cp, INT16SZ);
	GETSHORT(otherfieldlen, cp);
	BOUNDS_CHECK(cp, otherfieldlen);
	cp += otherfieldlen;

	if (cp != eom)
		return (NS_TSIG_ERROR_FORMERR);

	/*
	 * Do the verification.
	 */

	/* Digest the time signed and fudge. */
	cp2 = buf;
	PUTSHORT(0, cp2);       /*%< Top 16 bits of time. */
	PUTLONG(timesigned, cp2);
	PUTSHORT(NS_TSIG_FUDGE, cp2);

	dst_verify_data(SIG_MODE_UPDATE, state->key, &state->ctx,
			buf, (int)(cp2 - buf), NULL, 0);

	n = dst_verify_data(SIG_MODE_FINAL, state->key, &state->ctx, NULL, 0,
			    sigstart, (int)sigfieldlen);
	if (n < 0)
		return (-ns_r_badsig);

	if (sigfieldlen > sizeof(state->sig))
		return (NS_TSIG_ERROR_NO_SPACE);

	memcpy(state->sig, sigstart, sigfieldlen);
	state->siglen = sigfieldlen;

	/* Verify the time. */
	if (abs((int)(timesigned - time(NULL))) > fudge)
		return (-ns_r_badtime);

	*msglen = (int)(recstart - msg);

	if (error != NOERROR)
		return (error);

	return (0);
}

/*! \file */
