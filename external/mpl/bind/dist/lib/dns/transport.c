/*	$NetBSD: transport.c,v 1.3 2025/01/26 16:25:25 christos Exp $	*/

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

#include <isc/hashmap.h>
#include <isc/list.h>
#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/refcount.h>
#include <isc/result.h>
#include <isc/rwlock.h>
#include <isc/sockaddr.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/transport.h>

#define TRANSPORT_MAGIC	     ISC_MAGIC('T', 'r', 'n', 's')
#define VALID_TRANSPORT(ptr) ISC_MAGIC_VALID(ptr, TRANSPORT_MAGIC)

#define TRANSPORT_LIST_MAGIC	  ISC_MAGIC('T', 'r', 'L', 's')
#define VALID_TRANSPORT_LIST(ptr) ISC_MAGIC_VALID(ptr, TRANSPORT_LIST_MAGIC)

struct dns_transport_list {
	unsigned int magic;
	isc_refcount_t references;
	isc_mem_t *mctx;
	isc_rwlock_t lock;
	isc_hashmap_t *transports[DNS_TRANSPORT_COUNT];
};

typedef enum ternary { ter_none = 0, ter_true = 1, ter_false = 2 } ternary_t;

struct dns_transport {
	unsigned int magic;
	isc_refcount_t references;
	isc_mem_t *mctx;
	dns_transport_type_t type;
	dns_fixedname_t fn;
	dns_name_t *name;
	struct {
		char *tlsname;
		char *certfile;
		char *keyfile;
		char *cafile;
		char *remote_hostname;
		char *ciphers;
		char *cipher_suites;
		uint32_t protocol_versions;
		ternary_t prefer_server_ciphers;
		bool always_verify_remote;
	} tls;
	struct {
		char *endpoint;
		dns_http_mode_t mode;
	} doh;
};

static bool
transport_match(void *node, const void *key) {
	dns_transport_t *transport = node;

	return dns_name_equal(transport->name, key);
}

static isc_result_t
list_add(dns_transport_list_t *list, const dns_name_t *name,
	 const dns_transport_type_t type, dns_transport_t *transport) {
	isc_result_t result;
	isc_hashmap_t *hm = NULL;

	RWLOCK(&list->lock, isc_rwlocktype_write);
	hm = list->transports[type];
	INSIST(hm != NULL);

	transport->name = dns_fixedname_initname(&transport->fn);
	dns_name_copy(name, transport->name);
	result = isc_hashmap_add(hm, dns_name_hash(name), transport_match, name,
				 transport, NULL);
	RWUNLOCK(&list->lock, isc_rwlocktype_write);

	return result;
}

dns_transport_type_t
dns_transport_get_type(const dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return transport->type;
}

char *
dns_transport_get_certfile(const dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return transport->tls.certfile;
}

char *
dns_transport_get_keyfile(const dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return transport->tls.keyfile;
}

char *
dns_transport_get_cafile(const dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return transport->tls.cafile;
}

char *
dns_transport_get_remote_hostname(const dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return transport->tls.remote_hostname;
}

char *
dns_transport_get_endpoint(const dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return transport->doh.endpoint;
}

dns_http_mode_t
dns_transport_get_mode(const dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return transport->doh.mode;
}

dns_transport_t *
dns_transport_new(const dns_name_t *name, dns_transport_type_t type,
		  dns_transport_list_t *list) {
	dns_transport_t *transport = isc_mem_get(list->mctx,
						 sizeof(*transport));
	*transport = (dns_transport_t){ .type = type };
	isc_refcount_init(&transport->references, 1);
	isc_mem_attach(list->mctx, &transport->mctx);
	transport->magic = TRANSPORT_MAGIC;

	list_add(list, name, type, transport);

	return transport;
}

