/*	$NetBSD: server.c,v 1.1.1.1 2018/08/12 12:08:07 christos Exp $	*/

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

#include <isc/mem.h>
#include <isc/stats.h>
#include <isc/util.h>

#include <dns/tkey.h>
#include <dns/stats.h>

#include <ns/server.h>
#include <ns/stats.h>

#define SCTX_MAGIC		ISC_MAGIC('S','c','t','x')
#define SCTX_VALID(s)		ISC_MAGIC_VALID(s, SCTX_MAGIC)

#define CHECKFATAL(op) 						\
	do { result = (op);					\
		RUNTIME_CHECK(result == ISC_R_SUCCESS);		\
	} while (0)						\

isc_result_t
ns_server_create(isc_mem_t *mctx, isc_entropy_t *entropy,
		 ns_matchview_t matchingview, ns_server_t **sctxp)
{
	ns_server_t *sctx;
	isc_result_t result;

	REQUIRE(sctxp != NULL && *sctxp == NULL);

	sctx = isc_mem_get(mctx, sizeof(*sctx));
	if (sctx == NULL)
		return (ISC_R_NOMEMORY);

	memset(sctx, 0, sizeof(*sctx));

	isc_mem_attach(mctx, &sctx->mctx);

	result = isc_refcount_init(&sctx->references, 1);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	CHECKFATAL(isc_quota_init(&sctx->xfroutquota, 10));
	CHECKFATAL(isc_quota_init(&sctx->tcpquota, 10));
	CHECKFATAL(isc_quota_init(&sctx->recursionquota, 100));

	CHECKFATAL(dns_tkeyctx_create(mctx, entropy, &sctx->tkeyctx));
	CHECKFATAL(isc_rng_create(mctx, entropy, &sctx->rngctx));

	CHECKFATAL(ns_stats_create(mctx, ns_statscounter_max, &sctx->nsstats));

	CHECKFATAL(dns_rdatatypestats_create(mctx, &sctx->rcvquerystats));

	CHECKFATAL(dns_opcodestats_create(mctx, &sctx->opcodestats));

	CHECKFATAL(dns_rcodestats_create(mctx, &sctx->rcodestats));

	CHECKFATAL(isc_stats_create(mctx, &sctx->udpinstats4,
				    dns_sizecounter_in_max));

	CHECKFATAL(isc_stats_create(mctx, &sctx->udpoutstats4,
				    dns_sizecounter_out_max));

	CHECKFATAL(isc_stats_create(mctx, &sctx->udpinstats6,
				    dns_sizecounter_in_max));

	CHECKFATAL(isc_stats_create(mctx, &sctx->udpoutstats6,
				    dns_sizecounter_out_max));

	CHECKFATAL(isc_stats_create(mctx, &sctx->tcpinstats4,
				    dns_sizecounter_in_max));

	CHECKFATAL(isc_stats_create(mctx, &sctx->tcpoutstats4,
				    dns_sizecounter_out_max));

	CHECKFATAL(isc_stats_create(mctx, &sctx->tcpinstats6,
				    dns_sizecounter_in_max));

	CHECKFATAL(isc_stats_create(mctx, &sctx->tcpoutstats6,
				    dns_sizecounter_out_max));

	sctx->initialtimo = 300;
	sctx->idletimo = 300;
	sctx->keepalivetimo = 300;
	sctx->advertisedtimo = 300;

	sctx->udpsize = 4096;
	sctx->transfer_tcp_message_size = 20480;

	sctx->fuzztype = isc_fuzz_none;
	sctx->fuzznotify = NULL;
	sctx->gethostname = NULL;

	sctx->matchingview = matchingview;
	sctx->answercookie = ISC_TRUE;

	ISC_LIST_INIT(sctx->altsecrets);

	sctx->magic = SCTX_MAGIC;
	*sctxp = sctx;

	return (ISC_R_SUCCESS);

 cleanup:
	isc_mem_putanddetach(&sctx->mctx, sctx, sizeof(*sctx));

	return (result);
}

void
ns_server_attach(ns_server_t *src, ns_server_t **dest) {
	REQUIRE(SCTX_VALID(src));
	REQUIRE(dest != NULL && *dest == NULL);

	isc_refcount_increment(&src->references, NULL);

	*dest = src;
}

