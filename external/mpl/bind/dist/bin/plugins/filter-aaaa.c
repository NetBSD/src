/*	$NetBSD: filter-aaaa.c,v 1.8 2023/01/25 21:43:24 christos Exp $	*/

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
#include <stdbool.h>
#include <string.h>

#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/ht.h>
#include <isc/lib.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/result.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/db.h>
#include <dns/enumtype.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/rdataset.h>
#include <dns/result.h>
#include <dns/types.h>
#include <dns/view.h>

#include <isccfg/aclconf.h>
#include <isccfg/cfg.h>
#include <isccfg/grammar.h>

#include <ns/client.h>
#include <ns/hooks.h>
#include <ns/log.h>
#include <ns/query.h>
#include <ns/types.h>

#define CHECK(op)                              \
	do {                                   \
		result = (op);                 \
		if (result != ISC_R_SUCCESS) { \
			goto cleanup;          \
		}                              \
	} while (0)

/*
 * Possible values for the settings of filter-aaaa-on-v4 and
 * filter-aaaa-on-v6: "no" is NONE, "yes" is FILTER, "break-dnssec"
 * is BREAK_DNSSEC.
 */
typedef enum { NONE = 0, FILTER = 1, BREAK_DNSSEC = 2 } filter_aaaa_t;

/*
 * Persistent data for use by this module. This will be associated
 * with client object address in the hash table, and will remain
 * accessible until the client object is detached.
 */
typedef struct filter_data {
	filter_aaaa_t mode;
	uint32_t flags;
} filter_data_t;

typedef struct filter_instance {
	ns_plugin_t *module;
	isc_mem_t *mctx;

	/*
	 * Hash table associating a client object with its persistent data.
	 */
	isc_ht_t *ht;
	isc_mutex_t hlock;

	/*
	 * Values configured when the module is loaded.
	 */
	filter_aaaa_t v4_aaaa;
	filter_aaaa_t v6_aaaa;
	dns_acl_t *aaaa_acl;
} filter_instance_t;

/*
 * Per-client flags set by this module
 */
#define FILTER_AAAA_RECURSING 0x0001 /* Recursing for A */
#define FILTER_AAAA_FILTERED  0x0002 /* AAAA was removed from answer */

/*
 * Client attribute tests.
 */
#define WANTDNSSEC(c)  (((c)->attributes & NS_CLIENTATTR_WANTDNSSEC) != 0)
#define RECURSIONOK(c) (((c)->query.attributes & NS_QUERYATTR_RECURSIONOK) != 0)

/*
 * Forward declarations of functions referenced in install_hooks().
 */
static ns_hookresult_t
filter_qctx_initialize(void *arg, void *cbdata, isc_result_t *resp);
static ns_hookresult_t
filter_respond_begin(void *arg, void *cbdata, isc_result_t *resp);
static ns_hookresult_t
filter_respond_any_found(void *arg, void *cbdata, isc_result_t *resp);
static ns_hookresult_t
filter_prep_response_begin(void *arg, void *cbdata, isc_result_t *resp);
static ns_hookresult_t
filter_query_done_send(void *arg, void *cbdata, isc_result_t *resp);
static ns_hookresult_t
filter_qctx_destroy(void *arg, void *cbdata, isc_result_t *resp);

/*%
 * Register the functions to be called at each hook point in 'hooktable', using
 * memory context 'mctx' for allocating copies of stack-allocated structures
 * passed to ns_hook_add().  Make sure 'inst' will be passed as the 'cbdata'
 * argument to every callback.
 */
