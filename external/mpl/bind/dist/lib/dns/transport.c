/*	$NetBSD: transport.c,v 1.2.2.2 2024/02/25 15:46:53 martin Exp $	*/

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

#include <isc/list.h>
#include <isc/mem.h>
#include <isc/refcount.h>
#include <isc/result.h>
#include <isc/rwlock.h>
#include <isc/util.h>

#include <dns/name.h>
#include <dns/rbt.h>
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
	dns_rbt_t *transports[DNS_TRANSPORT_COUNT];
};

typedef enum ternary { ter_none = 0, ter_true = 1, ter_false = 2 } ternary_t;

struct dns_transport {
	unsigned int magic;
	isc_refcount_t references;
	isc_mem_t *mctx;
	dns_transport_type_t type;
	struct {
		char *tlsname;
		char *certfile;
		char *keyfile;
		char *cafile;
		char *remote_hostname;
		char *ciphers;
		uint32_t protocol_versions;
		ternary_t prefer_server_ciphers;
	} tls;
	struct {
		char *endpoint;
		dns_http_mode_t mode;
	} doh;
};

static void
free_dns_transport(void *node, void *arg) {
	dns_transport_t *transport = node;

	REQUIRE(node != NULL);

	UNUSED(arg);

	dns_transport_detach(&transport);
}

static isc_result_t
list_add(dns_transport_list_t *list, const dns_name_t *name,
	 const dns_transport_type_t type, dns_transport_t *transport) {
	isc_result_t result;
	dns_rbt_t *rbt = NULL;

	RWLOCK(&list->lock, isc_rwlocktype_write);
	rbt = list->transports[type];
	INSIST(rbt != NULL);

	result = dns_rbt_addname(rbt, name, transport);

	RWUNLOCK(&list->lock, isc_rwlocktype_write);

	return (result);
}

dns_transport_type_t
dns_transport_get_type(dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return (transport->type);
}

char *
dns_transport_get_certfile(dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return (transport->tls.certfile);
}

char *
dns_transport_get_keyfile(dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return (transport->tls.keyfile);
}

char *
dns_transport_get_cafile(dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return (transport->tls.cafile);
}

char *
dns_transport_get_remote_hostname(dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return (transport->tls.remote_hostname);
}

char *
dns_transport_get_endpoint(dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return (transport->doh.endpoint);
}

dns_http_mode_t
dns_transport_get_mode(dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return (transport->doh.mode);
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

	return (transport);
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

	return (transport->tls.protocol_versions);
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
dns_transport_get_ciphers(dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return (transport->tls.ciphers);
}

char *
dns_transport_get_tlsname(dns_transport_t *transport) {
	REQUIRE(VALID_TRANSPORT(transport));

	return (transport->tls.tlsname);
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
		return (false);
	} else if (transport->tls.prefer_server_ciphers == ter_true) {
		*preferp = true;
		return (true);
	} else if (transport->tls.prefer_server_ciphers == ter_false) {
		*preferp = false;
		return (true);
	}

	UNREACHABLE();
	return false;
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
	dns_rbt_t *rbt = NULL;

	REQUIRE(VALID_TRANSPORT_LIST(list));
	REQUIRE(list->transports[type] != NULL);

	rbt = list->transports[type];

	RWLOCK(&list->lock, isc_rwlocktype_read);
	result = dns_rbt_findname(rbt, name, 0, NULL, (void *)&transport);
	if (result == ISC_R_SUCCESS) {
		isc_refcount_increment(&transport->references);
	}
	RWUNLOCK(&list->lock, isc_rwlocktype_read);

	return (transport);
}

dns_transport_list_t *
dns_transport_list_new(isc_mem_t *mctx) {
	dns_transport_list_t *list = isc_mem_get(mctx, sizeof(*list));

	*list = (dns_transport_list_t){ 0 };

	isc_rwlock_init(&list->lock, 0, 0);

	isc_mem_attach(mctx, &list->mctx);
	isc_refcount_init(&list->references, 1);

	list->magic = TRANSPORT_LIST_MAGIC;

	for (size_t type = 0; type < DNS_TRANSPORT_COUNT; type++) {
		isc_result_t result;
		result = dns_rbt_create(list->mctx, free_dns_transport, NULL,
					&list->transports[type]);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}

	return (list);
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
		if (list->transports[type] != NULL) {
			dns_rbt_destroy(&list->transports[type]);
		}
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
