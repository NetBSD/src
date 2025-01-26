/*	$NetBSD: kaspconf.c,v 1.9 2025/01/26 16:25:45 christos Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include <isc/fips.h>
#include <isc/mem.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/kasp.h>
#include <dns/keystore.h>
#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/nsec3.h>
#include <dns/secalg.h>
#include <dns/ttl.h>

#include <isccfg/cfg.h>
#include <isccfg/duration.h>
#include <isccfg/kaspconf.h>
#include <isccfg/namedconf.h>

#define DEFAULT_NSEC3PARAM_ITER	   0
#define DEFAULT_NSEC3PARAM_SALTLEN 0

/*
 * Utility function for getting a configuration option.
 */
static isc_result_t
confget(cfg_obj_t const *const *maps, const char *name, const cfg_obj_t **obj) {
	for (size_t i = 0;; i++) {
		if (maps[i] == NULL) {
			return ISC_R_NOTFOUND;
		}
		if (cfg_map_get(maps[i], name, obj) == ISC_R_SUCCESS) {
			return ISC_R_SUCCESS;
		}
	}
}

/*
 * Utility function for parsing durations from string.
 */
static uint32_t
parse_duration(const char *str) {
	uint32_t time = 0;
	isccfg_duration_t duration;
	isc_result_t result;
	isc_textregion_t tr;

	tr.base = UNCONST(str);
	tr.length = strlen(tr.base);
	result = isccfg_parse_duration(&tr, &duration);
	if (result == ISC_R_SUCCESS) {
		time = isccfg_duration_toseconds(&duration);
	}
	return time;
}

/*
 * Utility function for configuring durations.
 */
static uint32_t
get_duration(const cfg_obj_t **maps, const char *option, const char *dfl) {
	const cfg_obj_t *obj;
	isc_result_t result;
	obj = NULL;

	result = confget(maps, option, &obj);
	if (result == ISC_R_NOTFOUND) {
		return parse_duration(dfl);
	}
	INSIST(result == ISC_R_SUCCESS);
	return cfg_obj_asduration(obj);
}

/*
 * Utility function for configuring strings.
 */
static const char *
get_string(const cfg_obj_t **maps, const char *option) {
	const cfg_obj_t *obj;
	isc_result_t result;
	obj = NULL;

	result = confget(maps, option, &obj);
	if (result == ISC_R_NOTFOUND) {
		return NULL;
	}
	INSIST(result == ISC_R_SUCCESS);
	return cfg_obj_asstring(obj);
}

/*
 * Create a new kasp key derived from configuration.
 */