static void
install_hooks(ns_hooktable_t *hooktable, isc_mem_t *mctx,
	      filter_instance_t *inst) {
	const ns_hook_t filter_init = {
		.action = filter_qctx_initialize,
		.action_data = inst,
	};

	const ns_hook_t filter_respbegin = {
		.action = filter_respond_begin,
		.action_data = inst,
	};

	const ns_hook_t filter_respanyfound = {
		.action = filter_respond_any_found,
		.action_data = inst,
	};

	const ns_hook_t filter_prepresp = {
		.action = filter_prep_response_begin,
		.action_data = inst,
	};

	const ns_hook_t filter_donesend = {
		.action = filter_query_done_send,
		.action_data = inst,
	};

	const ns_hook_t filter_destroy = {
		.action = filter_qctx_destroy,
		.action_data = inst,
	};

	ns_hook_add(hooktable, mctx, NS_QUERY_QCTX_INITIALIZED, &filter_init);
	ns_hook_add(hooktable, mctx, NS_QUERY_RESPOND_BEGIN, &filter_respbegin);
	ns_hook_add(hooktable, mctx, NS_QUERY_RESPOND_ANY_FOUND,
		    &filter_respanyfound);
	ns_hook_add(hooktable, mctx, NS_QUERY_PREP_RESPONSE_BEGIN,
		    &filter_prepresp);
	ns_hook_add(hooktable, mctx, NS_QUERY_DONE_SEND, &filter_donesend);
	ns_hook_add(hooktable, mctx, NS_QUERY_QCTX_DESTROYED, &filter_destroy);
}

/**
** Support for parsing of parameters and configuration of the module.
**/

/*
 * Support for parsing of parameters.
 */
static const char *filter_aaaa_enums[] = { "break-dnssec", NULL };

static isc_result_t
parse_filter_aaaa(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (cfg_parse_enum_or_other(pctx, type, &cfg_type_boolean, ret));
}

static void
doc_filter_aaaa(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_doc_enum_or_other(pctx, type, &cfg_type_boolean);
}

static cfg_type_t cfg_type_filter_aaaa = {
	"filter_aaaa",	 parse_filter_aaaa, cfg_print_ustring,
	doc_filter_aaaa, &cfg_rep_string,   filter_aaaa_enums,
};

static cfg_clausedef_t param_clauses[] = {
	{ "filter-aaaa", &cfg_type_bracketed_aml, 0 },
	{ "filter-aaaa-on-v4", &cfg_type_filter_aaaa, 0 },
	{ "filter-aaaa-on-v6", &cfg_type_filter_aaaa, 0 },
};

static cfg_clausedef_t *param_clausesets[] = { param_clauses, NULL };

static cfg_type_t cfg_type_parameters = {
	"filter-aaaa-params", cfg_parse_mapbody, cfg_print_mapbody,
	cfg_doc_mapbody,      &cfg_rep_map,	 param_clausesets
};

static isc_result_t
parse_filter_aaaa_on(const cfg_obj_t *param_obj, const char *param_name,
		     filter_aaaa_t *dstp) {
	const cfg_obj_t *obj = NULL;
	isc_result_t result;

	result = cfg_map_get(param_obj, param_name, &obj);
	if (result != ISC_R_SUCCESS) {
		return (ISC_R_SUCCESS);
	}

	if (cfg_obj_isboolean(obj)) {
		if (cfg_obj_asboolean(obj)) {
			*dstp = FILTER;
		} else {
			*dstp = NONE;
		}
	} else if (strcasecmp(cfg_obj_asstring(obj), "break-dnssec") == 0) {
		*dstp = BREAK_DNSSEC;
	} else {
		result = ISC_R_UNEXPECTED;
	}

	return (result);
}

static isc_result_t
check_syntax(cfg_obj_t *fmap, const void *cfg, isc_mem_t *mctx, isc_log_t *lctx,
	     void *actx) {
	isc_result_t result = ISC_R_SUCCESS;
	const cfg_obj_t *aclobj = NULL;
	dns_acl_t *acl = NULL;
	filter_aaaa_t f4 = NONE, f6 = NONE;

	cfg_map_get(fmap, "filter-aaaa", &aclobj);
	if (aclobj == NULL) {
		return (result);
	}

	CHECK(cfg_acl_fromconfig(aclobj, (const cfg_obj_t *)cfg, lctx,
				 (cfg_aclconfctx_t *)actx, mctx, 0, &acl));

	CHECK(parse_filter_aaaa_on(fmap, "filter-aaaa-on-v4", &f4));
	CHECK(parse_filter_aaaa_on(fmap, "filter-aaaa-on-v6", &f6));

	if ((f4 != NONE || f6 != NONE) && dns_acl_isnone(acl)) {
		cfg_obj_log(aclobj, lctx, ISC_LOG_WARNING,
			    "\"filter-aaaa\" is 'none;' but "
			    "either filter-aaaa-on-v4 or filter-aaaa-on-v6 "
			    "is enabled");
		result = ISC_R_FAILURE;
	} else if (f4 == NONE && f6 == NONE && !dns_acl_isnone(acl)) {
		cfg_obj_log(aclobj, lctx, ISC_LOG_WARNING,
			    "\"filter-aaaa\" is set but "
			    "neither filter-aaaa-on-v4 or filter-aaaa-on-v6 "
			    "is enabled");
		result = ISC_R_FAILURE;
	}

cleanup:
	if (acl != NULL) {
		dns_acl_detach(&acl);
	}

	return (result);
}

