/*	$NetBSD: listenlist.c,v 1.6.2.1 2024/02/25 15:47:34 martin Exp $	*/

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

#include <stdbool.h>

#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/util.h>

#include <dns/acl.h>

#include <ns/listenlist.h>

static void
destroy(ns_listenlist_t *list);

static isc_result_t
listenelt_create(isc_mem_t *mctx, in_port_t port, dns_acl_t *acl,
		 const uint16_t family, const bool is_http, bool tls,
		 const ns_listen_tls_params_t *tls_params,
		 isc_tlsctx_cache_t *tlsctx_cache, ns_listenelt_t **target) {
	ns_listenelt_t *elt = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	isc_tlsctx_t *sslctx = NULL;
	isc_tls_cert_store_t *store = NULL, *found_store = NULL;

	REQUIRE(target != NULL && *target == NULL);
	REQUIRE(!tls || (tls_params != NULL && tlsctx_cache != NULL));

	if (tls) {
		const isc_tlsctx_cache_transport_t transport =
			is_http ? isc_tlsctx_cache_https : isc_tlsctx_cache_tls;

		/*
		 * Let's try to reuse the existing context from the cache in
		 * order to avoid excessive TLS contexts creation.
		 */
		result = isc_tlsctx_cache_find(tlsctx_cache, tls_params->name,
					       transport, family, &sslctx,
					       &found_store, NULL);
		if (result != ISC_R_SUCCESS) {
			/*
			 * The lookup failed, let's try to create a new context
			 * and store it within the cache.
			 */
			INSIST(tls_params->name != NULL &&
			       *tls_params->name != '\0');

			result = isc_tlsctx_createserver(
				tls_params->key, tls_params->cert, &sslctx);
			if (result != ISC_R_SUCCESS) {
				goto tls_error;
			}

			/*
			 * We need to initialise session ID context to make TLS
			 * session resumption work correctly - in particular in
			 * the case when client certificates are used (Mutual
			 * TLS) - otherwise resumption attempts will lead to
			 * handshake failures. See OpenSSL documentation for
			 * 'SSL_CTX_set_session_id_context()', the "Warnings"
			 * section.
			 */
			isc_tlsctx_set_random_session_id_context(sslctx);

			/*
			 * If CA-bundle file is specified - enable client
			 * certificates validation.
			 */
			if (tls_params->ca_file != NULL) {
				if (found_store == NULL) {
					result = isc_tls_cert_store_create(
						tls_params->ca_file, &store);
					if (result != ISC_R_SUCCESS) {
						goto tls_error;
					}
				} else {
					store = found_store;
				}

				result = isc_tlsctx_enable_peer_verification(
					sslctx, true, store, NULL, false);
				if (result != ISC_R_SUCCESS) {
					goto tls_error;
				}

				/*
				 * Load the list of allowed client certificate
				 * issuers to send to TLS clients.
				 */
				result = isc_tlsctx_load_client_ca_names(
					sslctx, tls_params->ca_file);
				if (result != ISC_R_SUCCESS) {
					goto tls_error;
				}
			}

			if (tls_params->protocols != 0) {
				isc_tlsctx_set_protocols(sslctx,
							 tls_params->protocols);
			}

			if (tls_params->dhparam_file != NULL) {
				if (!isc_tlsctx_load_dhparams(
					    sslctx, tls_params->dhparam_file))
				{
					result = ISC_R_FAILURE;
					goto tls_error;
				}
			}

			if (tls_params->ciphers != NULL) {
				isc_tlsctx_set_cipherlist(sslctx,
							  tls_params->ciphers);
			}

			if (tls_params->prefer_server_ciphers_set) {
				isc_tlsctx_prefer_server_ciphers(
					sslctx,
					tls_params->prefer_server_ciphers);
			}

			if (tls_params->session_tickets_set) {
				isc_tlsctx_session_tickets(
					sslctx, tls_params->session_tickets);
			}

#ifdef HAVE_LIBNGHTTP2
			if (is_http) {
				isc_tlsctx_enable_http2server_alpn(sslctx);
			}
#endif /* HAVE_LIBNGHTTP2 */

			if (!is_http) {
				isc_tlsctx_enable_dot_server_alpn(sslctx);
			}

			/*
			 * The storing in the cache should not fail because the
			 * (re)initialisation happens from within a single
			 * thread.
			 *
			 * Taking into account that the most recent call to
			 * 'isc_tlsctx_cache_find()' has failed, it means that
			 * the TLS context has not been found. Considering that
			 * the initialisation happens from within the context of
			 * a single thread, the call to 'isc_tlsctx_cache_add()'
			 * is expected not to fail.
			 */
			RUNTIME_CHECK(isc_tlsctx_cache_add(
					      tlsctx_cache, tls_params->name,
					      transport, family, sslctx, store,
					      NULL, NULL, NULL,
					      NULL) == ISC_R_SUCCESS);
		} else {
			INSIST(sslctx != NULL);
		}
	}

	elt = isc_mem_get(mctx, sizeof(*elt));
	elt->mctx = mctx;
	ISC_LINK_INIT(elt, link);
	elt->port = port;
	elt->is_http = false;
	elt->acl = acl;
	elt->sslctx = sslctx;
	elt->sslctx_cache = NULL;
	if (sslctx != NULL && tlsctx_cache != NULL) {
		isc_tlsctx_cache_attach(tlsctx_cache, &elt->sslctx_cache);
	}
	elt->http_endpoints = NULL;
	elt->http_endpoints_number = 0;
	elt->http_max_clients = 0;
	elt->max_concurrent_streams = 0;

	*target = elt;
	return (ISC_R_SUCCESS);
tls_error:
	if (sslctx != NULL) {
		isc_tlsctx_free(&sslctx);
	}

	if (store != NULL && store != found_store) {
		isc_tls_cert_store_free(&store);
	}
	return (result);
}