static isc_result_t
cfg_kaspkey_fromconfig(const cfg_obj_t *config, dns_kasp_t *kasp,
		       bool check_algorithms, isc_log_t *logctx,
		       bool offline_ksk, dns_keystorelist_t *keystorelist,
		       uint32_t ksk_min_lifetime, uint32_t zsk_min_lifetime) {
	isc_result_t result;
	dns_kasp_key_t *key = NULL;
	const cfg_obj_t *tagrange = NULL;

	/* Create a new key reference. */
	result = dns_kasp_key_create(kasp, &key);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	if (config == NULL) {
		/* We are creating a key reference for the default kasp. */
		INSIST(!offline_ksk);
		key->role |= DNS_KASP_KEY_ROLE_KSK | DNS_KASP_KEY_ROLE_ZSK;
		key->lifetime = 0; /* unlimited */
		key->algorithm = DNS_KEYALG_ECDSA256;
		key->length = -1;
		result = dns_keystorelist_find(keystorelist,
					       DNS_KEYSTORE_KEYDIRECTORY,
					       &key->keystore);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
	} else {
		const char *rolestr = NULL;
		const char *keydir = NULL;
		const cfg_obj_t *obj = NULL;
		isc_consttextregion_t alg;
		bool error = false;

		rolestr = cfg_obj_asstring(cfg_tuple_get(config, "role"));
		if (strcmp(rolestr, "ksk") == 0) {
			key->role |= DNS_KASP_KEY_ROLE_KSK;
		} else if (strcmp(rolestr, "zsk") == 0) {
			key->role |= DNS_KASP_KEY_ROLE_ZSK;
		} else if (strcmp(rolestr, "csk") == 0) {
			if (offline_ksk) {
				cfg_obj_log(
					config, logctx, ISC_LOG_ERROR,
					"dnssec-policy: csk keys are not "
					"allowed when offline-ksk is enabled");
				result = ISC_R_FAILURE;
				goto cleanup;
			}
			key->role |= DNS_KASP_KEY_ROLE_KSK;
			key->role |= DNS_KASP_KEY_ROLE_ZSK;
		}

		obj = cfg_tuple_get(config, "keystorage");
		if (cfg_obj_isstring(obj)) {
			keydir = cfg_obj_asstring(obj);
		}
		if (keydir == NULL) {
			keydir = DNS_KEYSTORE_KEYDIRECTORY;
		}
		result = dns_keystorelist_find(keystorelist, keydir,
					       &key->keystore);
		if (result == ISC_R_NOTFOUND) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "dnssec-policy: keystore %s does not exist",
				    keydir);
			result = ISC_R_FAILURE;
			goto cleanup;
		} else if (result != ISC_R_SUCCESS) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "dnssec-policy: bad keystore %s", keydir);
			result = ISC_R_FAILURE;
			goto cleanup;
		}
		INSIST(key->keystore != NULL);

		key->lifetime = 0; /* unlimited */
		obj = cfg_tuple_get(config, "lifetime");
		if (cfg_obj_isduration(obj)) {
			key->lifetime = cfg_obj_asduration(obj);
		}
		if (key->lifetime > 0) {
			if (key->lifetime < 30 * (24 * 3600)) {
				cfg_obj_log(obj, logctx, ISC_LOG_WARNING,
					    "dnssec-policy: key lifetime is "
					    "shorter than 30 days");
			}
			if ((key->role & DNS_KASP_KEY_ROLE_KSK) != 0 &&
			    key->lifetime <= ksk_min_lifetime)
			{
				error = true;
			}
			if ((key->role & DNS_KASP_KEY_ROLE_ZSK) != 0 &&
			    key->lifetime <= zsk_min_lifetime)
			{
				error = true;
			}
			if (error) {
				cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
					    "dnssec-policy: key lifetime is "
					    "shorter than the time it takes to "
					    "do a rollover");
				result = ISC_R_FAILURE;
				goto cleanup;
			}
		}

		obj = cfg_tuple_get(config, "algorithm");
		alg.base = cfg_obj_asstring(obj);
		alg.length = strlen(alg.base);
		result = dns_secalg_fromtext(&key->algorithm,
					     (isc_textregion_t *)&alg);
		if (result != ISC_R_SUCCESS) {
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "dnssec-policy: bad algorithm %s",
				    alg.base);
			result = DNS_R_BADALG;
			goto cleanup;
		}

		if (check_algorithms && isc_fips_mode() &&
		    (key->algorithm == DNS_KEYALG_RSASHA1 ||
		     key->algorithm == DNS_KEYALG_NSEC3RSASHA1))
		{
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "dnssec-policy: algorithm %s not supported "
				    "in FIPS mode",
				    alg.base);
			result = DNS_R_BADALG;
			goto cleanup;
		}

		if (check_algorithms &&
		    !dst_algorithm_supported(key->algorithm))
		{
			cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
				    "dnssec-policy: algorithm %s not supported",
				    alg.base);
			result = DNS_R_BADALG;
			goto cleanup;
		}

		obj = cfg_tuple_get(config, "length");
		if (cfg_obj_isuint32(obj)) {
			uint32_t min, size;
			size = cfg_obj_asuint32(obj);

			switch (key->algorithm) {
			case DNS_KEYALG_RSASHA1:
			case DNS_KEYALG_NSEC3RSASHA1:
			case DNS_KEYALG_RSASHA256:
			case DNS_KEYALG_RSASHA512:
				if (isc_fips_mode()) {
					min = 2048;
				} else {
					min = DNS_KEYALG_RSASHA512 ? 1024 : 512;
				}
				if (size < min || size > 4096) {
					cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
						    "dnssec-policy: key with "
						    "algorithm %s has invalid "
						    "key length %u",
						    alg.base, size);
					result = ISC_R_RANGE;
					goto cleanup;
				}
				break;
			case DNS_KEYALG_ECDSA256:
			case DNS_KEYALG_ECDSA384:
			case DNS_KEYALG_ED25519:
			case DNS_KEYALG_ED448:
				cfg_obj_log(obj, logctx, ISC_LOG_WARNING,
					    "dnssec-policy: key algorithm %s "
					    "has predefined length; ignoring "
					    "length value %u",
					    alg.base, size);
			default:
				break;
			}

			key->length = size;
		}

		tagrange = cfg_tuple_get(config, "tag-range");
		if (cfg_obj_istuple(tagrange)) {
			uint32_t tag_min = 0, tag_max = 0xffff;
			obj = cfg_tuple_get(tagrange, "tag-min");
			tag_min = cfg_obj_asuint32(obj);
			if (tag_min > 0xffff) {
				cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
					    "dnssec-policy: tag-min "
					    "too big");
				result = ISC_R_RANGE;
				goto cleanup;
			}
			obj = cfg_tuple_get(tagrange, "tag-max");
			tag_max = cfg_obj_asuint32(obj);
			if (tag_max > 0xffff) {
				cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
					    "dnssec-policy: tag-max "
					    "too big");
				result = ISC_R_RANGE;
				goto cleanup;
			}
			if (tag_min >= tag_max) {
				cfg_obj_log(
					obj, logctx, ISC_LOG_ERROR,
					"dnssec-policy: tag-min >= tag_max");
				result = ISC_R_RANGE;
				goto cleanup;
			}
			key->tag_min = tag_min;
			key->tag_max = tag_max;
		}
	}

	dns_kasp_addkey(kasp, key);
	return ISC_R_SUCCESS;