static isc_result_t
parse_parameters(filter_instance_t *inst, const char *parameters,
		 const void *cfg, const char *cfg_file, unsigned long cfg_line,
		 isc_mem_t *mctx, isc_log_t *lctx, void *actx) {
	isc_result_t result = ISC_R_SUCCESS;
	cfg_parser_t *parser = NULL;
	cfg_obj_t *param_obj = NULL;
	const cfg_obj_t *obj = NULL;
	isc_buffer_t b;

	CHECK(cfg_parser_create(mctx, lctx, &parser));

	isc_buffer_constinit(&b, parameters, strlen(parameters));
	isc_buffer_add(&b, strlen(parameters));
	CHECK(cfg_parse_buffer(parser, &b, cfg_file, cfg_line,
			       &cfg_type_parameters, 0, &param_obj));

	CHECK(check_syntax(param_obj, cfg, mctx, lctx, actx));

	CHECK(parse_filter_aaaa_on(param_obj, "filter-aaaa-on-v4",
				   &inst->v4_aaaa));
	CHECK(parse_filter_aaaa_on(param_obj, "filter-aaaa-on-v6",
				   &inst->v6_aaaa));

	result = cfg_map_get(param_obj, "filter-aaaa", &obj);
	if (result == ISC_R_SUCCESS) {
		CHECK(cfg_acl_fromconfig(obj, (const cfg_obj_t *)cfg, lctx,
					 (cfg_aclconfctx_t *)actx, mctx, 0,
					 &inst->aaaa_acl));
	} else {
		CHECK(dns_acl_any(mctx, &inst->aaaa_acl));
	}

cleanup:
	if (param_obj != NULL) {
		cfg_obj_destroy(parser, &param_obj);
	}
	if (parser != NULL) {
		cfg_parser_destroy(&parser);
	}
	return (result);
}

/**
** Mandatory plugin API functions:
**
** - plugin_destroy
** - plugin_register
** - plugin_version
** - plugin_check
**/

/*
 * Called by ns_plugin_register() to initialize the plugin and
 * register hook functions into the view hook table.
 */
isc_result_t
plugin_register(const char *parameters, const void *cfg, const char *cfg_file,
		unsigned long cfg_line, isc_mem_t *mctx, isc_log_t *lctx,
		void *actx, ns_hooktable_t *hooktable, void **instp) {
	filter_instance_t *inst = NULL;
	isc_result_t result = ISC_R_SUCCESS;

	isc_log_write(lctx, NS_LOGCATEGORY_GENERAL, NS_LOGMODULE_HOOKS,
		      ISC_LOG_INFO,
		      "registering 'filter-aaaa' "
		      "module from %s:%lu, %s parameters",
		      cfg_file, cfg_line, parameters != NULL ? "with" : "no");

	inst = isc_mem_get(mctx, sizeof(*inst));
	memset(inst, 0, sizeof(*inst));
	isc_mem_attach(mctx, &inst->mctx);

	if (parameters != NULL) {
		CHECK(parse_parameters(inst, parameters, cfg, cfg_file,
				       cfg_line, mctx, lctx, actx));
	}

	isc_ht_init(&inst->ht, mctx, 16);
	isc_mutex_init(&inst->hlock);

	/*
	 * Set hook points in the view's hooktable.
	 */
	install_hooks(hooktable, mctx, inst);

	*instp = inst;

cleanup:
	if (result != ISC_R_SUCCESS && inst != NULL) {
		plugin_destroy((void **)&inst);
	}

	return (result);
}