void
dns_transport_set_certfile(dns_transport_t *transport, const char *certfile) {
	REQUIRE(VALID_TRANSPORT(transport));
	REQUIRE(transport->type == DNS_TRANSPORT_TLS ||
		transport->type == DNS_TRANSPORT_HTTP);

	if (transport->tls.certfile != NULL) {
		isc_mem_free(transport->mctx, transport->tls.certfile);
	}

	if (certfile != NULL) {
		transport->tls.certfile = isc_mem_strdup(transport->mctx,
							 certfile);
	}
}

void
dns_transport_set_keyfile(dns_transport_t *transport, const char *keyfile) {
	REQUIRE(VALID_TRANSPORT(transport));
	REQUIRE(transport->type == DNS_TRANSPORT_TLS ||
		transport->type == DNS_TRANSPORT_HTTP);

	if (transport->tls.keyfile != NULL) {
		isc_mem_free(transport->mctx, transport->tls.keyfile);
	}

	if (keyfile != NULL) {
		transport->tls.keyfile = isc_mem_strdup(transport->mctx,
							keyfile);
	}
}

void
dns_transport_set_cafile(dns_transport_t *transport, const char *cafile) {
	REQUIRE(VALID_TRANSPORT(transport));
	REQUIRE(transport->type == DNS_TRANSPORT_TLS ||
		transport->type == DNS_TRANSPORT_HTTP);

	if (transport->tls.cafile != NULL) {
		isc_mem_free(transport->mctx, transport->tls.cafile);
	}

	if (cafile != NULL) {
		transport->tls.cafile = isc_mem_strdup(transport->mctx, cafile);
	}
}

void
dns_transport_set_remote_hostname(dns_transport_t *transport,
				  const char *hostname) {
	REQUIRE(VALID_TRANSPORT(transport));
	REQUIRE(transport->type == DNS_TRANSPORT_TLS ||
		transport->type == DNS_TRANSPORT_HTTP);

	if (transport->tls.remote_hostname != NULL) {
		isc_mem_free(transport->mctx, transport->tls.remote_hostname);
	}

	if (hostname != NULL) {
		transport->tls.remote_hostname = isc_mem_strdup(transport->mctx,
								hostname);
	}
}

void
dns_transport_set_endpoint(dns_transport_t *transport, const char *endpoint) {
	REQUIRE(VALID_TRANSPORT(transport));
	REQUIRE(transport->type == DNS_TRANSPORT_HTTP);

	if (transport->doh.endpoint != NULL) {
		isc_mem_free(transport->mctx, transport->doh.endpoint);
	}

	if (endpoint != NULL) {
		transport->doh.endpoint = isc_mem_strdup(transport->mctx,
							 endpoint);
	}
}

void
dns_transport_set_mode(dns_transport_t *transport, dns_http_mode_t mode) {
	REQUIRE(VALID_TRANSPORT(transport));
	REQUIRE(transport->type == DNS_TRANSPORT_HTTP);

	transport->doh.mode = mode;
}

void
dns_transport_set_tls_versions(dns_transport_t *transport,
			       const uint32_t tls_versions) {
	REQUIRE(VALID_TRANSPORT(transport));
	REQUIRE(transport->type == DNS_TRANSPORT_HTTP ||
		transport->type == DNS_TRANSPORT_TLS);

	transport->tls.protocol_versions = tls_versions;
}

uint32_t
dns_transport_get_tls_versions(const dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return transport->tls.protocol_versions;
}

void
dns_transport_set_ciphers(dns_transport_t *transport, const char *ciphers) {
	REQUIRE(VALID_TRANSPORT(transport));
	REQUIRE(transport->type == DNS_TRANSPORT_TLS ||
		transport->type == DNS_TRANSPORT_HTTP);

	if (transport->tls.ciphers != NULL) {
		isc_mem_free(transport->mctx, transport->tls.ciphers);
	}

	if (ciphers != NULL) {
		transport->tls.ciphers = isc_mem_strdup(transport->mctx,
							ciphers);
	}
}

