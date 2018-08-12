/*	$NetBSD: ipkeylist.c,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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

#include <config.h>

#include <string.h>

#include <isc/mem.h>
#include <isc/sockaddr.h>
#include <isc/util.h>

#include <dns/ipkeylist.h>
#include <dns/name.h>

void
dns_ipkeylist_init(dns_ipkeylist_t *ipkl) {
	ipkl->count = 0;
	ipkl->allocated = 0;
	ipkl->addrs = NULL;
	ipkl->dscps = NULL;
	ipkl->keys = NULL;
	ipkl->labels = NULL;
}

void
dns_ipkeylist_clear(isc_mem_t *mctx, dns_ipkeylist_t *ipkl) {
	isc_uint32_t i;

	REQUIRE(ipkl != NULL);

	if (ipkl->allocated == 0)
		return;

	if (ipkl->addrs != NULL)
		isc_mem_put(mctx, ipkl->addrs,
			    ipkl->allocated * sizeof(isc_sockaddr_t));

	if (ipkl->dscps != NULL)
		isc_mem_put(mctx, ipkl->dscps,
			    ipkl->allocated * sizeof(isc_dscp_t));

	if (ipkl->keys != NULL) {
		for (i = 0; i < ipkl->allocated; i++) {
			if (ipkl->keys[i] == NULL)
				continue;
			if (dns_name_dynamic(ipkl->keys[i]))
				dns_name_free(ipkl->keys[i], mctx);
			isc_mem_put(mctx, ipkl->keys[i], sizeof(dns_name_t));
		}
		isc_mem_put(mctx, ipkl->keys,
			    ipkl->allocated * sizeof(dns_name_t *));
	}

	if (ipkl->labels != NULL) {
		for (i = 0; i < ipkl->allocated; i++) {
			if (ipkl->labels[i] == NULL)
				continue;
			if (dns_name_dynamic(ipkl->labels[i]))
				dns_name_free(ipkl->labels[i], mctx);
			isc_mem_put(mctx, ipkl->labels[i], sizeof(dns_name_t));
		}
		isc_mem_put(mctx, ipkl->labels,
			    ipkl->allocated * sizeof(dns_name_t *));
	}

	dns_ipkeylist_init(ipkl);
}

isc_result_t
dns_ipkeylist_copy(isc_mem_t *mctx, const dns_ipkeylist_t *src,
		   dns_ipkeylist_t *dst)
{
	isc_result_t result = ISC_R_SUCCESS;
	isc_uint32_t i;

	REQUIRE(dst != NULL);
	/* dst might be preallocated, we don't care, but it must be empty */
	REQUIRE(dst->count == 0);

	if (src->count == 0)
		return (ISC_R_SUCCESS);

	result = dns_ipkeylist_resize(mctx, dst, src->count);
	if (result != ISC_R_SUCCESS)
		return (result);

	memmove(dst->addrs, src->addrs, src->count * sizeof(isc_sockaddr_t));

	if (src->dscps != NULL) {
		memmove(dst->dscps, src->dscps,
			src->count * sizeof(isc_dscp_t));
	}

	if (src->keys != NULL) {
		for (i = 0; i < src->count; i++) {
			if (src->keys[i] != NULL) {
				dst->keys[i] = isc_mem_get(mctx,
							   sizeof(dns_name_t));
				if (dst->keys[i] == NULL) {
					result = ISC_R_NOMEMORY;
					goto cleanup_keys;
				}
				dns_name_init(dst->keys[i], NULL);
				result = dns_name_dup(src->keys[i], mctx,
						      dst->keys[i]);
				if (result != ISC_R_SUCCESS)
					goto cleanup_keys;
			} else {
				dst->keys[i] = NULL;
			}
		}
	}

	if (src->labels != NULL) {
		for (i = 0; i < src->count; i++) {
			if (src->labels[i] != NULL) {
				dst->labels[i] = isc_mem_get(mctx,
							   sizeof(dns_name_t));
				if (dst->labels[i] == NULL) {
					result = ISC_R_NOMEMORY;
					goto cleanup_labels;
				}
				dns_name_init(dst->labels[i], NULL);
				result = dns_name_dup(src->labels[i], mctx,
						      dst->labels[i]);
				if (result != ISC_R_SUCCESS)
					goto cleanup_labels;
			} else {
				dst->labels[i] = NULL;
			}
		}
	}
	dst->count = src->count;
	return (ISC_R_SUCCESS);

  cleanup_labels:
	do {
		if (dst->labels[i] != NULL) {
			if (dns_name_dynamic(dst->labels[i]))
				dns_name_free(dst->labels[i], mctx);
			isc_mem_put(mctx, dst->labels[i], sizeof(dns_name_t));
			dst->labels[i] = NULL;
		}
	} while (i-- > 0);

  cleanup_keys:
	do {
		if (dst->keys[i] != NULL) {
			if (dns_name_dynamic(dst->keys[i]))
				dns_name_free(dst->keys[i], mctx);
			isc_mem_put(mctx, dst->keys[i], sizeof(dns_name_t));
			dst->keys[i] = NULL;
		}
	} while (i-- > 0);

	return (result);
}