isc_result_t
plugin_check(const char *parameters, const void *cfg, const char *cfg_file,
	     unsigned long cfg_line, isc_mem_t *mctx, isc_log_t *lctx,
	     void *actx) {
	isc_result_t result = ISC_R_SUCCESS;
	cfg_parser_t *parser = NULL;
	cfg_obj_t *param_obj = NULL;
	isc_buffer_t b;

	CHECK(cfg_parser_create(mctx, lctx, &parser));

	isc_buffer_constinit(&b, parameters, strlen(parameters));
	isc_buffer_add(&b, strlen(parameters));
	CHECK(cfg_parse_buffer(parser, &b, cfg_file, cfg_line,
			       &cfg_type_parameters, 0, &param_obj));

	CHECK(check_syntax(param_obj, cfg, mctx, lctx, actx));

cleanup:
	if (param_obj != NULL) {
		cfg_obj_destroy(parser, &param_obj);
	}
	if (parser != NULL) {
		cfg_parser_destroy(&parser);
	}
	return (result);
}

/*
 * Called by ns_plugins_free(); frees memory allocated by
 * the module when it was registered.
 */
void
plugin_destroy(void **instp) {
	filter_instance_t *inst = (filter_instance_t *)*instp;

	if (inst->ht != NULL) {
		isc_ht_destroy(&inst->ht);
		isc_mutex_destroy(&inst->hlock);
	}
	if (inst->aaaa_acl != NULL) {
		dns_acl_detach(&inst->aaaa_acl);
	}

	isc_mem_putanddetach(&inst->mctx, inst, sizeof(*inst));
	*instp = NULL;

	return;
}

/*
 * Returns plugin API version for compatibility checks.
 */
int
plugin_version(void) {
	return (NS_PLUGIN_VERSION);
}

/**
** "filter-aaaa" feature implementation begins here.
**/

/*%
 * Structure describing the filtering to be applied by process_section().
 */
typedef struct section_filter {
	query_ctx_t *qctx;
	filter_aaaa_t mode;
	dns_section_t section;
	const dns_name_t *name;
	dns_rdatatype_t type;
	bool only_if_a_exists;
} section_filter_t;

/*
 * Check whether this is an IPv4 client.
 */
static bool
is_v4_client(ns_client_t *client) {
	if (isc_sockaddr_pf(&client->peeraddr) == AF_INET) {
		return (true);
	}
	if (isc_sockaddr_pf(&client->peeraddr) == AF_INET6 &&
	    IN6_IS_ADDR_V4MAPPED(&client->peeraddr.type.sin6.sin6_addr))
	{
		return (true);
	}
	return (false);
}

/*
 * Check whether this is an IPv6 client.
 */
static bool
is_v6_client(ns_client_t *client) {
	if (isc_sockaddr_pf(&client->peeraddr) == AF_INET6 &&
	    !IN6_IS_ADDR_V4MAPPED(&client->peeraddr.type.sin6.sin6_addr))
	{
		return (true);
	}
	return (false);
}

static filter_data_t *
client_state_get(const query_ctx_t *qctx, filter_instance_t *inst) {
	filter_data_t *client_state = NULL;
	isc_result_t result;

	LOCK(&inst->hlock);
	result = isc_ht_find(inst->ht, (const unsigned char *)&qctx->client,
			     sizeof(qctx->client), (void **)&client_state);
	UNLOCK(&inst->hlock);

	return (result == ISC_R_SUCCESS ? client_state : NULL);
}

static void
client_state_create(const query_ctx_t *qctx, filter_instance_t *inst) {
	filter_data_t *client_state;
	isc_result_t result;

	client_state = isc_mem_get(inst->mctx, sizeof(*client_state));

	client_state->mode = NONE;
	client_state->flags = 0;

	LOCK(&inst->hlock);
	result = isc_ht_add(inst->ht, (const unsigned char *)&qctx->client,
			    sizeof(qctx->client), client_state);
	UNLOCK(&inst->hlock);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
}

static void
client_state_destroy(const query_ctx_t *qctx, filter_instance_t *inst) {
	filter_data_t *client_state = client_state_get(qctx, inst);
	isc_result_t result;

	if (client_state == NULL) {
		return;
	}

	LOCK(&inst->hlock);
	result = isc_ht_delete(inst->ht, (const unsigned char *)&qctx->client,
			       sizeof(qctx->client));
	UNLOCK(&inst->hlock);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	isc_mem_put(inst->mctx, client_state, sizeof(*client_state));
}