cleanup:

	dns_kasp_key_destroy(key);
	return result;
}

static isc_result_t
cfg_nsec3param_fromconfig(const cfg_obj_t *config, dns_kasp_t *kasp,
			  isc_log_t *logctx) {
	dns_kasp_key_t *kkey;
	unsigned int min_keysize = 4096;
	const cfg_obj_t *obj = NULL;
	uint32_t iter = DEFAULT_NSEC3PARAM_ITER;
	uint32_t saltlen = DEFAULT_NSEC3PARAM_SALTLEN;
	uint32_t badalg = 0;
	bool optout = false;

	/* How many iterations. */
	obj = cfg_tuple_get(config, "iterations");
	if (cfg_obj_isuint32(obj)) {
		iter = cfg_obj_asuint32(obj);
	}
	dns_kasp_freeze(kasp);
	for (kkey = ISC_LIST_HEAD(dns_kasp_keys(kasp)); kkey != NULL;
	     kkey = ISC_LIST_NEXT(kkey, link))
	{
		unsigned int keysize = dns_kasp_key_size(kkey);
		uint32_t keyalg = dns_kasp_key_algorithm(kkey);

		if (keysize < min_keysize) {
			min_keysize = keysize;
		}

		/* NSEC3 cannot be used with certain key algorithms. */
		if (keyalg == DNS_KEYALG_RSAMD5 || keyalg == DNS_KEYALG_DSA ||
		    keyalg == DNS_KEYALG_RSASHA1)
		{
			badalg = keyalg;
		}
	}
	dns_kasp_thaw(kasp);

	if (badalg > 0) {
		char algstr[DNS_SECALG_FORMATSIZE];
		dns_secalg_format((dns_secalg_t)badalg, algstr, sizeof(algstr));
		cfg_obj_log(
			obj, logctx, ISC_LOG_ERROR,
			"dnssec-policy: cannot use nsec3 with algorithm '%s'",
			algstr);
		return DNS_R_NSEC3BADALG;
	}

	if (iter != DEFAULT_NSEC3PARAM_ITER) {
		cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
			    "dnssec-policy: nsec3 iterations value %u "
			    "not allowed, must be zero",
			    iter);
		return DNS_R_NSEC3ITERRANGE;
	}

	/* Opt-out? */
	obj = cfg_tuple_get(config, "optout");
	if (cfg_obj_isboolean(obj)) {
		optout = cfg_obj_asboolean(obj);
	}

	/* Salt */
	obj = cfg_tuple_get(config, "salt-length");
	if (cfg_obj_isuint32(obj)) {
		saltlen = cfg_obj_asuint32(obj);
	}
	if (saltlen > 0xff) {
		cfg_obj_log(obj, logctx, ISC_LOG_ERROR,
			    "dnssec-policy: nsec3 salt length %u too high",
			    saltlen);
		return DNS_R_NSEC3SALTRANGE;
	}

	dns_kasp_setnsec3param(kasp, iter, optout, saltlen);
	return ISC_R_SUCCESS;
}

