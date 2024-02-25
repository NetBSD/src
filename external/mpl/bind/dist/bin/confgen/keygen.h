/*	$NetBSD: keygen.h,v 1.6.2.1 2024/02/25 15:43:00 martin Exp $	*/

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

#pragma once

/*! \file */

#include <isc/buffer.h>
#include <isc/lang.h>
#include <isc/mem.h>

#include <dns/secalg.h>

ISC_LANG_BEGINDECLS

void
generate_key(isc_mem_t *mctx, dns_secalg_t alg, int keysize,
	     isc_buffer_t *key_txtbuffer);

void
write_key_file(const char *keyfile, const char *user, const char *keyname,
	       isc_buffer_t *secret, dns_secalg_t alg);

const char *
alg_totext(dns_secalg_t alg);
dns_secalg_t
alg_fromtext(const char *name);
int
alg_bits(dns_secalg_t alg);

ISC_LANG_ENDDECLS