/*%
 * Mark 'rdataset' and 'sigrdataset' as rendered, gracefully handling NULL
 * pointers and non-associated rdatasets.
 */
static void
mark_as_rendered(dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset) {
	if (rdataset != NULL && dns_rdataset_isassociated(rdataset)) {
		rdataset->attributes |= DNS_RDATASETATTR_RENDERED;
	}
	if (sigrdataset != NULL && dns_rdataset_isassociated(sigrdataset)) {
		sigrdataset->attributes |= DNS_RDATASETATTR_RENDERED;
	}
}

/*%
 * Check whether an RRset of given 'type' is present at given 'name'.  If
 * it is found and either it is not signed or the combination of query
 * flags and configured processing 'mode' allows it, mark the RRset and its
 * associated signatures as already rendered to prevent them from appearing
 * in the response message stored in 'qctx'.  If 'only_if_a_exists' is
 * true, an RRset of type A must also exist at 'name' in order for the
 * above processing to happen.
 */
static bool
process_name(query_ctx_t *qctx, filter_aaaa_t mode, const dns_name_t *name,
	     dns_rdatatype_t type, bool only_if_a_exists) {
	dns_rdataset_t *rdataset = NULL, *sigrdataset = NULL;
	isc_result_t result;
	bool modified = false;

	if (only_if_a_exists) {
		CHECK(dns_message_findtype(name, dns_rdatatype_a, 0, NULL));
	}

	(void)dns_message_findtype(name, type, 0, &rdataset);
	(void)dns_message_findtype(name, dns_rdatatype_rrsig, type,
				   &sigrdataset);

	if (rdataset != NULL &&
	    (sigrdataset == NULL || !WANTDNSSEC(qctx->client) ||
	     mode == BREAK_DNSSEC))
	{
		/*
		 * An RRset of given 'type' was found at 'name' and at least
		 * one of the following is true:
		 *
		 *   - the RRset is not signed,
		 *   - the client did not set the DO bit in its request,
		 *   - configuration allows us to tamper with signed responses.
		 *
		 * This means it is okay to filter out this RRset and its
		 * signatures, if any, from the response.
		 */
		mark_as_rendered(rdataset, sigrdataset);
		modified = true;
	}

cleanup:
	return (modified);
}

/*%
 * Apply the requested section filter, i.e. prevent (when possible, as
 * determined by process_name()) RRsets of given 'type' from being rendered
 * in the given 'section' of the response message stored in 'qctx'.  Clear
 * the AD bit if the answer and/or authority section was modified.  If
 * 'name' is NULL, all names in the given 'section' are processed;
 * otherwise, only 'name' is.  'only_if_a_exists' is passed through to
 * process_name().
 */
static void
process_section(const section_filter_t *filter) {
	query_ctx_t *qctx = filter->qctx;
	filter_aaaa_t mode = filter->mode;
	dns_section_t section = filter->section;
	const dns_name_t *name = filter->name;
	dns_rdatatype_t type = filter->type;
	bool only_if_a_exists = filter->only_if_a_exists;

	dns_message_t *message = qctx->client->message;
	isc_result_t result;

	for (result = dns_message_firstname(message, section);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(message, section))
	{
		dns_name_t *cur = NULL;
		dns_message_currentname(message, section, &cur);
		if (name != NULL && !dns_name_equal(name, cur)) {
			/*
			 * We only want to process 'name' and this is not it.
			 */
			continue;
		}

		if (!process_name(qctx, mode, cur, type, only_if_a_exists)) {
			/*
			 * Response was not modified, do not touch the AD bit.
			 */
			continue;
		}

		if (section == DNS_SECTION_ANSWER ||
		    section == DNS_SECTION_AUTHORITY)
		{
			message->flags &= ~DNS_MESSAGEFLAG_AD;
		}
	}
}

/*
 * Initialize filter state, fetching it from a memory pool and storing it
 * in a hash table keyed according to the client object; this enables us to
 * retrieve persistent data related to a client query for as long as the
 * object persists.
 */