static isc_result_t
add_digest(dns_kasp_t *kasp, const cfg_obj_t *digest, isc_log_t *logctx) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_textregion_t r;
	dns_dsdigest_t alg;
	const char *str = cfg_obj_asstring(digest);

	r.base = UNCONST(str);
	r.length = strlen(str);
	result = dns_dsdigest_fromtext(&alg, &r);
	if (result != ISC_R_SUCCESS) {
		cfg_obj_log(digest, logctx, ISC_LOG_ERROR,
			    "dnssec-policy: bad cds digest-type %s", str);
		result = DNS_R_BADALG;
	} else if (!dst_ds_digest_supported(alg)) {
		cfg_obj_log(digest, logctx, ISC_LOG_ERROR,
			    "dnssec-policy: unsupported cds "
			    "digest-type %s",
			    str);
		result = DST_R_UNSUPPORTEDALG;
	} else {
		dns_kasp_adddigest(kasp, alg);
	}
	return result;
}

isc_result_t
cfg_kasp_fromconfig(const cfg_obj_t *config, dns_kasp_t *default_kasp,
		    bool check_algorithms, isc_mem_t *mctx, isc_log_t *logctx,
		    dns_keystorelist_t *keystorelist, dns_kasplist_t *kasplist,
		    dns_kasp_t **kaspp) {
	isc_result_t result;
	const cfg_obj_t *maps[2];
	const cfg_obj_t *koptions = NULL;
	const cfg_obj_t *keys = NULL;
	const cfg_obj_t *nsec3 = NULL;
	const cfg_obj_t *inlinesigning = NULL;
	const cfg_obj_t *cds = NULL;
	const cfg_obj_t *obj = NULL;
	const cfg_listelt_t *element = NULL;
	const char *kaspname = NULL;
	dns_kasp_t *kasp = NULL;
	size_t i = 0;
	uint32_t sigjitter = 0, sigrefresh = 0, sigvalidity = 0;
	uint32_t dnskeyttl = 0, dsttl = 0, maxttl = 0;
	uint32_t publishsafety = 0, retiresafety = 0;
	uint32_t zonepropdelay = 0, parentpropdelay = 0;
	uint32_t ipub = 0, iret = 0;
	uint32_t ksk_min_lifetime = 0, zsk_min_lifetime = 0;
	bool offline_ksk = false;

	REQUIRE(config != NULL);
	REQUIRE(kaspp != NULL && *kaspp == NULL);

	kaspname = cfg_obj_asstring(cfg_tuple_get(config, "name"));
	INSIST(kaspname != NULL);

	cfg_obj_log(config, logctx, ISC_LOG_DEBUG(1),
		    "dnssec-policy: load policy '%s'", kaspname);

	result = dns_kasplist_find(kasplist, kaspname, &kasp);

	if (result == ISC_R_SUCCESS) {
		cfg_obj_log(
			config, logctx, ISC_LOG_ERROR,
			"dnssec-policy: duplicately named policy found '%s'",
			kaspname);
		dns_kasp_detach(&kasp);
		return ISC_R_EXISTS;
	}
	if (result != ISC_R_NOTFOUND) {
		return result;
	}

	/* No kasp with configured name was found in list, create new one. */
	INSIST(kasp == NULL);
	result = dns_kasp_create(mctx, kaspname, &kasp);
	if (result != ISC_R_SUCCESS) {
		return result;
	}
	INSIST(kasp != NULL);

	/* Now configure. */
	INSIST(DNS_KASP_VALID(kasp));

	if (config != NULL) {
		koptions = cfg_tuple_get(config, "options");
		maps[i++] = koptions;
	}
	maps[i] = NULL;

	/* Configuration: Signatures */
	sigjitter = get_duration(maps, "signatures-jitter",
				 DNS_KASP_SIG_JITTER);
	dns_kasp_setsigjitter(kasp, sigjitter);

	sigrefresh = get_duration(maps, "signatures-refresh",
				  DNS_KASP_SIG_REFRESH);
	dns_kasp_setsigrefresh(kasp, sigrefresh);

	sigvalidity = get_duration(maps, "signatures-validity-dnskey",
				   DNS_KASP_SIG_VALIDITY_DNSKEY);
	if (sigrefresh >= (sigvalidity * 0.9)) {
		cfg_obj_log(
			config, logctx, ISC_LOG_ERROR,
			"dnssec-policy: policy '%s' signatures-refresh must be "
			"at most 90%% of the signatures-validity-dnskey",
			kaspname);
		result = ISC_R_FAILURE;
	}
	dns_kasp_setsigvalidity_dnskey(kasp, sigvalidity);

	if (sigjitter > sigvalidity) {
		cfg_obj_log(
			config, logctx, ISC_LOG_ERROR,
			"dnssec-policy: policy '%s' signatures-jitter cannot "
			"be larger than signatures-validity-dnskey",
			kaspname);
		result = ISC_R_FAILURE;
	}

	sigvalidity = get_duration(maps, "signatures-validity",
				   DNS_KASP_SIG_VALIDITY);
	if (sigrefresh >= (sigvalidity * 0.9)) {
		cfg_obj_log(
			config, logctx, ISC_LOG_ERROR,
			"dnssec-policy: policy '%s' signatures-refresh must be "
			"at most 90%% of the signatures-validity",
			kaspname);
		result = ISC_R_FAILURE;
	}
	dns_kasp_setsigvalidity(kasp, sigvalidity);

	if (sigjitter > sigvalidity) {
		cfg_obj_log(
			config, logctx, ISC_LOG_ERROR,
			"dnssec-policy: policy '%s' signatures-jitter cannot "
			"be larger than signatures-validity",
			kaspname);
		result = ISC_R_FAILURE;
	}

	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	/* Configuration: Zone settings */
	(void)confget(maps, "inline-signing", &inlinesigning);
	if (inlinesigning != NULL && cfg_obj_isboolean(inlinesigning)) {
		dns_kasp_setinlinesigning(kasp,
					  cfg_obj_asboolean(inlinesigning));
	} else {
		dns_kasp_setinlinesigning(kasp, true);
	}

	maxttl = get_duration(maps, "max-zone-ttl", DNS_KASP_ZONE_MAXTTL);
	dns_kasp_setzonemaxttl(kasp, maxttl);

	zonepropdelay = get_duration(maps, "zone-propagation-delay",
				     DNS_KASP_ZONE_PROPDELAY);
	dns_kasp_setzonepropagationdelay(kasp, zonepropdelay);

	/* Configuration: Parent settings */
	dsttl = get_duration(maps, "parent-ds-ttl", DNS_KASP_DS_TTL);
	dns_kasp_setdsttl(kasp, dsttl);

	parentpropdelay = get_duration(maps, "parent-propagation-delay",
				       DNS_KASP_PARENT_PROPDELAY);
	dns_kasp_setparentpropagationdelay(kasp, parentpropdelay);

	/* Configuration: Keys */
	obj = NULL;
	(void)confget(maps, "offline-ksk", &obj);
	if (obj != NULL) {
		offline_ksk = cfg_obj_asboolean(obj);
	}
	dns_kasp_setofflineksk(kasp, offline_ksk);

	obj = NULL;
	(void)confget(maps, "cdnskey", &obj);
	if (obj != NULL) {
		dns_kasp_setcdnskey(kasp, cfg_obj_asboolean(obj));
	} else {
		dns_kasp_setcdnskey(kasp, true);
	}

	(void)confget(maps, "cds-digest-types", &cds);
	if (cds != NULL) {
		for (element = cfg_list_first(cds); element != NULL;
		     element = cfg_list_next(element))
		{
			result = add_digest(kasp, cfg_listelt_value(element),
					    logctx);
			if (result != ISC_R_SUCCESS) {
				goto cleanup;
			}
		}
	} else {
		dns_kasp_adddigest(kasp, DNS_DSDIGEST_SHA256);
	}

	dnskeyttl = get_duration(maps, "dnskey-ttl", DNS_KASP_KEY_TTL);
	dns_kasp_setdnskeyttl(kasp, dnskeyttl);

	publishsafety = get_duration(maps, "publish-safety",
				     DNS_KASP_PUBLISH_SAFETY);
	dns_kasp_setpublishsafety(kasp, publishsafety);

	retiresafety = get_duration(maps, "retire-safety",
				    DNS_KASP_RETIRE_SAFETY);
	dns_kasp_setretiresafety(kasp, retiresafety);

	dns_kasp_setpurgekeys(
		kasp, get_duration(maps, "purge-keys", DNS_KASP_PURGE_KEYS));

	ipub = dnskeyttl + publishsafety + zonepropdelay;
	iret = dsttl + retiresafety + parentpropdelay;
	ksk_min_lifetime = ISC_MAX(ipub, iret);

	iret = (sigvalidity - sigrefresh) + maxttl + retiresafety +
	       zonepropdelay;
	zsk_min_lifetime = ISC_MAX(ipub, iret);

	(void)confget(maps, "keys", &keys);
	if (keys != NULL) {
		char role[256] = { 0 };
		bool warn[256][2] = { { false } };
		dns_kasp_key_t *kkey = NULL;

		for (element = cfg_list_first(keys); element != NULL;
		     element = cfg_list_next(element))
		{
			cfg_obj_t *kobj = cfg_listelt_value(element);
			result = cfg_kaspkey_fromconfig(
				kobj, kasp, check_algorithms, logctx,
				offline_ksk, keystorelist, ksk_min_lifetime,
				zsk_min_lifetime);
			if (result != ISC_R_SUCCESS) {
				cfg_obj_log(kobj, logctx, ISC_LOG_ERROR,
					    "dnssec-policy: failed to "
					    "configure keys (%s)",
					    isc_result_totext(result));
				goto cleanup;
			}
		}
		dns_kasp_freeze(kasp);
		for (kkey = ISC_LIST_HEAD(dns_kasp_keys(kasp)); kkey != NULL;
		     kkey = ISC_LIST_NEXT(kkey, link))
		{
			uint32_t keyalg = dns_kasp_key_algorithm(kkey);
			INSIST(keyalg < ARRAY_SIZE(role));

			if (dns_kasp_key_zsk(kkey)) {
				if ((role[keyalg] & DNS_KASP_KEY_ROLE_ZSK) != 0)
				{
					warn[keyalg][0] = true;
				}
				role[keyalg] |= DNS_KASP_KEY_ROLE_ZSK;
			}

			if (dns_kasp_key_ksk(kkey)) {
				if ((role[keyalg] & DNS_KASP_KEY_ROLE_KSK) != 0)
				{
					warn[keyalg][1] = true;
				}
				role[keyalg] |= DNS_KASP_KEY_ROLE_KSK;
			}
		}
		dns_kasp_thaw(kasp);
		for (i = 0; i < ARRAY_SIZE(role); i++) {
			if (role[i] == 0) {
				continue;
			}
			if (role[i] !=
			    (DNS_KASP_KEY_ROLE_ZSK | DNS_KASP_KEY_ROLE_KSK))
			{
				cfg_obj_log(keys, logctx, ISC_LOG_ERROR,
					    "dnssec-policy: algorithm %zu "
					    "requires both KSK and ZSK roles",
					    i);
				result = ISC_R_FAILURE;
			}
			if (warn[i][0]) {
				cfg_obj_log(keys, logctx, ISC_LOG_WARNING,
					    "dnssec-policy: algorithm %zu has "
					    "multiple keys with ZSK role",
					    i);
			}
			if (warn[i][1]) {
				cfg_obj_log(keys, logctx, ISC_LOG_WARNING,
					    "dnssec-policy: algorithm %zu has "
					    "multiple keys with KSK role",
					    i);
			}
		}
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
	} else if (default_kasp) {
		dns_kasp_key_t *key, *new_key;
		/*
		 * If there are no specific keys configured in the policy,
		 * inherit from the default policy (except for the built-in
		 * "insecure" policy).
		 */
		for (key = ISC_LIST_HEAD(dns_kasp_keys(default_kasp));
		     key != NULL; key = ISC_LIST_NEXT(key, link))
		{
			/* Create a new key reference. */
			new_key = NULL;
			result = dns_kasp_key_create(kasp, &new_key);
			if (result != ISC_R_SUCCESS) {
				cfg_obj_log(config, logctx, ISC_LOG_ERROR,
					    "dnssec-policy: failed to "
					    "configure keys (%s)",
					    isc_result_totext(result));
				goto cleanup;
			}
			if (dns_kasp_key_ksk(key)) {
				new_key->role |= DNS_KASP_KEY_ROLE_KSK;
			}
			if (dns_kasp_key_zsk(key)) {
				new_key->role |= DNS_KASP_KEY_ROLE_ZSK;
			}
			new_key->lifetime = dns_kasp_key_lifetime(key);
			new_key->algorithm = dns_kasp_key_algorithm(key);
			new_key->length = dns_kasp_key_size(key);
			result = dns_keystorelist_find(
				keystorelist, DNS_KEYSTORE_KEYDIRECTORY,
				&new_key->keystore);
			if (result != ISC_R_SUCCESS) {
				cfg_obj_log(config, logctx, ISC_LOG_ERROR,
					    "dnssec-policy: failed to "
					    "find keystore (%s)",
					    isc_result_totext(result));
				goto cleanup;
			}
			dns_kasp_addkey(kasp, new_key);
		}
	}

	if (strcmp(kaspname, "insecure") == 0) {
		/* "dnssec-policy insecure": key list must be empty */
		INSIST(dns_kasp_keylist_empty(kasp));
	} else if (default_kasp != NULL) {
		/* There must be keys configured. */
		INSIST(!(dns_kasp_keylist_empty(kasp)));
	}

	/* Configuration: NSEC3 */
	(void)confget(maps, "nsec3param", &nsec3);
	if (nsec3 == NULL) {
		if (default_kasp != NULL && dns_kasp_nsec3(default_kasp)) {
			dns_kasp_setnsec3param(
				kasp, dns_kasp_nsec3iter(default_kasp),
				(dns_kasp_nsec3flags(default_kasp) == 0x01),
				dns_kasp_nsec3saltlen(default_kasp));
		} else {
			dns_kasp_setnsec3(kasp, false);
		}
	} else {
		dns_kasp_setnsec3(kasp, true);
		result = cfg_nsec3param_fromconfig(nsec3, kasp, logctx);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
	}

	/* Append it to the list for future lookups. */
	ISC_LIST_APPEND(*kasplist, kasp, link);
	INSIST(!(ISC_LIST_EMPTY(*kasplist)));

	/* Success: Attach the kasp to the pointer and return. */
	dns_kasp_attach(kasp, kaspp);

	/* Don't detach as kasp is on '*kasplist' */
	return ISC_R_SUCCESS;

cleanup:

	/* Something bad happened, detach (destroys kasp) and return error. */
	dns_kasp_detach(&kasp);
	return result;
}