isc_result_t
dns_ipkeylist_resize(isc_mem_t *mctx, dns_ipkeylist_t *ipkl, unsigned int n) {
	isc_sockaddr_t *addrs = NULL;
	isc_dscp_t *dscps = NULL;
	dns_name_t **keys = NULL;
	dns_name_t **labels = NULL;

	REQUIRE(ipkl != NULL);
	REQUIRE(n > ipkl->count);

	if (n <= ipkl->allocated)
		return (ISC_R_SUCCESS);

	addrs = isc_mem_get(mctx, n * sizeof(isc_sockaddr_t));
	if (addrs == NULL)
		goto nomemory;
	dscps = isc_mem_get(mctx, n * sizeof(isc_dscp_t));
	if (dscps == NULL)
		goto nomemory;
	keys = isc_mem_get(mctx, n * sizeof(dns_name_t *));
	if (keys == NULL)
		goto nomemory;
	labels = isc_mem_get(mctx, n * sizeof(dns_name_t *));
	if (labels == NULL)
		goto nomemory;

	if (ipkl->addrs != NULL) {
		memmove(addrs, ipkl->addrs,
			ipkl->allocated * sizeof(isc_sockaddr_t));
		isc_mem_put(mctx, ipkl->addrs,
			    ipkl->allocated * sizeof(isc_sockaddr_t));
	}
	ipkl->addrs = addrs;
	memset(&ipkl->addrs[ipkl->allocated], 0,
	       (n - ipkl->allocated) * sizeof(isc_sockaddr_t));

	if (ipkl->dscps != NULL) {
		memmove(dscps, ipkl->dscps,
			ipkl->allocated * sizeof(isc_dscp_t));
		isc_mem_put(mctx, ipkl->dscps,
			    ipkl->allocated * sizeof(isc_dscp_t));
	}
	ipkl->dscps = dscps;
	memset(&ipkl->dscps[ipkl->allocated], 0,
	       (n - ipkl->allocated) * sizeof(isc_dscp_t));

	if (ipkl->keys) {
		memmove(keys, ipkl->keys,
			ipkl->allocated * sizeof(dns_name_t *));
		isc_mem_put(mctx, ipkl->keys,
			    ipkl->allocated * sizeof(dns_name_t *));
	}
	ipkl->keys = keys;
	memset(&ipkl->keys[ipkl->allocated], 0,
	       (n - ipkl->allocated) * sizeof(dns_name_t *));

	if (ipkl->labels != NULL) {
		memmove(labels, ipkl->labels,
			ipkl->allocated * sizeof(dns_name_t *));
		isc_mem_put(mctx, ipkl->labels,
			    ipkl->allocated * sizeof(dns_name_t *));
	}
	ipkl->labels = labels;
	memset(&ipkl->labels[ipkl->allocated], 0,
	       (n - ipkl->allocated) * sizeof(dns_name_t *));

	ipkl->allocated = n;
	return (ISC_R_SUCCESS);

nomemory:
	if (addrs != NULL)
		isc_mem_put(mctx, addrs, n * sizeof(isc_sockaddr_t));
	if (dscps != NULL)
		isc_mem_put(mctx, dscps, n * sizeof(isc_dscp_t));
	if (keys != NULL)
		isc_mem_put(mctx, keys, n * sizeof(dns_name_t *));
	if (labels != NULL)
		isc_mem_put(mctx, labels, n * sizeof(dns_name_t *));

	return (ISC_R_NOMEMORY);
}
