/*	$NetBSD: tsigconf.c,v 1.1.1.1 2018/08/12 12:07:41 christos Exp $	*/

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

#include <isc/base64.h>
#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/string.h>
#include <isc/util.h>

#include <isccfg/cfg.h>

#include <dns/tsig.h>
#include <dns/result.h>

#include <named/log.h>

#include <named/config.h>
#include <named/tsigconf.h>

static isc_result_t
add_initial_keys(const cfg_obj_t *list, dns_tsig_keyring_t *ring,
		 isc_mem_t *mctx)
{
	dns_tsigkey_t *tsigkey = NULL;
	const cfg_listelt_t *element;
	const cfg_obj_t *key = NULL;
	const char *keyid = NULL;
	unsigned char *secret = NULL;
	int secretalloc = 0;
	int secretlen = 0;
	isc_result_t ret;
	isc_stdtime_t now;
	isc_uint16_t bits;

	for (element = cfg_list_first(list);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *algobj = NULL;
		const cfg_obj_t *secretobj = NULL;
		dns_name_t keyname;
		const dns_name_t *alg;
		const char *algstr;
		char keynamedata[1024];
		isc_buffer_t keynamesrc, keynamebuf;
		const char *secretstr;
		isc_buffer_t secretbuf;

		key = cfg_listelt_value(element);
		keyid = cfg_obj_asstring(cfg_map_getname(key));

		algobj = NULL;
		secretobj = NULL;
		(void)cfg_map_get(key, "algorithm", &algobj);
		(void)cfg_map_get(key, "secret", &secretobj);
		INSIST(algobj != NULL && secretobj != NULL);

		/*
		 * Create the key name.
		 */
		dns_name_init(&keyname, NULL);
		isc_buffer_constinit(&keynamesrc, keyid, strlen(keyid));
		isc_buffer_add(&keynamesrc, strlen(keyid));
		isc_buffer_init(&keynamebuf, keynamedata, sizeof(keynamedata));
		ret = dns_name_fromtext(&keyname, &keynamesrc, dns_rootname,
					DNS_NAME_DOWNCASE, &keynamebuf);
		if (ret != ISC_R_SUCCESS)
			goto failure;

		/*
		 * Create the algorithm.
		 */
		algstr = cfg_obj_asstring(algobj);
		if (named_config_getkeyalgorithm(algstr, &alg, &bits)
		    != ISC_R_SUCCESS) {
			cfg_obj_log(algobj, named_g_lctx, ISC_LOG_ERROR,
				    "key '%s': has a "
				    "unsupported algorithm '%s'",
				    keyid, algstr);
			ret = DNS_R_BADALG;
			goto failure;
		}

		secretstr = cfg_obj_asstring(secretobj);
		secretalloc = secretlen = strlen(secretstr) * 3 / 4;
		secret = isc_mem_get(mctx, secretlen);
		if (secret == NULL) {
			ret = ISC_R_NOMEMORY;
			goto failure;
		}
		isc_buffer_init(&secretbuf, secret, secretlen);
		ret = isc_base64_decodestring(secretstr, &secretbuf);
		if (ret != ISC_R_SUCCESS)
			goto failure;
		secretlen = isc_buffer_usedlength(&secretbuf);

		isc_stdtime_get(&now);
		ret = dns_tsigkey_create(&keyname, alg, secret, secretlen,
					 ISC_FALSE, NULL, now, now,
					 mctx, ring, &tsigkey);
		isc_mem_put(mctx, secret, secretalloc);
		secret = NULL;
		if (ret != ISC_R_SUCCESS)
			goto failure;
		/*
		 * Set digest bits.
		 */
		dst_key_setbits(tsigkey->key, bits);
		dns_tsigkey_detach(&tsigkey);
	}

	return (ISC_R_SUCCESS);

 failure:
	cfg_obj_log(key, named_g_lctx, ISC_LOG_ERROR,
		    "configuring key '%s': %s", keyid,
		    isc_result_totext(ret));

	if (secret != NULL)
		isc_mem_put(mctx, secret, secretalloc);
	return (ret);
}

isc_result_t
named_tsigkeyring_fromconfig(const cfg_obj_t *config, const cfg_obj_t *vconfig,
			     isc_mem_t *mctx, dns_tsig_keyring_t **ringp)
{
	const cfg_obj_t *maps[3];
	const cfg_obj_t *keylist;
	dns_tsig_keyring_t *ring = NULL;
	isc_result_t result;
	int i;

	REQUIRE(ringp != NULL && *ringp == NULL);

	i = 0;
	if (config != NULL)
		maps[i++] = config;
	if (vconfig != NULL)
		maps[i++] = cfg_tuple_get(vconfig, "options");
	maps[i] = NULL;

	result = dns_tsigkeyring_create(mctx, &ring);
	if (result != ISC_R_SUCCESS)
		return (result);

	for (i = 0; ; i++) {
		if (maps[i] == NULL)
			break;
		keylist = NULL;
		result = cfg_map_get(maps[i], "key", &keylist);
		if (result != ISC_R_SUCCESS)
			continue;
		result = add_initial_keys(keylist, ring, mctx);
		if (result != ISC_R_SUCCESS)
			goto failure;
	}

	*ringp = ring;
	return (ISC_R_SUCCESS);

 failure:
	dns_tsigkeyring_detach(&ring);
	return (result);
}