void
dns_transport_set_tlsname(dns_transport_t *transport, const char *tlsname) {
	REQUIRE(VALID_TRANSPORT(transport));
	REQUIRE(transport->type == DNS_TRANSPORT_TLS ||
		transport->type == DNS_TRANSPORT_HTTP);

	if (transport->tls.tlsname != NULL) {
		isc_mem_free(transport->mctx, transport->tls.tlsname);
	}

	if (tlsname != NULL) {
		transport->tls.tlsname = isc_mem_strdup(transport->mctx,
							tlsname);
	}
}

char *
dns_transport_get_ciphers(const dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return transport->tls.ciphers;
}

void
dns_transport_set_cipher_suites(dns_transport_t *transport,
				const char *cipher_suites) {
	REQUIRE(VALID_TRANSPORT(transport));
	REQUIRE(transport->type == DNS_TRANSPORT_TLS ||
		transport->type == DNS_TRANSPORT_HTTP);

	if (transport->tls.cipher_suites != NULL) {
		isc_mem_free(transport->mctx, transport->tls.cipher_suites);
	}

	if (cipher_suites != NULL) {
		transport->tls.cipher_suites = isc_mem_strdup(transport->mctx,
							      cipher_suites);
	}
}

char *
dns_transport_get_cipher_suites(const dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return transport->tls.cipher_suites;
}

char *
dns_transport_get_tlsname(const dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return transport->tls.tlsname;
}

void
dns_transport_set_prefer_server_ciphers(dns_transport_t *transport,
					const bool prefer) {
	REQUIRE(VALID_TRANSPORT(transport));
	REQUIRE(transport->type == DNS_TRANSPORT_TLS ||
		transport->type == DNS_TRANSPORT_HTTP);

	transport->tls.prefer_server_ciphers = prefer ? ter_true : ter_false;
}

bool
dns_transport_get_prefer_server_ciphers(const dns_transport_t *transport,
					bool *preferp) {
	REQUIRE(VALID_TRANSPORT(transport));
	REQUIRE(preferp != NULL);
	if (transport->tls.prefer_server_ciphers == ter_none) {
		return false;
	} else if (transport->tls.prefer_server_ciphers == ter_true) {
		*preferp = true;
		return true;
	} else if (transport->tls.prefer_server_ciphers == ter_false) {
		*preferp = false;
		return true;
	}

	UNREACHABLE();
	return false;
}

void
dns_transport_set_always_verify_remote(dns_transport_t *transport,
				       const bool always_verify_remote) {
	REQUIRE(VALID_TRANSPORT(transport));
	REQUIRE(transport->type == DNS_TRANSPORT_TLS ||
		transport->type == DNS_TRANSPORT_HTTP);

	transport->tls.always_verify_remote = always_verify_remote;
}

bool
dns_transport_get_always_verify_remote(dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));
	REQUIRE(transport->type == DNS_TRANSPORT_TLS ||
		transport->type == DNS_TRANSPORT_HTTP);

	return transport->tls.always_verify_remote;
}