isc_result_t
cfg_keystore_fromconfig(const cfg_obj_t *config, isc_mem_t *mctx,
			isc_log_t *logctx, const char *engine,
			dns_keystorelist_t *keystorelist,
			dns_keystore_t **kspp) {
	isc_result_t result;
	const cfg_obj_t *maps[2];
	const cfg_obj_t *koptions = NULL;
	const char *name = NULL;
	const char *keydirectory = DNS_KEYSTORE_KEYDIRECTORY;
	dns_keystore_t *keystore = NULL;
	int i = 0;

	if (config != NULL) {
		name = cfg_obj_asstring(cfg_tuple_get(config, "name"));
	} else {
		name = keydirectory;
	}
	INSIST(name != NULL);

	result = dns_keystorelist_find(keystorelist, name, &keystore);

	if (result == ISC_R_SUCCESS) {
		cfg_obj_log(config, logctx, ISC_LOG_ERROR,
			    "key-store: duplicate key-store found '%s'", name);
		dns_keystore_detach(&keystore);
		return ISC_R_EXISTS;
	}
	if (result != ISC_R_NOTFOUND) {
		cfg_obj_log(config, logctx, ISC_LOG_ERROR,
			    "key-store: lookup '%s' failed: %s", name,
			    isc_result_totext(result));
		return result;
	}

	/*
	 * No key-store with configured name was found in list, create new one.
	 */
	INSIST(keystore == NULL);
	result = dns_keystore_create(mctx, name, engine, &keystore);
	if (result != ISC_R_SUCCESS) {
		return result;
	}
	INSIST(keystore != NULL);

	/* Now configure. */
	INSIST(DNS_KEYSTORE_VALID(keystore));

	if (config != NULL) {
		koptions = cfg_tuple_get(config, "options");
		maps[i++] = koptions;
		maps[i] = NULL;
		dns_keystore_setdirectory(keystore,
					  get_string(maps, "directory"));
		dns_keystore_setpkcs11uri(keystore,
					  get_string(maps, "pkcs11-uri"));
	}

	/* Append it to the list for future lookups. */
	ISC_LIST_APPEND(*keystorelist, keystore, link);
	INSIST(!(ISC_LIST_EMPTY(*keystorelist)));

	/* Success: Attach the keystore to the pointer and return. */
	if (kspp != NULL) {
		INSIST(*kspp == NULL);
		dns_keystore_attach(keystore, kspp);
	}

	/* Don't detach as keystore is on '*keystorelist' */
	return ISC_R_SUCCESS;
}