isc_result_t
ns_listenelt_create(isc_mem_t *mctx, in_port_t port, dns_acl_t *acl,
		    const uint16_t family, bool tls,
		    const ns_listen_tls_params_t *tls_params,
		    isc_tlsctx_cache_t *tlsctx_cache, ns_listenelt_t **target) {
	return listenelt_create(mctx, port, acl, family, false, tls, tls_params,
				tlsctx_cache, target);
}

isc_result_t
ns_listenelt_create_http(isc_mem_t *mctx, in_port_t http_port, dns_acl_t *acl,
			 const uint16_t family, bool tls,
			 const ns_listen_tls_params_t *tls_params,
			 isc_tlsctx_cache_t *tlsctx_cache, char **endpoints,
			 size_t nendpoints, const uint32_t max_clients,
			 const uint32_t max_streams, ns_listenelt_t **target) {
	isc_result_t result;

	REQUIRE(target != NULL && *target == NULL);
	REQUIRE(endpoints != NULL && *endpoints != NULL);
	REQUIRE(nendpoints > 0);

	result = listenelt_create(mctx, http_port, acl, family, true, tls,
				  tls_params, tlsctx_cache, target);
	if (result == ISC_R_SUCCESS) {
		(*target)->is_http = true;
		(*target)->http_endpoints = endpoints;
		(*target)->http_endpoints_number = nendpoints;
		/*
		 * 0 sized quota - means unlimited quota. We used to not
		 * create a quota object in such a case, but we might need to
		 * update the value of the quota during reconfiguration, so we
		 * need to have a quota object in place anyway.
		 */
		(*target)->http_max_clients = max_clients == 0 ? UINT32_MAX
							       : max_clients;
		(*target)->max_concurrent_streams = max_streams;
	} else {
		size_t i;
		for (i = 0; i < nendpoints; i++) {
			isc_mem_free(mctx, endpoints[i]);
		}
		isc_mem_free(mctx, endpoints);
	}
	return (result);
}

void
ns_listenelt_destroy(ns_listenelt_t *elt) {
	if (elt->acl != NULL) {
		dns_acl_detach(&elt->acl);
	}

	elt->sslctx = NULL; /* this one is going to be destroyed alongside the
			       sslctx_cache */
	if (elt->sslctx_cache != NULL) {
		isc_tlsctx_cache_detach(&elt->sslctx_cache);
	}
	if (elt->http_endpoints != NULL) {
		size_t i;
		INSIST(elt->http_endpoints_number > 0);
		for (i = 0; i < elt->http_endpoints_number; i++) {
			isc_mem_free(elt->mctx, elt->http_endpoints[i]);
		}
		isc_mem_free(elt->mctx, elt->http_endpoints);
	}
	isc_mem_put(elt->mctx, elt, sizeof(*elt));
}

isc_result_t
ns_listenlist_create(isc_mem_t *mctx, ns_listenlist_t **target) {
	ns_listenlist_t *list = NULL;
	REQUIRE(target != NULL && *target == NULL);
	list = isc_mem_get(mctx, sizeof(*list));
	list->mctx = mctx;
	list->refcount = 1;
	ISC_LIST_INIT(list->elts);
	*target = list;
	return (ISC_R_SUCCESS);
}

static void
destroy(ns_listenlist_t *list) {
	ns_listenelt_t *elt, *next;
	for (elt = ISC_LIST_HEAD(list->elts); elt != NULL; elt = next) {
		next = ISC_LIST_NEXT(elt, link);
		ns_listenelt_destroy(elt);
	}
	isc_mem_put(list->mctx, list, sizeof(*list));
}

void
ns_listenlist_attach(ns_listenlist_t *source, ns_listenlist_t **target) {
	INSIST(source->refcount > 0);
	source->refcount++;
	*target = source;
}

void
ns_listenlist_detach(ns_listenlist_t **listp) {
	ns_listenlist_t *list = *listp;
	*listp = NULL;
	INSIST(list->refcount > 0);
	list->refcount--;
	if (list->refcount == 0) {
		destroy(list);
	}
}

isc_result_t
ns_listenlist_default(isc_mem_t *mctx, in_port_t port, bool enabled,
		      const uint16_t family, ns_listenlist_t **target) {
	isc_result_t result;
	dns_acl_t *acl = NULL;
	ns_listenelt_t *elt = NULL;
	ns_listenlist_t *list = NULL;

	REQUIRE(target != NULL && *target == NULL);
	if (enabled) {
		result = dns_acl_any(mctx, &acl);
	} else {
		result = dns_acl_none(mctx, &acl);
	}
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = ns_listenelt_create(mctx, port, acl, family, false, NULL, NULL,
				     &elt);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_acl;
	}

	result = ns_listenlist_create(mctx, &list);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_listenelt;
	}

	ISC_LIST_APPEND(list->elts, elt, link);

	*target = list;
	return (ISC_R_SUCCESS);

cleanup_listenelt:
	ns_listenelt_destroy(elt);
cleanup_acl:
	dns_acl_detach(&acl);
cleanup:
	return (result);
}