isc_result_t
dns_transport_get_tlsctx(dns_transport_t *transport, const isc_sockaddr_t *peer,
			 isc_tlsctx_cache_t *tlsctx_cache, isc_mem_t *mctx,
			 isc_tlsctx_t **pctx,
			 isc_tlsctx_client_session_cache_t **psess_cache) {
	isc_result_t result = ISC_R_FAILURE;
	isc_tlsctx_t *tlsctx = NULL, *found = NULL;
	isc_tls_cert_store_t *store = NULL, *found_store = NULL;
	isc_tlsctx_client_session_cache_t *sess_cache = NULL;
	isc_tlsctx_client_session_cache_t *found_sess_cache = NULL;
	uint32_t tls_versions;
	const char *ciphers = NULL;
	const char *cipher_suites = NULL;
	bool prefer_server_ciphers;
	uint16_t family;
	const char *tlsname = NULL;

	REQUIRE(VALID_TRANSPORT(transport));
	REQUIRE(transport->type == DNS_TRANSPORT_TLS);
	REQUIRE(peer != NULL);
	REQUIRE(tlsctx_cache != NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(pctx != NULL && *pctx == NULL);
	REQUIRE(psess_cache != NULL && *psess_cache == NULL);

	family = (isc_sockaddr_pf(peer) == PF_INET6) ? AF_INET6 : AF_INET;

	tlsname = dns_transport_get_tlsname(transport);
	INSIST(tlsname != NULL && *tlsname != '\0');

	/*
	 * Let's try to re-use the already created context. This way
	 * we have a chance to resume the TLS session, bypassing the
	 * full TLS handshake procedure, making establishing
	 * subsequent TLS connections faster.
	 */
	result = isc_tlsctx_cache_find(tlsctx_cache, tlsname,
				       isc_tlsctx_cache_tls, family, &found,
				       &found_store, &found_sess_cache);
	if (result != ISC_R_SUCCESS) {
		const char *hostname =
			dns_transport_get_remote_hostname(transport);
		const char *ca_file = dns_transport_get_cafile(transport);
		const char *cert_file = dns_transport_get_certfile(transport);
		const char *key_file = dns_transport_get_keyfile(transport);
		const bool always_verify_remote =
			dns_transport_get_always_verify_remote(transport);
		char peer_addr_str[INET6_ADDRSTRLEN] = { 0 };
		isc_netaddr_t peer_netaddr = { 0 };
		bool hostname_ignore_subject;

		/*
		 * So, no context exists. Let's create one using the
		 * parameters from the configuration file and try to
		 * store it for further reuse.
		 */
		result = isc_tlsctx_createclient(&tlsctx);
		if (result != ISC_R_SUCCESS) {
			goto failure;
		}
		tls_versions = dns_transport_get_tls_versions(transport);
		if (tls_versions != 0) {
			isc_tlsctx_set_protocols(tlsctx, tls_versions);
		}
		ciphers = dns_transport_get_ciphers(transport);
		if (ciphers != NULL) {
			isc_tlsctx_set_cipherlist(tlsctx, ciphers);
		}
		cipher_suites = dns_transport_get_cipher_suites(transport);
		if (cipher_suites != NULL) {
			isc_tlsctx_set_cipher_suites(tlsctx, cipher_suites);
		}

		if (dns_transport_get_prefer_server_ciphers(
			    transport, &prefer_server_ciphers))
		{
			isc_tlsctx_prefer_server_ciphers(tlsctx,
							 prefer_server_ciphers);
		}

		if (always_verify_remote || hostname != NULL || ca_file != NULL)
		{
			/*
			 * The situation when 'found_store != NULL' while
			 * 'found == NULL' may occur as there is a one-to-many
			 * relation between cert stores and per-transport TLS
			 * contexts. That is, there could be one store
			 * shared between multiple contexts.
			 */
			if (found_store == NULL) {
				/*
				 * 'ca_file' can equal 'NULL' here, in
				 * which case the store with system-wide
				 * CA certificates will be created.
				 */
				result = isc_tls_cert_store_create(ca_file,
								   &store);

				if (result != ISC_R_SUCCESS) {
					goto failure;
				}
			} else {
				store = found_store;
			}

			INSIST(store != NULL);
			if (hostname == NULL) {
				/*
				 * If hostname is not specified, then use the
				 * peer IP address for validation.
				 */
				isc_netaddr_fromsockaddr(&peer_netaddr, peer);
				isc_netaddr_format(&peer_netaddr, peer_addr_str,
						   sizeof(peer_addr_str));
				hostname = peer_addr_str;
			}

			/*
			 * According to RFC 8310, Subject field MUST NOT
			 * be inspected when verifying hostname for DoT.
			 * Only SubjectAltName must be checked.
			 */
			hostname_ignore_subject = true;
			result = isc_tlsctx_enable_peer_verification(
				tlsctx, false, store, hostname,
				hostname_ignore_subject);
			if (result != ISC_R_SUCCESS) {
				goto failure;
			}

			/*
			 * Let's load client certificate and enable
			 * Mutual TLS. We do that only in the case when
			 * Strict TLS is enabled, because Mutual TLS is
			 * an extension of it.
			 */
			if (cert_file != NULL) {
				INSIST(key_file != NULL);

				result = isc_tlsctx_load_certificate(
					tlsctx, key_file, cert_file);
				if (result != ISC_R_SUCCESS) {
					goto failure;
				}
			}
		}

		isc_tlsctx_enable_dot_client_alpn(tlsctx);

		isc_tlsctx_client_session_cache_create(
			mctx, tlsctx,
			ISC_TLSCTX_CLIENT_SESSION_CACHE_DEFAULT_SIZE,
			&sess_cache);

		found_store = NULL;
		result = isc_tlsctx_cache_add(tlsctx_cache, tlsname,
					      isc_tlsctx_cache_tls, family,
					      tlsctx, store, sess_cache, &found,
					      &found_store, &found_sess_cache);
		if (result == ISC_R_EXISTS) {
			/*
			 * It seems the entry has just been created from
			 * within another thread while we were initialising
			 * ours. Although this is unlikely, it could happen
			 * after startup/re-initialisation. In such a case,
			 * discard the new context and associated data and use
			 * the already established one from now on.
			 *
			 * Such situation will not occur after the
			 * initial 'warm-up', so it is not critical
			 * performance-wise.
			 */
			INSIST(found != NULL);
			isc_tlsctx_free(&tlsctx);
			/*
			 * The 'store' variable can be 'NULL' when remote server
			 * verification is not enabled (that is, when Strict or
			 * Mutual TLS are not used).
			 *
			 * The 'found_store' might be equal to 'store' as there
			 * is one-to-many relation between a store and
			 * per-transport TLS contexts. In that case, the call to
			 * 'isc_tlsctx_cache_find()' above could have returned a
			 * store via the 'found_store' variable, whose value we
			 * can assign to 'store' later. In that case,
			 * 'isc_tlsctx_cache_add()' will return the same value.
			 * When that happens, we should not free the store
			 * object, as it is managed by the TLS context cache.
			 */
			if (store != NULL && store != found_store) {
				isc_tls_cert_store_free(&store);
			}
			isc_tlsctx_client_session_cache_detach(&sess_cache);
			/* Let's return the data from the cache. */
			*psess_cache = found_sess_cache;
			*pctx = found;
		} else {
			/*
			 * Adding the fresh values into the cache has been
			 * successful, let's return them
			 */
			INSIST(result == ISC_R_SUCCESS);
			*psess_cache = sess_cache;
			*pctx = tlsctx;
		}
	} else {
		/*
		 * The cache lookup has been successful, let's return the
		 * results.
		 */
		INSIST(result == ISC_R_SUCCESS);
		*psess_cache = found_sess_cache;
		*pctx = found;
	}

	return ISC_R_SUCCESS;

failure:
	if (tlsctx != NULL) {
		isc_tlsctx_free(&tlsctx);
	}

	/*
	 * The 'found_store' is being managed by the TLS context
	 * cache. Thus, we should keep it as it is, as it will get
	 * destroyed alongside the cache. As there is one store per
	 * multiple TLS contexts, we need to handle store deletion in a
	 * special way.
	 */
	if (store != NULL && store != found_store) {
		isc_tls_cert_store_free(&store);
	}

	return result;
}

static void
transport_destroy(dns_transport_t *transport) {
	isc_refcount_destroy(&transport->references);
	transport->magic = 0;

	if (transport->doh.endpoint != NULL) {
		isc_mem_free(transport->mctx, transport->doh.endpoint);
	}
	if (transport->tls.remote_hostname != NULL) {
		isc_mem_free(transport->mctx, transport->tls.remote_hostname);
	}
	if (transport->tls.cafile != NULL) {
		isc_mem_free(transport->mctx, transport->tls.cafile);
	}
	if (transport->tls.keyfile != NULL) {
		isc_mem_free(transport->mctx, transport->tls.keyfile);
	}
	if (transport->tls.certfile != NULL) {
		isc_mem_free(transport->mctx, transport->tls.certfile);
	}
	if (transport->tls.ciphers != NULL) {
		isc_mem_free(transport->mctx, transport->tls.ciphers);
	}
	if (transport->tls.cipher_suites != NULL) {
		isc_mem_free(transport->mctx, transport->tls.cipher_suites);
	}

	if (transport->tls.tlsname != NULL) {
		isc_mem_free(transport->mctx, transport->tls.tlsname);
	}

	isc_mem_putanddetach(&transport->mctx, transport, sizeof(*transport));
}

void
dns_transport_attach(dns_transport_t *source, dns_transport_t **targetp) {
	REQUIRE(source != NULL);
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);

	*targetp = source;
}

