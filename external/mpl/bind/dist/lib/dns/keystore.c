/*	$NetBSD: keystore.c,v 1.1.1.1 2025/01/26 16:12:33 christos Exp $	*/

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

#include <string.h>

#include <isc/assertions.h>
#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/keystore.h>
#include <dns/keyvalues.h>

isc_result_t
dns_keystore_create(isc_mem_t *mctx, const char *name, const char *engine,
		    dns_keystore_t **kspp) {
	dns_keystore_t *keystore;

	REQUIRE(name != NULL);
	REQUIRE(kspp != NULL && *kspp == NULL);

	keystore = isc_mem_get(mctx, sizeof(*keystore));
	keystore->engine = engine;
	keystore->mctx = NULL;
	isc_mem_attach(mctx, &keystore->mctx);

	keystore->name = isc_mem_strdup(mctx, name);
	isc_mutex_init(&keystore->lock);

	isc_refcount_init(&keystore->references, 1);

	ISC_LINK_INIT(keystore, link);

	keystore->directory = NULL;
	keystore->pkcs11uri = NULL;

	keystore->magic = DNS_KEYSTORE_MAGIC;
	*kspp = keystore;

	return ISC_R_SUCCESS;
}

static inline void
dns__keystore_destroy(dns_keystore_t *keystore) {
	char *name;

	REQUIRE(!ISC_LINK_LINKED(keystore, link));

	isc_mutex_destroy(&keystore->lock);
	name = UNCONST(keystore->name);
	isc_mem_free(keystore->mctx, name);
	if (keystore->directory != NULL) {
		isc_mem_free(keystore->mctx, keystore->directory);
	}
	if (keystore->pkcs11uri != NULL) {
		isc_mem_free(keystore->mctx, keystore->pkcs11uri);
	}
	isc_mem_putanddetach(&keystore->mctx, keystore, sizeof(*keystore));
}

#ifdef DNS_KEYSTORE_TRACE
ISC_REFCOUNT_TRACE_IMPL(dns_keystore, dns__keystore_destroy);
#else
ISC_REFCOUNT_IMPL(dns_keystore, dns__keystore_destroy);
#endif

const char *
dns_keystore_name(dns_keystore_t *keystore) {
	REQUIRE(DNS_KEYSTORE_VALID(keystore));

	return keystore->name;
}

const char *
dns_keystore_engine(dns_keystore_t *keystore) {
	REQUIRE(DNS_KEYSTORE_VALID(keystore));

	return keystore->engine;
}

const char *
dns_keystore_directory(dns_keystore_t *keystore, const char *keydir) {
	if (keystore == NULL) {
		return keydir;
	}

	INSIST(DNS_KEYSTORE_VALID(keystore));

	if (keystore->directory == NULL) {
		return keydir;
	}

	return keystore->directory;
}

void
dns_keystore_setdirectory(dns_keystore_t *keystore, const char *dir) {
	REQUIRE(DNS_KEYSTORE_VALID(keystore));

	if (keystore->directory != NULL) {
		isc_mem_free(keystore->mctx, keystore->directory);
	}
	keystore->directory = (dir == NULL)
				      ? NULL
				      : isc_mem_strdup(keystore->mctx, dir);
}

const char *
dns_keystore_pkcs11uri(dns_keystore_t *keystore) {
	REQUIRE(DNS_KEYSTORE_VALID(keystore));

	return keystore->pkcs11uri;
}

void
dns_keystore_setpkcs11uri(dns_keystore_t *keystore, const char *uri) {
	REQUIRE(DNS_KEYSTORE_VALID(keystore));

	if (keystore->pkcs11uri != NULL) {
		isc_mem_free(keystore->mctx, keystore->pkcs11uri);
	}
	keystore->pkcs11uri = (uri == NULL)
				      ? NULL
				      : isc_mem_strdup(keystore->mctx, uri);
}

static isc_result_t
buildpkcs11label(const char *uri, const dns_name_t *zname, const char *policy,
		 int flags, isc_buffer_t *buf) {
	bool ksk = ((flags & DNS_KEYFLAG_KSK) != 0);
	char timebuf[18];
	isc_time_t now = isc_time_now();
	isc_result_t result;
	dns_fixedname_t fname;
	dns_name_t *pname = dns_fixedname_initname(&fname);

	/* uri + object */
	if (isc_buffer_availablelength(buf) < strlen(uri) + strlen(";object="))
	{
		return ISC_R_NOSPACE;
	}
	isc_buffer_putstr(buf, uri);
	isc_buffer_putstr(buf, ";object=");
	/* zone name */
	result = dns_name_tofilenametext(zname, false, buf);
	if (result != ISC_R_SUCCESS) {
		return result;
	}
	/*
	 * policy name
	 *
	 * Note that strlen(policy) is not the actual length, but if this
	 * already does not fit, the escaped version returned from
	 * dns_name_tofilenametext() certainly won't fit.
	 */
	if (isc_buffer_availablelength(buf) < (strlen(policy) + 1)) {
		return ISC_R_NOSPACE;
	}
	isc_buffer_putstr(buf, "-");
	result = dns_name_fromstring(pname, policy, dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		return result;
	}
	result = dns_name_tofilenametext(pname, false, buf);
	if (result != ISC_R_SUCCESS) {
		return result;
	}
	/* key type + current time */
	isc_time_formatshorttimestamp(&now, timebuf, sizeof(timebuf));
	return isc_buffer_printf(buf, "-%s-%s", ksk ? "ksk" : "zsk", timebuf);
}