void
ns_server_detach(ns_server_t **sctxp) {
	ns_server_t *sctx;
	unsigned int refs;

	REQUIRE(sctxp != NULL);
	sctx = *sctxp;
	REQUIRE(SCTX_VALID(sctx));

	isc_refcount_decrement(&sctx->references, &refs);
	if (refs == 0) {
		ns_altsecret_t *altsecret;

		sctx->magic = 0;

		while ((altsecret = ISC_LIST_HEAD(sctx->altsecrets)) != NULL) {
			ISC_LIST_UNLINK(sctx->altsecrets, altsecret, link);
			isc_mem_put(sctx->mctx, altsecret, sizeof(*altsecret));
		}

		isc_quota_destroy(&sctx->recursionquota);
		isc_quota_destroy(&sctx->tcpquota);
		isc_quota_destroy(&sctx->xfroutquota);

		if (sctx->server_id != NULL)
			isc_mem_free(sctx->mctx, sctx->server_id);

		if (sctx->blackholeacl != NULL)
			dns_acl_detach(&sctx->blackholeacl);
		if (sctx->keepresporder != NULL)
			dns_acl_detach(&sctx->keepresporder);
		if (sctx->rngctx != NULL)
			isc_rng_detach(&sctx->rngctx);
		if (sctx->tkeyctx != NULL)
			dns_tkeyctx_destroy(&sctx->tkeyctx);

		if (sctx->nsstats != NULL)
			ns_stats_detach(&sctx->nsstats);

		if (sctx->rcvquerystats != NULL)
			dns_stats_detach(&sctx->rcvquerystats);
		if (sctx->opcodestats != NULL)
			dns_stats_detach(&sctx->opcodestats);
		if (sctx->rcodestats != NULL)
			dns_stats_detach(&sctx->rcodestats);

		if (sctx->udpinstats4 != NULL)
			isc_stats_detach(&sctx->udpinstats4);
		if (sctx->tcpinstats4 != NULL)
			isc_stats_detach(&sctx->tcpinstats4);
		if (sctx->udpoutstats4 != NULL)
			isc_stats_detach(&sctx->udpoutstats4);
		if (sctx->tcpoutstats4 != NULL)
			isc_stats_detach(&sctx->tcpoutstats4);

		if (sctx->udpinstats6 != NULL)
			isc_stats_detach(&sctx->udpinstats6);
		if (sctx->tcpinstats6 != NULL)
			isc_stats_detach(&sctx->tcpinstats6);
		if (sctx->udpoutstats6 != NULL)
			isc_stats_detach(&sctx->udpoutstats6);
		if (sctx->tcpoutstats6 != NULL)
			isc_stats_detach(&sctx->tcpoutstats6);

		isc_mem_putanddetach(&sctx->mctx, sctx, sizeof(*sctx));
	}

	*sctxp = NULL;
}

isc_result_t
ns_server_setserverid(ns_server_t *sctx, const char *serverid) {
	REQUIRE(SCTX_VALID(sctx));

	if (sctx->server_id != NULL) {
		isc_mem_free(sctx->mctx, sctx->server_id);
		sctx->server_id = NULL;
	}

	if (serverid != NULL) {
		sctx->server_id = isc_mem_strdup(sctx->mctx, serverid);
		if (sctx->server_id == NULL)
			return (ISC_R_NOMEMORY);
	}

	return (ISC_R_SUCCESS);
}

void
ns_server_settimeouts(ns_server_t *sctx, unsigned int initial,
		      unsigned int idle, unsigned int keepalive,
		      unsigned int advertised)
{
	REQUIRE(SCTX_VALID(sctx));

	sctx->initialtimo = initial;
	sctx->idletimo = idle;
	sctx->keepalivetimo = keepalive;
	sctx->advertisedtimo = advertised;
}

void
ns_server_gettimeouts(ns_server_t *sctx, unsigned int *initial,
		      unsigned int *idle, unsigned int *keepalive,
		      unsigned int *advertised)
{
	REQUIRE(SCTX_VALID(sctx));
	REQUIRE(initial != NULL && idle != NULL &&
		keepalive != NULL && advertised != NULL);

	*initial = sctx->initialtimo;
	*idle = sctx->idletimo;
	*keepalive = sctx->keepalivetimo;
	*advertised = sctx->advertisedtimo;
}

void
ns_server_setoption(ns_server_t *sctx, unsigned int option,
		    isc_boolean_t value)
{
	REQUIRE(SCTX_VALID(sctx));
	if (value) {
		sctx->options |= option;
	} else {
		sctx->options &= ~option;
	}
}

isc_boolean_t
ns_server_getoption(ns_server_t *sctx, unsigned int option) {
	REQUIRE(SCTX_VALID(sctx));

	return (ISC_TF((sctx->options & option) != 0));
}