void
dns_transport_detach(dns_transport_t **transportp) {
	dns_transport_t *transport = NULL;

	REQUIRE(transportp != NULL);
	REQUIRE(VALID_TRANSPORT(*transportp));

	transport = *transportp;
	*transportp = NULL;

	if (isc_refcount_decrement(&transport->references) == 1) {
		transport_destroy(transport);
	}
}

dns_transport_t *
dns_transport_find(const dns_transport_type_t type, const dns_name_t *name,
		   dns_transport_list_t *list) {
	isc_result_t result;
	dns_transport_t *transport = NULL;
	isc_hashmap_t *hm = NULL;

	REQUIRE(VALID_TRANSPORT_LIST(list));
	REQUIRE(list->transports[type] != NULL);

	hm = list->transports[type];

	RWLOCK(&list->lock, isc_rwlocktype_read);
	result = isc_hashmap_find(hm, dns_name_hash(name), transport_match,
				  name, (void **)&transport);
	if (result == ISC_R_SUCCESS) {
		isc_refcount_increment(&transport->references);
	}
	RWUNLOCK(&list->lock, isc_rwlocktype_read);

	return transport;
}

dns_transport_list_t *
dns_transport_list_new(isc_mem_t *mctx) {
	dns_transport_list_t *list = isc_mem_get(mctx, sizeof(*list));

	*list = (dns_transport_list_t){ 0 };

	isc_rwlock_init(&list->lock);

	isc_mem_attach(mctx, &list->mctx);
	isc_refcount_init(&list->references, 1);

	list->magic = TRANSPORT_LIST_MAGIC;

	for (size_t type = 0; type < DNS_TRANSPORT_COUNT; type++) {
		isc_hashmap_create(list->mctx, 10, &list->transports[type]);
	}

	return list;
}