static ns_hookresult_t
filter_qctx_initialize(void *arg, void *cbdata, isc_result_t *resp) {
	query_ctx_t *qctx = (query_ctx_t *)arg;
	filter_instance_t *inst = (filter_instance_t *)cbdata;
	filter_data_t *client_state;

	*resp = ISC_R_UNSET;

	client_state = client_state_get(qctx, inst);
	if (client_state == NULL) {
		client_state_create(qctx, inst);
	}

	return (NS_HOOK_CONTINUE);
}

/*
 * Determine whether this client should have AAAA filtered or not, based on
 * the client address family and the settings of filter-aaaa-on-v4 and
 * filter-aaaa-on-v6.
 */
static ns_hookresult_t
filter_prep_response_begin(void *arg, void *cbdata, isc_result_t *resp) {
	query_ctx_t *qctx = (query_ctx_t *)arg;
	filter_instance_t *inst = (filter_instance_t *)cbdata;
	filter_data_t *client_state = client_state_get(qctx, inst);
	isc_result_t result;

	*resp = ISC_R_UNSET;

	if (client_state == NULL) {
		return (NS_HOOK_CONTINUE);
	}

	if (inst->v4_aaaa != NONE || inst->v6_aaaa != NONE) {
		result = ns_client_checkaclsilent(qctx->client, NULL,
						  inst->aaaa_acl, true);
		if (result == ISC_R_SUCCESS && inst->v4_aaaa != NONE &&
		    is_v4_client(qctx->client))
		{
			client_state->mode = inst->v4_aaaa;
		} else if (result == ISC_R_SUCCESS && inst->v6_aaaa != NONE &&
			   is_v6_client(qctx->client))
		{
			client_state->mode = inst->v6_aaaa;
		}
	}

	return (NS_HOOK_CONTINUE);
}

/*
 * Hide AAAA rrsets if there is a matching A. Trigger recursion if
 * necessary to find out whether an A exists.
 *
 * (This version is for processing answers to explicit AAAA queries; ANY
 * queries are handled in filter_respond_any_found().)
 */
static ns_hookresult_t
filter_respond_begin(void *arg, void *cbdata, isc_result_t *resp) {
	query_ctx_t *qctx = (query_ctx_t *)arg;
	filter_instance_t *inst = (filter_instance_t *)cbdata;
	filter_data_t *client_state = client_state_get(qctx, inst);
	isc_result_t result = ISC_R_UNSET;

	*resp = ISC_R_UNSET;

	if (client_state == NULL) {
		return (NS_HOOK_CONTINUE);
	}

	if (client_state->mode != BREAK_DNSSEC &&
	    (client_state->mode != FILTER ||
	     (WANTDNSSEC(qctx->client) && qctx->sigrdataset != NULL &&
	      dns_rdataset_isassociated(qctx->sigrdataset))))
	{
		return (NS_HOOK_CONTINUE);
	}

	if (qctx->qtype == dns_rdatatype_aaaa) {
		dns_rdataset_t *trdataset;
		trdataset = ns_client_newrdataset(qctx->client);
		result = dns_db_findrdataset(
			qctx->db, qctx->node, qctx->version, dns_rdatatype_a, 0,
			qctx->client->now, trdataset, NULL);
		if (dns_rdataset_isassociated(trdataset)) {
			dns_rdataset_disassociate(trdataset);
		}
		ns_client_putrdataset(qctx->client, &trdataset);

		/*
		 * We found an AAAA. If we also found an A, then the AAAA
		 * must not be rendered.
		 *
		 * If the A is not in our cache, then any result other than
		 * DNS_R_DELEGATION or ISC_R_NOTFOUND means there is no A,
		 * and so AAAAs are okay.
		 *
		 * We assume there is no A if we can't recurse for this
		 * client. That might be the wrong answer, but what else
		 * can we do?  Besides, the fact that we have the AAAA and
		 * are using this mechanism in the first place suggests
		 * that we care more about As than AAAAs, and would have
		 * cached an A if it existed.
		 */
		if (result == ISC_R_SUCCESS) {
			mark_as_rendered(qctx->rdataset, qctx->sigrdataset);
			qctx->client->message->flags &= ~DNS_MESSAGEFLAG_AD;
			client_state->flags |= FILTER_AAAA_FILTERED;
		} else if (!qctx->authoritative && RECURSIONOK(qctx->client) &&
			   (result == DNS_R_DELEGATION ||
			    result == ISC_R_NOTFOUND))
		{
			/*
			 * This is an ugly kludge to recurse
			 * for the A and discard the result.
			 *
			 * Continue to add the AAAA now.
			 * We'll make a note to not render it
			 * if the recursion for the A succeeds.
			 */
			result = ns_query_recurse(qctx->client, dns_rdatatype_a,
						  qctx->client->query.qname,
						  NULL, NULL, qctx->resuming);
			if (result == ISC_R_SUCCESS) {
				client_state->flags |= FILTER_AAAA_RECURSING;
				qctx->client->query.attributes |=
					NS_QUERYATTR_RECURSING;
			}
		}
	} else if (qctx->qtype == dns_rdatatype_a &&
		   (client_state->flags & FILTER_AAAA_RECURSING) != 0)
	{
		const section_filter_t filter_answer = {
			.qctx = qctx,
			.mode = client_state->mode,
			.section = DNS_SECTION_ANSWER,
			.name = qctx->fname,
			.type = dns_rdatatype_aaaa,
		};
		process_section(&filter_answer);

		client_state->flags &= ~FILTER_AAAA_RECURSING;

		result = ns_query_done(qctx);

		*resp = result;

		return (NS_HOOK_RETURN);
	}

	*resp = result;
	return (NS_HOOK_CONTINUE);
}

