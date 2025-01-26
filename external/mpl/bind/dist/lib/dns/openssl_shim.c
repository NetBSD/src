/*	$NetBSD: openssl_shim.c,v 1.3 2025/01/26 16:25:23 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include "openssl_shim.h"

#include <isc/util.h>

#if !HAVE_RSA_SET0_KEY && OPENSSL_VERSION_NUMBER < 0x30000000L
/* From OpenSSL 1.1.0 */
int
RSA_set0_key(RSA *r, BIGNUM *n, BIGNUM *e, BIGNUM *d) {
	/*
	 * If the fields n and e in r are NULL, the corresponding input
	 * parameters MUST be non-NULL for n and e.  d may be
	 * left NULL (in case only the public key is used).
	 */
	if ((r->n == NULL && n == NULL) || (r->e == NULL && e == NULL)) {
		return 0;
	}

	if (n != NULL) {
		BN_free(r->n);
		r->n = n;
	}
	if (e != NULL) {
		BN_free(r->e);
		r->e = e;
	}
	if (d != NULL) {
		BN_clear_free(r->d);
		r->d = d;
	}

	return 1;
}

int
RSA_set0_factors(RSA *r, BIGNUM *p, BIGNUM *q) {
	/*
	 * If the fields p and q in r are NULL, the corresponding input
	 * parameters MUST be non-NULL.
	 */
	if ((r->p == NULL && p == NULL) || (r->q == NULL && q == NULL)) {
		return 0;
	}

	if (p != NULL) {
		BN_clear_free(r->p);
		r->p = p;
	}
	if (q != NULL) {
		BN_clear_free(r->q);
		r->q = q;
	}

	return 1;
}

int
RSA_set0_crt_params(RSA *r, BIGNUM *dmp1, BIGNUM *dmq1, BIGNUM *iqmp) {
	/*
	 * If the fields dmp1, dmq1 and iqmp in r are NULL, the
	 * corresponding input parameters MUST be non-NULL.
	 */
	if ((r->dmp1 == NULL && dmp1 == NULL) ||
	    (r->dmq1 == NULL && dmq1 == NULL) ||
	    (r->iqmp == NULL && iqmp == NULL))
	{
		return 0;
	}

	if (dmp1 != NULL) {
		BN_clear_free(r->dmp1);
		r->dmp1 = dmp1;
	}
	if (dmq1 != NULL) {
		BN_clear_free(r->dmq1);
		r->dmq1 = dmq1;
	}
	if (iqmp != NULL) {
		BN_clear_free(r->iqmp);
		r->iqmp = iqmp;
	}

	return 1;
}

void
RSA_get0_key(const RSA *r, const BIGNUM **n, const BIGNUM **e,
	     const BIGNUM **d) {
	SET_IF_NOT_NULL(n, r->n);
	SET_IF_NOT_NULL(e, r->e);
	SET_IF_NOT_NULL(d, r->d);
}

void
RSA_get0_factors(const RSA *r, const BIGNUM **p, const BIGNUM **q) {
	SET_IF_NOT_NULL(p, r->p);
	SET_IF_NOT_NULL(q, r->q);
}

void
RSA_get0_crt_params(const RSA *r, const BIGNUM **dmp1, const BIGNUM **dmq1,
		    const BIGNUM **iqmp) {
	SET_IF_NOT_NULL(dmp1, r->dmp1);
	SET_IF_NOT_NULL(dmq1, r->dmq1);
	SET_IF_NOT_NULL(iqmp, r->iqmp);
}

int
RSA_test_flags(const RSA *r, int flags) {
	return r->flags & flags;
}
#endif /* !HAVE_RSA_SET0_KEY && OPENSSL_VERSION_NUMBER < 0x30000000L */

#if !HAVE_ECDSA_SIG_GET0
/* From OpenSSL 1.1 */
void
ECDSA_SIG_get0(const ECDSA_SIG *sig, const BIGNUM **pr, const BIGNUM **ps) {
	SET_IF_NOT_NULL(pr, sig->r);
	SET_IF_NOT_NULL(ps, sig->s);
}

int
ECDSA_SIG_set0(ECDSA_SIG *sig, BIGNUM *r, BIGNUM *s) {
	if (r == NULL || s == NULL) {
		return 0;
	}

	BN_clear_free(sig->r);
	BN_clear_free(sig->s);
	sig->r = r;
	sig->s = s;

	return 1;
}
#endif /* !HAVE_ECDSA_SIG_GET0 */

#if !HAVE_ERR_GET_ERROR_ALL
static const char err_empty_string = '\0';

unsigned long
ERR_get_error_all(const char **file, int *line, const char **func,
		  const char **data, int *flags) {
	SET_IF_NOT_NULL(func, &err_empty_string);
	return ERR_get_error_line_data(file, line, data, flags);
}
#endif /* if !HAVE_ERR_GET_ERROR_ALL */