void
dns_transport_list_attach(dns_transport_list_t *source,
			  dns_transport_list_t **targetp) {
	REQUIRE(VALID_TRANSPORT_LIST(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);

	*targetp = source;
}

static void
transport_list_destroy(dns_transport_list_t *list) {
	isc_refcount_destroy(&list->references);
	list->magic = 0;

	for (size_t type = 0; type < DNS_TRANSPORT_COUNT; type++) {
		isc_result_t result;
		isc_hashmap_iter_t *it = NULL;

		if (list->transports[type] == NULL) {
			continue;
		}

		isc_hashmap_iter_create(list->transports[type], &it);
		for (result = isc_hashmap_iter_first(it);
		     result == ISC_R_SUCCESS;
		     result = isc_hashmap_iter_delcurrent_next(it))
		{
			dns_transport_t *transport = NULL;
			isc_hashmap_iter_current(it, (void **)&transport);
			dns_transport_detach(&transport);
		}
		isc_hashmap_iter_destroy(&it);
		isc_hashmap_destroy(&list->transports[type]);
	}
	isc_rwlock_destroy(&list->lock);
	isc_mem_putanddetach(&list->mctx, list, sizeof(*list));
}

void
dns_transport_list_detach(dns_transport_list_t **listp) {
	dns_transport_list_t *list = NULL;

	REQUIRE(listp != NULL);
	REQUIRE(VALID_TRANSPORT_LIST(*listp));

	list = *listp;
	*listp = NULL;

	if (isc_refcount_decrement(&list->references) == 1) {
		transport_list_destroy(list);
	}
}
