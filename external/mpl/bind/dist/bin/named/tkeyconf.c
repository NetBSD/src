/*	$NetBSD: tkeyconf.c,v 1.9 2026/01/29 18:36:27 christos Exp $	*/

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

/*! \file */

#include <inttypes.h>

#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/string.h>

#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/name.h>
#include <dns/tkey.h>

#include <dst/gssapi.h>

#include <isccfg/cfg.h>

#include <named/log.h>
#include <named/tkeyconf.h>
#define LOG(msg)                                               \
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL, \
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR, "%s", msg)

isc_result_t
named_tkeyctx_fromconfig(const cfg_obj_t *options, isc_mem_t *mctx,
			 dns_tkeyctx_t **tctxp) {
	isc_result_t result;
	dns_tkeyctx_t *tctx = NULL;
	const char *s = NULL;
	dns_fixedname_t fname;
	dns_name_t *name = NULL;
	isc_buffer_t b;
	const cfg_obj_t *obj = NULL;

	result = dns_tkeyctx_create(mctx, &tctx);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	result = cfg_map_get(options, "tkey-gssapi-credential", &obj);
	if (result == ISC_R_SUCCESS) {
		s = cfg_obj_asstring(obj);

		isc_buffer_constinit(&b, s, strlen(s));
		isc_buffer_add(&b, strlen(s));
		name = dns_fixedname_initname(&fname);
		CHECK(dns_name_fromtext(name, &b, dns_rootname, 0, NULL));
		CHECK(dst_gssapi_acquirecred(name, false, &tctx->gsscred));
	}

	obj = NULL;
	result = cfg_map_get(options, "tkey-gssapi-keytab", &obj);
	if (result == ISC_R_SUCCESS) {
		s = cfg_obj_asstring(obj);
		tctx->gssapi_keytab = isc_mem_strdup(mctx, s);
	}

	*tctxp = tctx;
	return ISC_R_SUCCESS;

cleanup:
	dns_tkeyctx_destroy(&tctx);
	return result;
}
