/*	$NetBSD: keygen.h,v 1.2 2018/08/12 13:02:27 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */


#ifndef RNDC_KEYGEN_H
#define RNDC_KEYGEN_H 1

/*! \file */

#include <isc/lang.h>

ISC_LANG_BEGINDECLS

void generate_key(isc_mem_t *mctx, const char *randomfile, dns_secalg_t alg,
		  int keysize, isc_buffer_t *key_txtbuffer);

void write_key_file(const char *keyfile, const char *user,
		    const char *keyname, isc_buffer_t *secret,
		    dns_secalg_t alg);

const char *alg_totext(dns_secalg_t alg);
dns_secalg_t alg_fromtext(const char *name);
int alg_bits(dns_secalg_t alg);

ISC_LANG_ENDDECLS

#endif /* RNDC_KEYGEN_H */
