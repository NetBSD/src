/*	$NetBSD: tkeyconf.c,v 1.1.1.1 2018/08/12 12:07:43 christos Exp $	*/

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


/*! \file */

#include <config.h>

#include <isc/buffer.h>
#include <isc/string.h>		/* Required for HP/UX (and others?) */
#include <isc/mem.h>

#include <isccfg/cfg.h>

#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/name.h>
#include <dns/tkey.h>

#include <dst/gssapi.h>

#include <named/tkeyconf.h>

#define RETERR(x) do { \
	result = (x); \
	if (result != ISC_R_SUCCESS) \
		goto failure; \
	} while (0)

#include<named/log.h>
#define LOG(msg) \
	isc_log_write(named_g_lctx, \
	NAMED_LOGCATEGORY_GENERAL, \
	NAMED_LOGMODULE_SERVER, \
	ISC_LOG_ERROR, \
	"%s", msg)

isc_result_t
named_tkeyctx_fromconfig(const cfg_obj_t *options, isc_mem_t *mctx,
			 isc_entropy_t *ectx, dns_tkeyctx_t **tctxp)
{
	isc_result_t result;
	dns_tkeyctx_t *tctx = NULL;
	const char *s;
	isc_uint32_t n;
	dns_fixedname_t fname;
	dns_name_t *name;
	isc_buffer_t b;
	const cfg_obj_t *obj;
	int type;

	result = dns_tkeyctx_create(mctx, ectx, &tctx);
	if (result != ISC_R_SUCCESS)
		return (result);

	obj = NULL;
	result = cfg_map_get(options, "tkey-dhkey", &obj);
	if (result == ISC_R_SUCCESS) {
		s = cfg_obj_asstring(cfg_tuple_get(obj, "name"));
		n = cfg_obj_asuint32(cfg_tuple_get(obj, "keyid"));
		isc_buffer_constinit(&b, s, strlen(s));
		isc_buffer_add(&b, strlen(s));
		name = dns_fixedname_initname(&fname);
		RETERR(dns_name_fromtext(name, &b, dns_rootname, 0, NULL));
		type = DST_TYPE_PUBLIC|DST_TYPE_PRIVATE|DST_TYPE_KEY;
		RETERR(dst_key_fromfile(name, (dns_keytag_t) n, DNS_KEYALG_DH,
					type, NULL, mctx, &tctx->dhkey));
	}

	obj = NULL;
	result = cfg_map_get(options, "tkey-domain", &obj);
	if (result == ISC_R_SUCCESS) {
		s = cfg_obj_asstring(obj);
		isc_buffer_constinit(&b, s, strlen(s));
		isc_buffer_add(&b, strlen(s));
		name = dns_fixedname_initname(&fname);
		RETERR(dns_name_fromtext(name, &b, dns_rootname, 0, NULL));
		tctx->domain = isc_mem_get(mctx, sizeof(dns_name_t));
		if (tctx->domain == NULL) {
			result = ISC_R_NOMEMORY;
			goto failure;
		}
		dns_name_init(tctx->domain, NULL);
		RETERR(dns_name_dup(name, mctx, tctx->domain));
	}

	obj = NULL;
	result = cfg_map_get(options, "tkey-gssapi-credential", &obj);
	if (result == ISC_R_SUCCESS) {
		s = cfg_obj_asstring(obj);

		isc_buffer_constinit(&b, s, strlen(s));
		isc_buffer_add(&b, strlen(s));
		name = dns_fixedname_initname(&fname);
		RETERR(dns_name_fromtext(name, &b, dns_rootname, 0, NULL));
		RETERR(dst_gssapi_acquirecred(name, ISC_FALSE, &tctx->gsscred));
	}

	obj = NULL;
	result = cfg_map_get(options, "tkey-gssapi-keytab", &obj);
	if (result == ISC_R_SUCCESS) {
		s = cfg_obj_asstring(obj);
		tctx->gssapi_keytab = isc_mem_strdup(mctx, s);
		if (tctx->gssapi_keytab == NULL) {
			result = ISC_R_NOMEMORY;
			goto failure;
		}
	}

	*tctxp = tctx;
	return (ISC_R_SUCCESS);

 failure:
	dns_tkeyctx_destroy(&tctx);
	return (result);
}