/*
 * When answering an ANY query, remove AAAA if A is present.
 */
static ns_hookresult_t
filter_respond_any_found(void *arg, void *cbdata, isc_result_t *resp) {
	query_ctx_t *qctx = (query_ctx_t *)arg;
	filter_instance_t *inst = (filter_instance_t *)cbdata;
	filter_data_t *client_state = client_state_get(qctx, inst);

	*resp = ISC_R_UNSET;

	if (client_state != NULL && client_state->mode != NONE) {
		/*
		 * If we are authoritative, require an A record to be
		 * present before filtering out AAAA records; otherwise,
		 * just assume an A record exists even if it was not in the
		 * cache (and therefore is not in the response message),
		 * thus proceeding with filtering out AAAA records.
		 */
		const section_filter_t filter_answer = {
			.qctx = qctx,
			.mode = client_state->mode,
			.section = DNS_SECTION_ANSWER,
			.name = qctx->tname,
			.type = dns_rdatatype_aaaa,
			.only_if_a_exists = qctx->authoritative,
		};
		process_section(&filter_answer);
	}

	return (NS_HOOK_CONTINUE);
}

/*
 * Hide AAAA rrsets in the additional section if there is a matching A, and
 * hide NS in the authority section if AAAA was filtered in the answer
 * section.
 */
static ns_hookresult_t
filter_query_done_send(void *arg, void *cbdata, isc_result_t *resp) {
	query_ctx_t *qctx = (query_ctx_t *)arg;
	filter_instance_t *inst = (filter_instance_t *)cbdata;
	filter_data_t *client_state = client_state_get(qctx, inst);

	*resp = ISC_R_UNSET;

	if (client_state != NULL && client_state->mode != NONE) {
		const section_filter_t filter_additional = {
			.qctx = qctx,
			.mode = client_state->mode,
			.section = DNS_SECTION_ADDITIONAL,
			.type = dns_rdatatype_aaaa,
			.only_if_a_exists = true,
		};
		process_section(&filter_additional);

		if ((client_state->flags & FILTER_AAAA_FILTERED) != 0) {
			const section_filter_t filter_authority = {
				.qctx = qctx,
				.mode = client_state->mode,
				.section = DNS_SECTION_AUTHORITY,
				.type = dns_rdatatype_ns,
			};
			process_section(&filter_authority);
		}
	}

	return (NS_HOOK_CONTINUE);
}

/*
 * If the client is being detached, then we can delete our persistent data
 * from hash table and return it to the memory pool.
 */
static ns_hookresult_t
filter_qctx_destroy(void *arg, void *cbdata, isc_result_t *resp) {
	query_ctx_t *qctx = (query_ctx_t *)arg;
	filter_instance_t *inst = (filter_instance_t *)cbdata;

	*resp = ISC_R_UNSET;

	if (!qctx->detach_client) {
		return (NS_HOOK_CONTINUE);
	}

	client_state_destroy(qctx, inst);

	return (NS_HOOK_CONTINUE);
}