isc_result_t
dns_keystore_keygen(dns_keystore_t *keystore, const dns_name_t *origin,
		    const char *policy, dns_rdataclass_t rdclass,
		    isc_mem_t *mctx, uint32_t alg, int size, int flags,
		    dst_key_t **dstkey) {
	isc_result_t result;
	dst_key_t *newkey = NULL;
	const char *uri = NULL;

	REQUIRE(DNS_KEYSTORE_VALID(keystore));
	REQUIRE(dns_name_isvalid(origin));
	REQUIRE(policy != NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(dstkey != NULL && *dstkey == NULL);

	uri = dns_keystore_pkcs11uri(keystore);
	if (uri != NULL) {
		/*
		 * Create the PKCS#11 label.
		 * The label consists of the configured URI, and the object
		 * parameter.  The object parameter needs to be unique.  We
		 * know that for a given point in time, there will be at most
		 * one key per type created for each zone in a given DNSSEC
		 * policy.  Hence the object is constructed out of the following
		 * parts: the zone name, policy name, key type, and the
		 * current time.
		 *
		 * The object may not contain any characters that conflict with
		 * special characters in the PKCS#11 URI scheme syntax (see
		 * RFC 7512, Section 2.3). Therefore, we mangle the zone name
		 * and policy name through 'dns_name_tofilenametext()'. We
		 * could create a new function to convert a name to PKCS#11
		 * text, but this existing function will suffice.
		 */
		char label[NAME_MAX];
		isc_buffer_t buf;
		isc_buffer_init(&buf, label, sizeof(label));
		result = buildpkcs11label(uri, origin, policy, flags, &buf);
		if (result != ISC_R_SUCCESS) {
			char namebuf[DNS_NAME_FORMATSIZE];
			dns_name_format(origin, namebuf, sizeof(namebuf));
			isc_log_write(
				dns_lctx, DNS_LOGCATEGORY_DNSSEC,
				DNS_LOGMODULE_DNSSEC, ISC_LOG_ERROR,
				"keystore: failed to create PKCS#11 object "
				"for zone %s, policy %s: %s",
				namebuf, policy, isc_result_totext(result));
			return result;
		}

		/* Generate the key */
		result = dst_key_generate(origin, alg, size, 0, flags,
					  DNS_KEYPROTO_DNSSEC, rdclass, label,
					  mctx, &newkey, NULL);

		if (result != ISC_R_SUCCESS) {
			isc_log_write(
				dns_lctx, DNS_LOGCATEGORY_DNSSEC,
				DNS_LOGMODULE_DNSSEC, ISC_LOG_ERROR,
				"keystore: failed to generate PKCS#11 object "
				"%s: %s",
				label, isc_result_totext(result));
			return result;
		}
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
			      DNS_LOGMODULE_DNSSEC, ISC_LOG_ERROR,
			      "keystore: generated PKCS#11 object %s", label);
	} else {
		result = dst_key_generate(origin, alg, size, 0, flags,
					  DNS_KEYPROTO_DNSSEC, rdclass, NULL,
					  mctx, &newkey, NULL);
	}

	if (result == ISC_R_SUCCESS) {
		*dstkey = newkey;
	}
	return result;
}

isc_result_t
dns_keystorelist_find(dns_keystorelist_t *list, const char *name,
		      dns_keystore_t **kspp) {
	dns_keystore_t *keystore = NULL;

	REQUIRE(kspp != NULL && *kspp == NULL);

	if (list == NULL) {
		return ISC_R_NOTFOUND;
	}

	for (keystore = ISC_LIST_HEAD(*list); keystore != NULL;
	     keystore = ISC_LIST_NEXT(keystore, link))
	{
		if (strcmp(keystore->name, name) == 0) {
			break;
		}
	}

	if (keystore == NULL) {
		return ISC_R_NOTFOUND;
	}

	dns_keystore_attach(keystore, kspp);
	return ISC_R_SUCCESS;
}
