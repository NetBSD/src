/*	$NetBSD: namedconf.c,v 1.14 2023/01/25 21:43:32 christos Exp $	*/

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
#include <stdlib.h>
#include <string.h>

#include <isc/lex.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/result.h>
#include <dns/ttl.h>

#include <isccfg/cfg.h>
#include <isccfg/grammar.h>
#include <isccfg/log.h>
#include <isccfg/namedconf.h>

#define TOKEN_STRING(pctx) (pctx->token.value.as_textregion.base)

/*% Check a return value. */
#define CHECK(op)                            \
	do {                                 \
		result = (op);               \
		if (result != ISC_R_SUCCESS) \
			goto cleanup;        \
	} while (0)

/*% Clean up a configuration object if non-NULL. */
#define CLEANUP_OBJ(obj)                               \
	do {                                           \
		if ((obj) != NULL)                     \
			cfg_obj_destroy(pctx, &(obj)); \
	} while (0)

/*%
 * Forward declarations of static functions.
 */

static isc_result_t
parse_keyvalue(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

static isc_result_t
parse_optional_keyvalue(cfg_parser_t *pctx, const cfg_type_t *type,
			cfg_obj_t **ret);

static isc_result_t
parse_updatepolicy(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);
static void
print_updatepolicy(cfg_printer_t *pctx, const cfg_obj_t *obj);

static void
doc_updatepolicy(cfg_printer_t *pctx, const cfg_type_t *type);

static void
print_keyvalue(cfg_printer_t *pctx, const cfg_obj_t *obj);

static void
doc_keyvalue(cfg_printer_t *pctx, const cfg_type_t *type);

static void
doc_optional_keyvalue(cfg_printer_t *pctx, const cfg_type_t *type);

static cfg_type_t cfg_type_acl;
static cfg_type_t cfg_type_bracketed_dscpsockaddrlist;
static cfg_type_t cfg_type_bracketed_namesockaddrkeylist;
static cfg_type_t cfg_type_bracketed_netaddrlist;
static cfg_type_t cfg_type_bracketed_sockaddrnameportlist;
static cfg_type_t cfg_type_controls;
static cfg_type_t cfg_type_controls_sockaddr;
static cfg_type_t cfg_type_destinationlist;
static cfg_type_t cfg_type_dialuptype;
static cfg_type_t cfg_type_dlz;
static cfg_type_t cfg_type_dnssecpolicy;
static cfg_type_t cfg_type_dnstap;
static cfg_type_t cfg_type_dnstapoutput;
static cfg_type_t cfg_type_dyndb;
static cfg_type_t cfg_type_plugin;
static cfg_type_t cfg_type_ixfrdifftype;
static cfg_type_t cfg_type_ixfrratio;
static cfg_type_t cfg_type_key;
static cfg_type_t cfg_type_logfile;
static cfg_type_t cfg_type_logging;
static cfg_type_t cfg_type_logseverity;
static cfg_type_t cfg_type_logsuffix;
static cfg_type_t cfg_type_logversions;
static cfg_type_t cfg_type_remoteselement;
static cfg_type_t cfg_type_maxduration;
static cfg_type_t cfg_type_minimal;
static cfg_type_t cfg_type_nameportiplist;
static cfg_type_t cfg_type_notifytype;
static cfg_type_t cfg_type_optional_allow;
static cfg_type_t cfg_type_optional_class;
static cfg_type_t cfg_type_optional_dscp;
static cfg_type_t cfg_type_optional_facility;
static cfg_type_t cfg_type_optional_keyref;
static cfg_type_t cfg_type_optional_port;
static cfg_type_t cfg_type_optional_uint32;
static cfg_type_t cfg_type_options;
static cfg_type_t cfg_type_portiplist;
static cfg_type_t cfg_type_printtime;
static cfg_type_t cfg_type_qminmethod;
static cfg_type_t cfg_type_querysource4;
static cfg_type_t cfg_type_querysource6;
static cfg_type_t cfg_type_querysource;
static cfg_type_t cfg_type_server;
static cfg_type_t cfg_type_server_key_kludge;
static cfg_type_t cfg_type_size;
static cfg_type_t cfg_type_sizenodefault;
static cfg_type_t cfg_type_sizeorpercent;
static cfg_type_t cfg_type_sizeval;
static cfg_type_t cfg_type_sockaddr4wild;
static cfg_type_t cfg_type_sockaddr6wild;
static cfg_type_t cfg_type_statschannels;
static cfg_type_t cfg_type_view;
static cfg_type_t cfg_type_viewopts;
static cfg_type_t cfg_type_zone;

/*% tkey-dhkey */

static cfg_tuplefielddef_t tkey_dhkey_fields[] = {
	{ "name", &cfg_type_qstring, 0 },
	{ "keyid", &cfg_type_uint32, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_tkey_dhkey = { "tkey-dhkey",	   cfg_parse_tuple,
					  cfg_print_tuple, cfg_doc_tuple,
					  &cfg_rep_tuple,  tkey_dhkey_fields };

/*% listen-on */

static cfg_tuplefielddef_t listenon_fields[] = {
	{ "port", &cfg_type_optional_port, 0 },
	{ "dscp", &cfg_type_optional_dscp, CFG_CLAUSEFLAG_DEPRECATED },
	{ "acl", &cfg_type_bracketed_aml, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_listenon = { "listenon",	 cfg_parse_tuple,
					cfg_print_tuple, cfg_doc_tuple,
					&cfg_rep_tuple,	 listenon_fields };

/*% acl */

static cfg_tuplefielddef_t acl_fields[] = { { "name", &cfg_type_astring, 0 },
					    { "value", &cfg_type_bracketed_aml,
					      0 },
					    { NULL, NULL, 0 } };

static cfg_type_t cfg_type_acl = { "acl",	    cfg_parse_tuple,
				   cfg_print_tuple, cfg_doc_tuple,
				   &cfg_rep_tuple,  acl_fields };

/*% remote servers, used for primaries and parental agents */
static cfg_tuplefielddef_t remotes_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "port", &cfg_type_optional_port, 0 },
	{ "dscp", &cfg_type_optional_dscp, CFG_CLAUSEFLAG_DEPRECATED },
	{ "addresses", &cfg_type_bracketed_namesockaddrkeylist, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_remoteservers = { "remote-servers", cfg_parse_tuple,
					     cfg_print_tuple,  cfg_doc_tuple,
					     &cfg_rep_tuple,   remotes_fields };

/*%
 * "sockaddrkeylist", a list of socket addresses with optional keys
 * and an optional default port, as used in the remote-servers option.
 * E.g.,
 *   "port 1234 { myservers; 10.0.0.1 key foo; 1::2 port 69; }"
 */

static cfg_tuplefielddef_t namesockaddrkey_fields[] = {
	{ "remoteselement", &cfg_type_remoteselement, 0 },
	{ "key", &cfg_type_optional_keyref, 0 },
	{ NULL, NULL, 0 },
};

static cfg_type_t cfg_type_namesockaddrkey = {
	"namesockaddrkey", cfg_parse_tuple, cfg_print_tuple,
	cfg_doc_tuple,	   &cfg_rep_tuple,  namesockaddrkey_fields
};

static cfg_type_t cfg_type_bracketed_namesockaddrkeylist = {
	"bracketed_namesockaddrkeylist",
	cfg_parse_bracketed_list,
	cfg_print_bracketed_list,
	cfg_doc_bracketed_list,
	&cfg_rep_list,
	&cfg_type_namesockaddrkey
};

static cfg_tuplefielddef_t namesockaddrkeylist_fields[] = {
	{ "port", &cfg_type_optional_port, 0 },
	{ "dscp", &cfg_type_optional_dscp, CFG_CLAUSEFLAG_DEPRECATED },
	{ "addresses", &cfg_type_bracketed_namesockaddrkeylist, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_namesockaddrkeylist = {
	"sockaddrkeylist", cfg_parse_tuple, cfg_print_tuple,
	cfg_doc_tuple,	   &cfg_rep_tuple,  namesockaddrkeylist_fields
};

/*%
 * A list of socket addresses with an optional default port, as used
 * in the 'listen-on' option.  E.g., "{ 10.0.0.1; 1::2 port 69; }"
 */
static cfg_tuplefielddef_t portiplist_fields[] = {
	{ "port", &cfg_type_optional_port, 0 },
	{ "dscp", &cfg_type_optional_dscp, CFG_CLAUSEFLAG_DEPRECATED },
	{ "addresses", &cfg_type_bracketed_dscpsockaddrlist, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_portiplist = { "portiplist",	   cfg_parse_tuple,
					  cfg_print_tuple, cfg_doc_tuple,
					  &cfg_rep_tuple,  portiplist_fields };

/*
 * Obsolete format for the "pubkey" statement.
 */
static cfg_tuplefielddef_t pubkey_fields[] = {
	{ "flags", &cfg_type_uint32, 0 },
	{ "protocol", &cfg_type_uint32, 0 },
	{ "algorithm", &cfg_type_uint32, 0 },
	{ "key", &cfg_type_qstring, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_pubkey = { "pubkey",	       cfg_parse_tuple,
				      cfg_print_tuple, cfg_doc_tuple,
				      &cfg_rep_tuple,  pubkey_fields };

/*%
 * A list of RR types, used in grant statements.
 * Note that the old parser allows quotes around the RR type names.
 */
static cfg_type_t cfg_type_rrtypelist = {
	"rrtypelist",	  cfg_parse_spacelist, cfg_print_spacelist,
	cfg_doc_terminal, &cfg_rep_list,       &cfg_type_astring
};

static const char *mode_enums[] = { "deny", "grant", NULL };
static cfg_type_t cfg_type_mode = {
	"mode",	      cfg_parse_enum,  cfg_print_ustring,
	cfg_doc_enum, &cfg_rep_string, &mode_enums
};

static isc_result_t
parse_matchtype(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;

	CHECK(cfg_peektoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_string &&
	    strcasecmp(TOKEN_STRING(pctx), "zonesub") == 0)
	{
		pctx->flags |= CFG_PCTX_SKIP;
	}
	return (cfg_parse_enum(pctx, type, ret));

cleanup:
	return (result);
}

static isc_result_t
parse_matchname(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;

	if ((pctx->flags & CFG_PCTX_SKIP) != 0) {
		pctx->flags &= ~CFG_PCTX_SKIP;
		CHECK(cfg_parse_void(pctx, NULL, &obj));
	} else {
		result = cfg_parse_astring(pctx, type, &obj);
	}

	*ret = obj;
cleanup:
	return (result);
}

static void
doc_matchname(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_print_cstr(pctx, "[ ");
	cfg_doc_obj(pctx, type->of);
	cfg_print_cstr(pctx, " ]");
}

static const char *matchtype_enums[] = { "6to4-self",
					 "external",
					 "krb5-self",
					 "krb5-selfsub",
					 "krb5-subdomain",
					 "ms-self",
					 "ms-selfsub",
					 "ms-subdomain",
					 "name",
					 "self",
					 "selfsub",
					 "selfwild",
					 "subdomain",
					 "tcp-self",
					 "wildcard",
					 "zonesub",
					 NULL };

static cfg_type_t cfg_type_matchtype = { "matchtype",	    parse_matchtype,
					 cfg_print_ustring, cfg_doc_enum,
					 &cfg_rep_string,   &matchtype_enums };

static cfg_type_t cfg_type_matchname = {
	"optional_matchname", parse_matchname, cfg_print_ustring,
	doc_matchname,	      &cfg_rep_tuple,  &cfg_type_ustring
};

/*%
 * A grant statement, used in the update policy.
 */
static cfg_tuplefielddef_t grant_fields[] = {
	{ "mode", &cfg_type_mode, 0 },
	{ "identity", &cfg_type_astring, 0 }, /* domain name */
	{ "matchtype", &cfg_type_matchtype, 0 },
	{ "name", &cfg_type_matchname, 0 }, /* domain name */
	{ "types", &cfg_type_rrtypelist, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_grant = { "grant",	      cfg_parse_tuple,
				     cfg_print_tuple, cfg_doc_tuple,
				     &cfg_rep_tuple,  grant_fields };

static cfg_type_t cfg_type_updatepolicy = {
	"update_policy",  parse_updatepolicy, print_updatepolicy,
	doc_updatepolicy, &cfg_rep_list,      &cfg_type_grant
};

static isc_result_t
parse_updatepolicy(cfg_parser_t *pctx, const cfg_type_t *type,
		   cfg_obj_t **ret) {
	isc_result_t result;
	CHECK(cfg_gettoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_special &&
	    pctx->token.value.as_char == '{')
	{
		cfg_ungettoken(pctx);
		return (cfg_parse_bracketed_list(pctx, type, ret));
	}

	if (pctx->token.type == isc_tokentype_string &&
	    strcasecmp(TOKEN_STRING(pctx), "local") == 0)
	{
		cfg_obj_t *obj = NULL;
		CHECK(cfg_create_obj(pctx, &cfg_type_ustring, &obj));
		obj->value.string.length = strlen("local");
		obj->value.string.base =
			isc_mem_get(pctx->mctx, obj->value.string.length + 1);
		memmove(obj->value.string.base, "local", 5);
		obj->value.string.base[5] = '\0';
		*ret = obj;
		return (ISC_R_SUCCESS);
	}

	cfg_ungettoken(pctx);
	return (ISC_R_UNEXPECTEDTOKEN);

cleanup:
	return (result);
}

static void
print_updatepolicy(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	if (cfg_obj_isstring(obj)) {
		cfg_print_ustring(pctx, obj);
	} else {
		cfg_print_bracketed_list(pctx, obj);
	}
}

static void
doc_updatepolicy(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_print_cstr(pctx, "( local | { ");
	cfg_doc_obj(pctx, type->of);
	cfg_print_cstr(pctx, "; ... } )");
}

/*%
 * A view statement.
 */
static cfg_tuplefielddef_t view_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "class", &cfg_type_optional_class, 0 },
	{ "options", &cfg_type_viewopts, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_view = { "view",	     cfg_parse_tuple,
				    cfg_print_tuple, cfg_doc_tuple,
				    &cfg_rep_tuple,  view_fields };

/*%
 * A zone statement.
 */
static cfg_tuplefielddef_t zone_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "class", &cfg_type_optional_class, 0 },
	{ "options", &cfg_type_zoneopts, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_zone = { "zone",	     cfg_parse_tuple,
				    cfg_print_tuple, cfg_doc_tuple,
				    &cfg_rep_tuple,  zone_fields };

/*%
 * A dnssec-policy statement.
 */
static cfg_tuplefielddef_t dnssecpolicy_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "options", &cfg_type_dnssecpolicyopts, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_dnssecpolicy = {
	"dnssec-policy", cfg_parse_tuple, cfg_print_tuple,
	cfg_doc_tuple,	 &cfg_rep_tuple,  dnssecpolicy_fields
};

/*%
 * A "category" clause in the "logging" statement.
 */
static cfg_tuplefielddef_t category_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "destinations", &cfg_type_destinationlist, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_category = { "category",	 cfg_parse_tuple,
					cfg_print_tuple, cfg_doc_tuple,
					&cfg_rep_tuple,	 category_fields };

static isc_result_t
parse_maxduration(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (cfg_parse_enum_or_other(pctx, type, &cfg_type_duration, ret));
}

static void
doc_maxduration(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_doc_enum_or_other(pctx, type, &cfg_type_duration);
}

/*%
 * A duration or "unlimited", but not "default".
 */
static const char *maxduration_enums[] = { "unlimited", NULL };
static cfg_type_t cfg_type_maxduration = {
	"maxduration_no_default", parse_maxduration, cfg_print_ustring,
	doc_maxduration,	  &cfg_rep_duration, maxduration_enums
};

/*%
 * A dnssec key, as used in the "trusted-keys" statement.
 */
static cfg_tuplefielddef_t dnsseckey_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "anchortype", &cfg_type_void, 0 },
	{ "rdata1", &cfg_type_uint32, 0 },
	{ "rdata2", &cfg_type_uint32, 0 },
	{ "rdata3", &cfg_type_uint32, 0 },
	{ "data", &cfg_type_qstring, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_dnsseckey = { "dnsseckey",	  cfg_parse_tuple,
					 cfg_print_tuple, cfg_doc_tuple,
					 &cfg_rep_tuple,  dnsseckey_fields };

/*%
 * Optional enums.
 *
 */
static isc_result_t
parse_optional_enum(cfg_parser_t *pctx, const cfg_type_t *type,
		    cfg_obj_t **ret) {
	return (cfg_parse_enum_or_other(pctx, type, &cfg_type_void, ret));
}

static void
doc_optional_enum(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "[ ");
	cfg_doc_enum(pctx, type);
	cfg_print_cstr(pctx, " ]");
}

/*%
 * A key initialization specifier, as used in the
 * "trust-anchors" (or synonymous "managed-keys") statement.
 */
static const char *anchortype_enums[] = { "static-key", "initial-key",
					  "static-ds", "initial-ds", NULL };
static cfg_type_t cfg_type_anchortype = { "anchortype",	     cfg_parse_enum,
					  cfg_print_ustring, cfg_doc_enum,
					  &cfg_rep_string,   anchortype_enums };
static cfg_tuplefielddef_t managedkey_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "anchortype", &cfg_type_anchortype, 0 },
	{ "rdata1", &cfg_type_uint32, 0 },
	{ "rdata2", &cfg_type_uint32, 0 },
	{ "rdata3", &cfg_type_uint32, 0 },
	{ "data", &cfg_type_qstring, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_managedkey = { "managedkey",	   cfg_parse_tuple,
					  cfg_print_tuple, cfg_doc_tuple,
					  &cfg_rep_tuple,  managedkey_fields };

/*%
 * DNSSEC key roles.
 */
static const char *dnsseckeyrole_enums[] = { "csk", "ksk", "zsk", NULL };
static cfg_type_t cfg_type_dnsseckeyrole = {
	"dnssec-key-role", cfg_parse_enum,  cfg_print_ustring,
	cfg_doc_enum,	   &cfg_rep_string, &dnsseckeyrole_enums
};

/*%
 * DNSSEC key storage types.
 */
static const char *dnsseckeystore_enums[] = { "key-directory", NULL };
static cfg_type_t cfg_type_dnsseckeystore = {
	"dnssec-key-storage", parse_optional_enum, cfg_print_ustring,
	doc_optional_enum,    &cfg_rep_string,	   dnsseckeystore_enums
};

/*%
 * A dnssec key, as used in the "keys" statement in a "dnssec-policy".
 */
static keyword_type_t algorithm_kw = { "algorithm", &cfg_type_ustring };
static cfg_type_t cfg_type_algorithm = { "algorithm",	  parse_keyvalue,
					 print_keyvalue,  doc_keyvalue,
					 &cfg_rep_string, &algorithm_kw };

static keyword_type_t lifetime_kw = { "lifetime",
				      &cfg_type_duration_or_unlimited };
static cfg_type_t cfg_type_lifetime = { "lifetime",	   parse_keyvalue,
					print_keyvalue,	   doc_keyvalue,
					&cfg_rep_duration, &lifetime_kw };

static cfg_tuplefielddef_t kaspkey_fields[] = {
	{ "role", &cfg_type_dnsseckeyrole, 0 },
	{ "keystore-type", &cfg_type_dnsseckeystore, 0 },
	{ "lifetime", &cfg_type_lifetime, 0 },
	{ "algorithm", &cfg_type_algorithm, 0 },
	{ "length", &cfg_type_optional_uint32, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_kaspkey = { "kaspkey",	cfg_parse_tuple,
				       cfg_print_tuple, cfg_doc_tuple,
				       &cfg_rep_tuple,	kaspkey_fields };

/*%
 * NSEC3 parameters.
 */
static keyword_type_t nsec3iter_kw = { "iterations", &cfg_type_uint32 };
static cfg_type_t cfg_type_nsec3iter = {
	"iterations",	       parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_uint32,		&nsec3iter_kw
};

static keyword_type_t nsec3optout_kw = { "optout", &cfg_type_boolean };
static cfg_type_t cfg_type_nsec3optout = {
	"optout",	  parse_optional_keyvalue,
	print_keyvalue,	  doc_optional_keyvalue,
	&cfg_rep_boolean, &nsec3optout_kw
};

static keyword_type_t nsec3salt_kw = { "salt-length", &cfg_type_uint32 };
static cfg_type_t cfg_type_nsec3salt = {
	"salt-length",	       parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_uint32,		&nsec3salt_kw
};

static cfg_tuplefielddef_t nsec3param_fields[] = {
	{ "iterations", &cfg_type_nsec3iter, 0 },
	{ "optout", &cfg_type_nsec3optout, 0 },
	{ "salt-length", &cfg_type_nsec3salt, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_nsec3 = { "nsec3param",    cfg_parse_tuple,
				     cfg_print_tuple, cfg_doc_tuple,
				     &cfg_rep_tuple,  nsec3param_fields };

/*%
 * Wild class, type, name.
 */
static keyword_type_t wild_class_kw = { "class", &cfg_type_ustring };

static cfg_type_t cfg_type_optional_wild_class = {
	"optional_wild_class", parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_string,		&wild_class_kw
};

static keyword_type_t wild_type_kw = { "type", &cfg_type_ustring };

static cfg_type_t cfg_type_optional_wild_type = {
	"optional_wild_type",  parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_string,		&wild_type_kw
};

static keyword_type_t wild_name_kw = { "name", &cfg_type_qstring };

static cfg_type_t cfg_type_optional_wild_name = {
	"optional_wild_name",  parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_string,		&wild_name_kw
};

/*%
 * An rrset ordering element.
 */
static cfg_tuplefielddef_t rrsetorderingelement_fields[] = {
	{ "class", &cfg_type_optional_wild_class, 0 },
	{ "type", &cfg_type_optional_wild_type, 0 },
	{ "name", &cfg_type_optional_wild_name, 0 },
	{ "order", &cfg_type_ustring, 0 }, /* must be literal "order" */
	{ "ordering", &cfg_type_ustring, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_rrsetorderingelement = {
	"rrsetorderingelement", cfg_parse_tuple, cfg_print_tuple,
	cfg_doc_tuple,		&cfg_rep_tuple,	 rrsetorderingelement_fields
};

/*%
 * A global or view "check-names" option.  Note that the zone
 * "check-names" option has a different syntax.
 */

static const char *checktype_enums[] = { "primary", "master",	"secondary",
					 "slave",   "response", NULL };
static cfg_type_t cfg_type_checktype = { "checktype",	    cfg_parse_enum,
					 cfg_print_ustring, cfg_doc_enum,
					 &cfg_rep_string,   &checktype_enums };

static const char *checkmode_enums[] = { "fail", "warn", "ignore", NULL };
static cfg_type_t cfg_type_checkmode = { "checkmode",	    cfg_parse_enum,
					 cfg_print_ustring, cfg_doc_enum,
					 &cfg_rep_string,   &checkmode_enums };

static const char *warn_enums[] = { "warn", "ignore", NULL };
static cfg_type_t cfg_type_warn = {
	"warn",	      cfg_parse_enum,  cfg_print_ustring,
	cfg_doc_enum, &cfg_rep_string, &warn_enums
};

static cfg_tuplefielddef_t checknames_fields[] = {
	{ "type", &cfg_type_checktype, 0 },
	{ "mode", &cfg_type_checkmode, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_checknames = { "checknames",	   cfg_parse_tuple,
					  cfg_print_tuple, cfg_doc_tuple,
					  &cfg_rep_tuple,  checknames_fields };

static cfg_type_t cfg_type_bracketed_dscpsockaddrlist = {
	"bracketed_sockaddrlist",
	cfg_parse_bracketed_list,
	cfg_print_bracketed_list,
	cfg_doc_bracketed_list,
	&cfg_rep_list,
	&cfg_type_sockaddrdscp
};

static cfg_type_t cfg_type_bracketed_netaddrlist = { "bracketed_netaddrlist",
						     cfg_parse_bracketed_list,
						     cfg_print_bracketed_list,
						     cfg_doc_bracketed_list,
						     &cfg_rep_list,
						     &cfg_type_netaddr };

static const char *autodnssec_enums[] = { "allow", "maintain", "off", NULL };
static cfg_type_t cfg_type_autodnssec = {
	"autodnssec", cfg_parse_enum,  cfg_print_ustring,
	cfg_doc_enum, &cfg_rep_string, &autodnssec_enums
};

static const char *dnssecupdatemode_enums[] = { "maintain", "no-resign", NULL };
static cfg_type_t cfg_type_dnssecupdatemode = {
	"dnssecupdatemode", cfg_parse_enum,  cfg_print_ustring,
	cfg_doc_enum,	    &cfg_rep_string, &dnssecupdatemode_enums
};

static const char *updatemethods_enums[] = { "date", "increment", "unixtime",
					     NULL };
static cfg_type_t cfg_type_updatemethod = {
	"updatemethod", cfg_parse_enum,	 cfg_print_ustring,
	cfg_doc_enum,	&cfg_rep_string, &updatemethods_enums
};

/*
 * zone-statistics: full, terse, or none.
 *
 * for backward compatibility, we also support boolean values.
 * yes represents "full", no represents "terse". in the future we
 * may change no to mean "none".
 */
static const char *zonestat_enums[] = { "full", "terse", "none", NULL };
static isc_result_t
parse_zonestat(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (cfg_parse_enum_or_other(pctx, type, &cfg_type_boolean, ret));
}
static void
doc_zonestat(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_doc_enum_or_other(pctx, type, &cfg_type_boolean);
}
static cfg_type_t cfg_type_zonestat = { "zonestat",	   parse_zonestat,
					cfg_print_ustring, doc_zonestat,
					&cfg_rep_string,   zonestat_enums };

static cfg_type_t cfg_type_rrsetorder = { "rrsetorder",
					  cfg_parse_bracketed_list,
					  cfg_print_bracketed_list,
					  cfg_doc_bracketed_list,
					  &cfg_rep_list,
					  &cfg_type_rrsetorderingelement };

static keyword_type_t dscp_kw = { "dscp", &cfg_type_uint32 };

static cfg_type_t cfg_type_optional_dscp = {
	"optional_dscp",       parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_uint32,		&dscp_kw
};

static keyword_type_t port_kw = { "port", &cfg_type_uint32 };

static cfg_type_t cfg_type_optional_port = {
	"optional_port",       parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_uint32,		&port_kw
};

/*% A list of keys, as in the "key" clause of the controls statement. */
static cfg_type_t cfg_type_keylist = { "keylist",
				       cfg_parse_bracketed_list,
				       cfg_print_bracketed_list,
				       cfg_doc_bracketed_list,
				       &cfg_rep_list,
				       &cfg_type_astring };

/*% A list of dnssec keys, as in "trusted-keys". Deprecated. */
static cfg_type_t cfg_type_trustedkeys = { "trustedkeys",
					   cfg_parse_bracketed_list,
					   cfg_print_bracketed_list,
					   cfg_doc_bracketed_list,
					   &cfg_rep_list,
					   &cfg_type_dnsseckey };

/*%
 * A list of managed trust anchors.  Each entry contains a name, a keyword
 * ("static-key", initial-key", "static-ds" or "initial-ds"), and the
 * fields associated with either a DNSKEY or a DS record.
 */
static cfg_type_t cfg_type_dnsseckeys = { "dnsseckeys",
					  cfg_parse_bracketed_list,
					  cfg_print_bracketed_list,
					  cfg_doc_bracketed_list,
					  &cfg_rep_list,
					  &cfg_type_managedkey };

/*%
 * A list of key entries, used in a DNSSEC Key and Signing Policy.
 */
static cfg_type_t cfg_type_kaspkeys = { "kaspkeys",
					cfg_parse_bracketed_list,
					cfg_print_bracketed_list,
					cfg_doc_bracketed_list,
					&cfg_rep_list,
					&cfg_type_kaspkey };

static const char *forwardtype_enums[] = { "first", "only", NULL };
static cfg_type_t cfg_type_forwardtype = {
	"forwardtype", cfg_parse_enum,	cfg_print_ustring,
	cfg_doc_enum,  &cfg_rep_string, &forwardtype_enums
};

static const char *zonetype_enums[] = {
	"primary",  "master",	       "secondary", "slave",
	"mirror",   "delegation-only", "forward",   "hint",
	"redirect", "static-stub",     "stub",	    NULL
};
static cfg_type_t cfg_type_zonetype = { "zonetype",	   cfg_parse_enum,
					cfg_print_ustring, cfg_doc_enum,
					&cfg_rep_string,   &zonetype_enums };

static const char *loglevel_enums[] = { "critical", "error", "warning",
					"notice",   "info",  "dynamic",
					NULL };
static cfg_type_t cfg_type_loglevel = { "loglevel",	   cfg_parse_enum,
					cfg_print_ustring, cfg_doc_enum,
					&cfg_rep_string,   &loglevel_enums };

static const char *transferformat_enums[] = { "many-answers", "one-answer",
					      NULL };
static cfg_type_t cfg_type_transferformat = {
	"transferformat", cfg_parse_enum,  cfg_print_ustring,
	cfg_doc_enum,	  &cfg_rep_string, &transferformat_enums
};

/*%
 * The special keyword "none", as used in the pid-file option.
 */

static void
print_none(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	UNUSED(obj);
	cfg_print_cstr(pctx, "none");
}

static cfg_type_t cfg_type_none = { "none", NULL,	   print_none,
				    NULL,   &cfg_rep_void, NULL };

/*%
 * A quoted string or the special keyword "none".  Used in the pid-file option.
 */
static isc_result_t
parse_qstringornone(cfg_parser_t *pctx, const cfg_type_t *type,
		    cfg_obj_t **ret) {
	isc_result_t result;

	CHECK(cfg_gettoken(pctx, CFG_LEXOPT_QSTRING));
	if (pctx->token.type == isc_tokentype_string &&
	    strcasecmp(TOKEN_STRING(pctx), "none") == 0)
	{
		return (cfg_create_obj(pctx, &cfg_type_none, ret));
	}
	cfg_ungettoken(pctx);
	return (cfg_parse_qstring(pctx, type, ret));
cleanup:
	return (result);
}

static void
doc_qstringornone(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "( <quoted_string> | none )");
}

static cfg_type_t cfg_type_qstringornone = { "qstringornone",
					     parse_qstringornone,
					     NULL,
					     doc_qstringornone,
					     NULL,
					     NULL };

/*%
 * A boolean ("yes" or "no"), or the special keyword "auto".
 * Used in the dnssec-validation option.
 */
static void
print_auto(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	UNUSED(obj);
	cfg_print_cstr(pctx, "auto");
}

static cfg_type_t cfg_type_auto = { "auto", NULL,	   print_auto,
				    NULL,   &cfg_rep_void, NULL };

static isc_result_t
parse_boolorauto(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;

	CHECK(cfg_gettoken(pctx, CFG_LEXOPT_QSTRING));
	if (pctx->token.type == isc_tokentype_string &&
	    strcasecmp(TOKEN_STRING(pctx), "auto") == 0)
	{
		return (cfg_create_obj(pctx, &cfg_type_auto, ret));
	}
	cfg_ungettoken(pctx);
	return (cfg_parse_boolean(pctx, type, ret));
cleanup:
	return (result);
}

static void
print_boolorauto(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	if (obj->type->rep == &cfg_rep_void) {
		cfg_print_cstr(pctx, "auto");
	} else if (obj->value.boolean) {
		cfg_print_cstr(pctx, "yes");
	} else {
		cfg_print_cstr(pctx, "no");
	}
}

static void
doc_boolorauto(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "( yes | no | auto )");
}

static cfg_type_t cfg_type_boolorauto = {
	"boolorauto", parse_boolorauto, print_boolorauto, doc_boolorauto, NULL,
	NULL
};

/*%
 * keyword hostname
 */
static void
print_hostname(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	UNUSED(obj);
	cfg_print_cstr(pctx, "hostname");
}

static cfg_type_t cfg_type_hostname = { "hostname",	  NULL,
					print_hostname,	  NULL,
					&cfg_rep_boolean, NULL };

/*%
 * "server-id" argument.
 */

static isc_result_t
parse_serverid(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	CHECK(cfg_gettoken(pctx, CFG_LEXOPT_QSTRING));
	if (pctx->token.type == isc_tokentype_string &&
	    strcasecmp(TOKEN_STRING(pctx), "none") == 0)
	{
		return (cfg_create_obj(pctx, &cfg_type_none, ret));
	}
	if (pctx->token.type == isc_tokentype_string &&
	    strcasecmp(TOKEN_STRING(pctx), "hostname") == 0)
	{
		result = cfg_create_obj(pctx, &cfg_type_hostname, ret);
		if (result == ISC_R_SUCCESS) {
			(*ret)->value.boolean = true;
		}
		return (result);
	}
	cfg_ungettoken(pctx);
	return (cfg_parse_qstring(pctx, type, ret));
cleanup:
	return (result);
}

static void
doc_serverid(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "( <quoted_string> | none | hostname )");
}

static cfg_type_t cfg_type_serverid = { "serverid",   parse_serverid, NULL,
					doc_serverid, NULL,	      NULL };

/*%
 * Port list.
 */
static void
print_porttuple(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	cfg_print_cstr(pctx, "range ");
	cfg_print_tuple(pctx, obj);
}
static cfg_tuplefielddef_t porttuple_fields[] = {
	{ "loport", &cfg_type_uint32, 0 },
	{ "hiport", &cfg_type_uint32, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_porttuple = { "porttuple",	  cfg_parse_tuple,
					 print_porttuple, cfg_doc_tuple,
					 &cfg_rep_tuple,  porttuple_fields };

static isc_result_t
parse_port(cfg_parser_t *pctx, cfg_obj_t **ret) {
	isc_result_t result;

	CHECK(cfg_parse_uint32(pctx, NULL, ret));
	if ((*ret)->value.uint32 > 0xffff) {
		cfg_parser_error(pctx, CFG_LOG_NEAR, "invalid port");
		cfg_obj_destroy(pctx, ret);
		result = ISC_R_RANGE;
	}

cleanup:
	return (result);
}

static isc_result_t
parse_portrange(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;

	UNUSED(type);

	CHECK(cfg_peektoken(pctx, ISC_LEXOPT_NUMBER | ISC_LEXOPT_CNUMBER));
	if (pctx->token.type == isc_tokentype_number) {
		CHECK(parse_port(pctx, ret));
	} else {
		CHECK(cfg_gettoken(pctx, 0));
		if (pctx->token.type != isc_tokentype_string ||
		    strcasecmp(TOKEN_STRING(pctx), "range") != 0)
		{
			cfg_parser_error(pctx, CFG_LOG_NEAR,
					 "expected integer or 'range'");
			return (ISC_R_UNEXPECTEDTOKEN);
		}
		CHECK(cfg_create_tuple(pctx, &cfg_type_porttuple, &obj));
		CHECK(parse_port(pctx, &obj->value.tuple[0]));
		CHECK(parse_port(pctx, &obj->value.tuple[1]));
		if (obj->value.tuple[0]->value.uint32 >
		    obj->value.tuple[1]->value.uint32)
		{
			cfg_parser_error(pctx, CFG_LOG_NOPREP,
					 "low port '%u' must not be larger "
					 "than high port",
					 obj->value.tuple[0]->value.uint32);
			result = ISC_R_RANGE;
			goto cleanup;
		}
		*ret = obj;
		obj = NULL;
	}

cleanup:
	if (obj != NULL) {
		cfg_obj_destroy(pctx, &obj);
	}
	return (result);
}

static cfg_type_t cfg_type_portrange = { "portrange", parse_portrange,
					 NULL,	      cfg_doc_terminal,
					 NULL,	      NULL };

static cfg_type_t cfg_type_bracketed_portlist = { "bracketed_sockaddrlist",
						  cfg_parse_bracketed_list,
						  cfg_print_bracketed_list,
						  cfg_doc_bracketed_list,
						  &cfg_rep_list,
						  &cfg_type_portrange };

static const char *cookiealg_enums[] = { "aes", "siphash24", NULL };
static cfg_type_t cfg_type_cookiealg = { "cookiealg",	    cfg_parse_enum,
					 cfg_print_ustring, cfg_doc_enum,
					 &cfg_rep_string,   &cookiealg_enums };

/*%
 * fetch-quota-params
 */

static cfg_tuplefielddef_t fetchquota_fields[] = {
	{ "frequency", &cfg_type_uint32, 0 },
	{ "low", &cfg_type_fixedpoint, 0 },
	{ "high", &cfg_type_fixedpoint, 0 },
	{ "discount", &cfg_type_fixedpoint, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_fetchquota = { "fetchquota",	   cfg_parse_tuple,
					  cfg_print_tuple, cfg_doc_tuple,
					  &cfg_rep_tuple,  fetchquota_fields };

/*%
 * fetches-per-server or fetches-per-zone
 */

static const char *response_enums[] = { "drop", "fail", NULL };

static cfg_type_t cfg_type_responsetype = {
	"responsetype",	   parse_optional_enum, cfg_print_ustring,
	doc_optional_enum, &cfg_rep_string,	response_enums
};

static cfg_tuplefielddef_t fetchesper_fields[] = {
	{ "fetches", &cfg_type_uint32, 0 },
	{ "response", &cfg_type_responsetype, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_fetchesper = { "fetchesper",	   cfg_parse_tuple,
					  cfg_print_tuple, cfg_doc_tuple,
					  &cfg_rep_tuple,  fetchesper_fields };

/*%
 * Clauses that can be found within the top level of the named.conf
 * file only.
 */
static cfg_clausedef_t namedconf_clauses[] = {
	{ "acl", &cfg_type_acl, CFG_CLAUSEFLAG_MULTI },
	{ "controls", &cfg_type_controls, CFG_CLAUSEFLAG_MULTI },
	{ "dnssec-policy", &cfg_type_dnssecpolicy, CFG_CLAUSEFLAG_MULTI },
	{ "logging", &cfg_type_logging, 0 },
	{ "lwres", &cfg_type_bracketed_text,
	  CFG_CLAUSEFLAG_MULTI | CFG_CLAUSEFLAG_OBSOLETE },
	{ "masters", &cfg_type_remoteservers, CFG_CLAUSEFLAG_MULTI },
	{ "options", &cfg_type_options, 0 },
	{ "parental-agents", &cfg_type_remoteservers, CFG_CLAUSEFLAG_MULTI },
	{ "primaries", &cfg_type_remoteservers, CFG_CLAUSEFLAG_MULTI },
	{ "statistics-channels", &cfg_type_statschannels,
	  CFG_CLAUSEFLAG_MULTI },
	{ "view", &cfg_type_view, CFG_CLAUSEFLAG_MULTI },
	{ NULL, NULL, 0 }
};

/*%
 * Clauses that can occur at the top level or in the view
 * statement, but not in the options block.
 */
static cfg_clausedef_t namedconf_or_view_clauses[] = {
	{ "dlz", &cfg_type_dlz, CFG_CLAUSEFLAG_MULTI },
	{ "dyndb", &cfg_type_dyndb, CFG_CLAUSEFLAG_MULTI },
	{ "key", &cfg_type_key, CFG_CLAUSEFLAG_MULTI },
	{ "managed-keys", &cfg_type_dnsseckeys,
	  CFG_CLAUSEFLAG_MULTI | CFG_CLAUSEFLAG_DEPRECATED },
	{ "plugin", &cfg_type_plugin, CFG_CLAUSEFLAG_MULTI },
	{ "server", &cfg_type_server, CFG_CLAUSEFLAG_MULTI },
	{ "trust-anchors", &cfg_type_dnsseckeys, CFG_CLAUSEFLAG_MULTI },
	{ "trusted-keys", &cfg_type_trustedkeys,
	  CFG_CLAUSEFLAG_MULTI | CFG_CLAUSEFLAG_DEPRECATED },
	{ "zone", &cfg_type_zone, CFG_CLAUSEFLAG_MULTI },
	{ NULL, NULL, 0 }
};

/*%
 * Clauses that can occur in the bind.keys file.
 */
static cfg_clausedef_t bindkeys_clauses[] = {
	{ "managed-keys", &cfg_type_dnsseckeys,
	  CFG_CLAUSEFLAG_MULTI | CFG_CLAUSEFLAG_DEPRECATED },
	{ "trust-anchors", &cfg_type_dnsseckeys, CFG_CLAUSEFLAG_MULTI },
	{ "trusted-keys", &cfg_type_trustedkeys,
	  CFG_CLAUSEFLAG_MULTI | CFG_CLAUSEFLAG_DEPRECATED },
	{ NULL, NULL, 0 }
};

static const char *fstrm_model_enums[] = { "mpsc", "spsc", NULL };
static cfg_type_t cfg_type_fstrm_model = {
	"model",      cfg_parse_enum,  cfg_print_ustring,
	cfg_doc_enum, &cfg_rep_string, &fstrm_model_enums
};

/*%
 * Clauses that can be found within the 'options' statement.
 */
static cfg_clausedef_t options_clauses[] = {
	{ "answer-cookie", &cfg_type_boolean, 0 },
	{ "automatic-interface-scan", &cfg_type_boolean, 0 },
	{ "avoid-v4-udp-ports", &cfg_type_bracketed_portlist, 0 },
	{ "avoid-v6-udp-ports", &cfg_type_bracketed_portlist, 0 },
	{ "bindkeys-file", &cfg_type_qstring, 0 },
	{ "blackhole", &cfg_type_bracketed_aml, 0 },
	{ "cookie-algorithm", &cfg_type_cookiealg, 0 },
	{ "cookie-secret", &cfg_type_sstring, CFG_CLAUSEFLAG_MULTI },
	{ "coresize", &cfg_type_size, 0 },
	{ "datasize", &cfg_type_size, 0 },
	{ "deallocate-on-exit", &cfg_type_boolean, CFG_CLAUSEFLAG_ANCIENT },
	{ "directory", &cfg_type_qstring, CFG_CLAUSEFLAG_CALLBACK },
#ifdef HAVE_DNSTAP
	{ "dnstap-output", &cfg_type_dnstapoutput, 0 },
	{ "dnstap-identity", &cfg_type_serverid, 0 },
	{ "dnstap-version", &cfg_type_qstringornone, 0 },
#else  /* ifdef HAVE_DNSTAP */
	{ "dnstap-output", &cfg_type_dnstapoutput,
	  CFG_CLAUSEFLAG_NOTCONFIGURED },
	{ "dnstap-identity", &cfg_type_serverid, CFG_CLAUSEFLAG_NOTCONFIGURED },
	{ "dnstap-version", &cfg_type_qstringornone,
	  CFG_CLAUSEFLAG_NOTCONFIGURED },
#endif /* ifdef HAVE_DNSTAP */
	{ "dscp", &cfg_type_uint32, CFG_CLAUSEFLAG_DEPRECATED },
	{ "dump-file", &cfg_type_qstring, 0 },
	{ "fake-iquery", &cfg_type_boolean, CFG_CLAUSEFLAG_ANCIENT },
	{ "files", &cfg_type_size, 0 },
	{ "flush-zones-on-shutdown", &cfg_type_boolean, 0 },
#ifdef HAVE_DNSTAP
	{ "fstrm-set-buffer-hint", &cfg_type_uint32, 0 },
	{ "fstrm-set-flush-timeout", &cfg_type_uint32, 0 },
	{ "fstrm-set-input-queue-size", &cfg_type_uint32, 0 },
	{ "fstrm-set-output-notify-threshold", &cfg_type_uint32, 0 },
	{ "fstrm-set-output-queue-model", &cfg_type_fstrm_model, 0 },
	{ "fstrm-set-output-queue-size", &cfg_type_uint32, 0 },
	{ "fstrm-set-reopen-interval", &cfg_type_duration, 0 },
#else  /* ifdef HAVE_DNSTAP */
	{ "fstrm-set-buffer-hint", &cfg_type_uint32,
	  CFG_CLAUSEFLAG_NOTCONFIGURED },
	{ "fstrm-set-flush-timeout", &cfg_type_uint32,
	  CFG_CLAUSEFLAG_NOTCONFIGURED },
	{ "fstrm-set-input-queue-size", &cfg_type_uint32,
	  CFG_CLAUSEFLAG_NOTCONFIGURED },
	{ "fstrm-set-output-notify-threshold", &cfg_type_uint32,
	  CFG_CLAUSEFLAG_NOTCONFIGURED },
	{ "fstrm-set-output-queue-model", &cfg_type_fstrm_model,
	  CFG_CLAUSEFLAG_NOTCONFIGURED },
	{ "fstrm-set-output-queue-size", &cfg_type_uint32,
	  CFG_CLAUSEFLAG_NOTCONFIGURED },
	{ "fstrm-set-reopen-interval", &cfg_type_duration,
	  CFG_CLAUSEFLAG_NOTCONFIGURED },
#endif /* HAVE_DNSTAP */
#if defined(HAVE_GEOIP2)
	{ "geoip-directory", &cfg_type_qstringornone, 0 },
#else  /* if defined(HAVE_GEOIP2) */
	{ "geoip-directory", &cfg_type_qstringornone,
	  CFG_CLAUSEFLAG_NOTCONFIGURED },
#endif /* HAVE_GEOIP2 */
	{ "geoip-use-ecs", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "has-old-clients", &cfg_type_boolean, CFG_CLAUSEFLAG_ANCIENT },
	{ "heartbeat-interval", &cfg_type_uint32, 0 },
	{ "host-statistics", &cfg_type_boolean, CFG_CLAUSEFLAG_ANCIENT },
	{ "host-statistics-max", &cfg_type_uint32, CFG_CLAUSEFLAG_ANCIENT },
	{ "hostname", &cfg_type_qstringornone, 0 },
	{ "interface-interval", &cfg_type_duration, 0 },
	{ "keep-response-order", &cfg_type_bracketed_aml, 0 },
	{ "listen-on", &cfg_type_listenon, CFG_CLAUSEFLAG_MULTI },
	{ "listen-on-v6", &cfg_type_listenon, CFG_CLAUSEFLAG_MULTI },
	{ "lock-file", &cfg_type_qstringornone, 0 },
	{ "managed-keys-directory", &cfg_type_qstring, 0 },
	{ "match-mapped-addresses", &cfg_type_boolean, 0 },
	{ "max-rsa-exponent-size", &cfg_type_uint32, 0 },
	{ "memstatistics", &cfg_type_boolean, 0 },
	{ "memstatistics-file", &cfg_type_qstring, 0 },
	{ "multiple-cnames", &cfg_type_boolean, CFG_CLAUSEFLAG_ANCIENT },
	{ "named-xfer", &cfg_type_qstring, CFG_CLAUSEFLAG_ANCIENT },
	{ "notify-rate", &cfg_type_uint32, 0 },
	{ "pid-file", &cfg_type_qstringornone, 0 },
	{ "port", &cfg_type_uint32, 0 },
	{ "querylog", &cfg_type_boolean, 0 },
	{ "random-device", &cfg_type_qstringornone, 0 },
	{ "recursing-file", &cfg_type_qstring, 0 },
	{ "recursive-clients", &cfg_type_uint32, 0 },
	{ "reuseport", &cfg_type_boolean, 0 },
	{ "reserved-sockets", &cfg_type_uint32, 0 },
	{ "secroots-file", &cfg_type_qstring, 0 },
	{ "serial-queries", &cfg_type_uint32, CFG_CLAUSEFLAG_ANCIENT },
	{ "serial-query-rate", &cfg_type_uint32, 0 },
	{ "server-id", &cfg_type_serverid, 0 },
	{ "session-keyalg", &cfg_type_astring, 0 },
	{ "session-keyfile", &cfg_type_qstringornone, 0 },
	{ "session-keyname", &cfg_type_astring, 0 },
	{ "sit-secret", &cfg_type_sstring, CFG_CLAUSEFLAG_OBSOLETE },
	{ "stacksize", &cfg_type_size, 0 },
	{ "startup-notify-rate", &cfg_type_uint32, 0 },
	{ "statistics-file", &cfg_type_qstring, 0 },
	{ "statistics-interval", &cfg_type_uint32, CFG_CLAUSEFLAG_ANCIENT },
	{ "tcp-advertised-timeout", &cfg_type_uint32, 0 },
	{ "tcp-clients", &cfg_type_uint32, 0 },
	{ "tcp-idle-timeout", &cfg_type_uint32, 0 },
	{ "tcp-initial-timeout", &cfg_type_uint32, 0 },
	{ "tcp-keepalive-timeout", &cfg_type_uint32, 0 },
	{ "tcp-listen-queue", &cfg_type_uint32, 0 },
	{ "tkey-dhkey", &cfg_type_tkey_dhkey, 0 },
	{ "tkey-domain", &cfg_type_qstring, 0 },
	{ "tkey-gssapi-credential", &cfg_type_qstring, 0 },
	{ "tkey-gssapi-keytab", &cfg_type_qstring, 0 },
	{ "transfer-message-size", &cfg_type_uint32, 0 },
	{ "transfers-in", &cfg_type_uint32, 0 },
	{ "transfers-out", &cfg_type_uint32, 0 },
	{ "transfers-per-ns", &cfg_type_uint32, 0 },
	{ "treat-cr-as-space", &cfg_type_boolean, CFG_CLAUSEFLAG_ANCIENT },
	{ "update-quota", &cfg_type_uint32, 0 },
	{ "use-id-pool", &cfg_type_boolean, CFG_CLAUSEFLAG_ANCIENT },
	{ "use-ixfr", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "use-v4-udp-ports", &cfg_type_bracketed_portlist, 0 },
	{ "use-v6-udp-ports", &cfg_type_bracketed_portlist, 0 },
	{ "version", &cfg_type_qstringornone, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_namelist = { "namelist",
					cfg_parse_bracketed_list,
					cfg_print_bracketed_list,
					cfg_doc_bracketed_list,
					&cfg_rep_list,
					&cfg_type_astring };

static keyword_type_t exclude_kw = { "exclude", &cfg_type_namelist };

static cfg_type_t cfg_type_optional_exclude = {
	"optional_exclude",    parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_list,		&exclude_kw
};

static keyword_type_t exceptionnames_kw = { "except-from", &cfg_type_namelist };

static cfg_type_t cfg_type_optional_exceptionnames = {
	"optional_allow",      parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_list,		&exceptionnames_kw
};

static cfg_tuplefielddef_t denyaddresses_fields[] = {
	{ "acl", &cfg_type_bracketed_aml, 0 },
	{ "except-from", &cfg_type_optional_exceptionnames, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_denyaddresses = {
	"denyaddresses", cfg_parse_tuple, cfg_print_tuple,
	cfg_doc_tuple,	 &cfg_rep_tuple,  denyaddresses_fields
};

static cfg_tuplefielddef_t denyaliases_fields[] = {
	{ "name", &cfg_type_namelist, 0 },
	{ "except-from", &cfg_type_optional_exceptionnames, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_denyaliases = {
	"denyaliases", cfg_parse_tuple, cfg_print_tuple,
	cfg_doc_tuple, &cfg_rep_tuple,	denyaliases_fields
};

static cfg_type_t cfg_type_algorithmlist = { "algorithmlist",
					     cfg_parse_bracketed_list,
					     cfg_print_bracketed_list,
					     cfg_doc_bracketed_list,
					     &cfg_rep_list,
					     &cfg_type_astring };

static cfg_tuplefielddef_t disablealgorithm_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "algorithms", &cfg_type_algorithmlist, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_disablealgorithm = {
	"disablealgorithm", cfg_parse_tuple, cfg_print_tuple,
	cfg_doc_tuple,	    &cfg_rep_tuple,  disablealgorithm_fields
};

static cfg_type_t cfg_type_dsdigestlist = { "dsdigestlist",
					    cfg_parse_bracketed_list,
					    cfg_print_bracketed_list,
					    cfg_doc_bracketed_list,
					    &cfg_rep_list,
					    &cfg_type_astring };

static cfg_tuplefielddef_t disabledsdigest_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "digests", &cfg_type_dsdigestlist, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_disabledsdigest = {
	"disabledsdigest", cfg_parse_tuple, cfg_print_tuple,
	cfg_doc_tuple,	   &cfg_rep_tuple,  disabledsdigest_fields
};

static cfg_tuplefielddef_t mustbesecure_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "value", &cfg_type_boolean, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_mustbesecure = {
	"mustbesecure", cfg_parse_tuple, cfg_print_tuple,
	cfg_doc_tuple,	&cfg_rep_tuple,	 mustbesecure_fields
};

static const char *masterformat_enums[] = { "map", "raw", "text", NULL };
static cfg_type_t cfg_type_masterformat = {
	"masterformat", cfg_parse_enum,	 cfg_print_ustring,
	cfg_doc_enum,	&cfg_rep_string, &masterformat_enums
};

static const char *masterstyle_enums[] = { "full", "relative", NULL };
static cfg_type_t cfg_type_masterstyle = {
	"masterstyle", cfg_parse_enum,	cfg_print_ustring,
	cfg_doc_enum,  &cfg_rep_string, &masterstyle_enums
};

static keyword_type_t blocksize_kw = { "block-size", &cfg_type_uint32 };

static cfg_type_t cfg_type_blocksize = { "blocksize",	  parse_keyvalue,
					 print_keyvalue,  doc_keyvalue,
					 &cfg_rep_uint32, &blocksize_kw };

static cfg_tuplefielddef_t resppadding_fields[] = {
	{ "acl", &cfg_type_bracketed_aml, 0 },
	{ "block-size", &cfg_type_blocksize, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_resppadding = {
	"resppadding", cfg_parse_tuple, cfg_print_tuple,
	cfg_doc_tuple, &cfg_rep_tuple,	resppadding_fields
};

/*%
 *  dnstap {
 *      &lt;message type&gt; [query | response] ;
 *      ...
 *  }
 *
 *  ... where message type is one of: client, resolver, auth, forwarder,
 *                                    update, all
 */
static const char *dnstap_types[] = { "all",	   "auth",     "client",
				      "forwarder", "resolver", "update",
				      NULL };

static const char *dnstap_modes[] = { "query", "response", NULL };

static cfg_type_t cfg_type_dnstap_type = { "dnstap_type",     cfg_parse_enum,
					   cfg_print_ustring, cfg_doc_enum,
					   &cfg_rep_string,   dnstap_types };

static cfg_type_t cfg_type_dnstap_mode = {
	"dnstap_mode",	   parse_optional_enum, cfg_print_ustring,
	doc_optional_enum, &cfg_rep_string,	dnstap_modes
};

static cfg_tuplefielddef_t dnstap_fields[] = {
	{ "type", &cfg_type_dnstap_type, 0 },
	{ "mode", &cfg_type_dnstap_mode, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_dnstap_entry = { "dnstap_value",  cfg_parse_tuple,
					    cfg_print_tuple, cfg_doc_tuple,
					    &cfg_rep_tuple,  dnstap_fields };

static cfg_type_t cfg_type_dnstap = { "dnstap",
				      cfg_parse_bracketed_list,
				      cfg_print_bracketed_list,
				      cfg_doc_bracketed_list,
				      &cfg_rep_list,
				      &cfg_type_dnstap_entry };

/*%
 * dnstap-output
 */
static isc_result_t
parse_dtout(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	const cfg_tuplefielddef_t *fields = type->of;

	CHECK(cfg_create_tuple(pctx, type, &obj));

	/* Parse the mandatory "mode" and "path" fields */
	CHECK(cfg_parse_obj(pctx, fields[0].type, &obj->value.tuple[0]));
	CHECK(cfg_parse_obj(pctx, fields[1].type, &obj->value.tuple[1]));

	/* Parse "versions" and "size" fields in any order. */
	for (;;) {
		CHECK(cfg_peektoken(pctx, 0));
		if (pctx->token.type == isc_tokentype_string) {
			CHECK(cfg_gettoken(pctx, 0));
			if (strcasecmp(TOKEN_STRING(pctx), "size") == 0 &&
			    obj->value.tuple[2] == NULL)
			{
				CHECK(cfg_parse_obj(pctx, fields[2].type,
						    &obj->value.tuple[2]));
			} else if (strcasecmp(TOKEN_STRING(pctx), "versions") ==
					   0 &&
				   obj->value.tuple[3] == NULL)
			{
				CHECK(cfg_parse_obj(pctx, fields[3].type,
						    &obj->value.tuple[3]));
			} else if (strcasecmp(TOKEN_STRING(pctx), "suffix") ==
					   0 &&
				   obj->value.tuple[4] == NULL)
			{
				CHECK(cfg_parse_obj(pctx, fields[4].type,
						    &obj->value.tuple[4]));
			} else {
				cfg_parser_error(pctx, CFG_LOG_NEAR,
						 "unexpected token");
				result = ISC_R_UNEXPECTEDTOKEN;
				goto cleanup;
			}
		} else {
			break;
		}
	}

	/* Create void objects for missing optional values. */
	if (obj->value.tuple[2] == NULL) {
		CHECK(cfg_parse_void(pctx, NULL, &obj->value.tuple[2]));
	}
	if (obj->value.tuple[3] == NULL) {
		CHECK(cfg_parse_void(pctx, NULL, &obj->value.tuple[3]));
	}
	if (obj->value.tuple[4] == NULL) {
		CHECK(cfg_parse_void(pctx, NULL, &obj->value.tuple[4]));
	}

	*ret = obj;
	return (ISC_R_SUCCESS);

cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

static void
print_dtout(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	cfg_print_obj(pctx, obj->value.tuple[0]); /* mode */
	cfg_print_obj(pctx, obj->value.tuple[1]); /* file */
	if (obj->value.tuple[2]->type->print != cfg_print_void) {
		cfg_print_cstr(pctx, " size ");
		cfg_print_obj(pctx, obj->value.tuple[2]);
	}
	if (obj->value.tuple[3]->type->print != cfg_print_void) {
		cfg_print_cstr(pctx, " versions ");
		cfg_print_obj(pctx, obj->value.tuple[3]);
	}
	if (obj->value.tuple[4]->type->print != cfg_print_void) {
		cfg_print_cstr(pctx, " suffix ");
		cfg_print_obj(pctx, obj->value.tuple[4]);
	}
}

static void
doc_dtout(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "( file | unix ) <quoted_string>");
	cfg_print_cstr(pctx, " ");
	cfg_print_cstr(pctx, "[ size ( unlimited | <size> ) ]");
	cfg_print_cstr(pctx, " ");
	cfg_print_cstr(pctx, "[ versions ( unlimited | <integer> ) ]");
	cfg_print_cstr(pctx, " ");
	cfg_print_cstr(pctx, "[ suffix ( increment | timestamp ) ]");
}

static const char *dtoutmode_enums[] = { "file", "unix", NULL };
static cfg_type_t cfg_type_dtmode = { "dtmode",		 cfg_parse_enum,
				      cfg_print_ustring, cfg_doc_enum,
				      &cfg_rep_string,	 &dtoutmode_enums };

static cfg_tuplefielddef_t dtout_fields[] = {
	{ "mode", &cfg_type_dtmode, 0 },
	{ "path", &cfg_type_qstring, 0 },
	{ "size", &cfg_type_sizenodefault, 0 },
	{ "versions", &cfg_type_logversions, 0 },
	{ "suffix", &cfg_type_logsuffix, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_dnstapoutput = { "dnstapoutput", parse_dtout,
					    print_dtout,    doc_dtout,
					    &cfg_rep_tuple, dtout_fields };

/*%
 *  response-policy {
 *	zone &lt;string&gt; [ policy (given|disabled|passthru|drop|tcp-only|
 *					nxdomain|nodata|cname &lt;domain&gt; ) ]
 *		      [ recursive-only yes|no ] [ log yes|no ]
 *		      [ max-policy-ttl number ]
 *		      [ nsip-enable yes|no ] [ nsdname-enable yes|no ];
 *  } [ recursive-only yes|no ] [ max-policy-ttl number ]
 *	 [ min-update-interval number ]
 *	 [ break-dnssec yes|no ] [ min-ns-dots number ]
 *	 [ qname-wait-recurse yes|no ]
 *	 [ nsip-enable yes|no ] [ nsdname-enable yes|no ]
 *	 [ dnsrps-enable yes|no ]
 *	 [ dnsrps-options { DNSRPS configuration string } ];
 */

static void
doc_rpz_policy(cfg_printer_t *pctx, const cfg_type_t *type) {
	const char *const *p;
	/*
	 * This is cfg_doc_enum() without the trailing " )".
	 */
	cfg_print_cstr(pctx, "( ");
	for (p = type->of; *p != NULL; p++) {
		cfg_print_cstr(pctx, *p);
		if (p[1] != NULL) {
			cfg_print_cstr(pctx, " | ");
		}
	}
}

static void
doc_rpz_cname(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_doc_terminal(pctx, type);
	cfg_print_cstr(pctx, " )");
}

/*
 * Parse
 *	given|disabled|passthru|drop|tcp-only|nxdomain|nodata|cname <domain>
 */
static isc_result_t
cfg_parse_rpz_policy(cfg_parser_t *pctx, const cfg_type_t *type,
		     cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	const cfg_tuplefielddef_t *fields;

	CHECK(cfg_create_tuple(pctx, type, &obj));

	fields = type->of;
	CHECK(cfg_parse_obj(pctx, fields[0].type, &obj->value.tuple[0]));
	/*
	 * parse cname domain only after "policy cname"
	 */
	if (strcasecmp("cname", cfg_obj_asstring(obj->value.tuple[0])) != 0) {
		CHECK(cfg_parse_void(pctx, NULL, &obj->value.tuple[1]));
	} else {
		CHECK(cfg_parse_obj(pctx, fields[1].type,
				    &obj->value.tuple[1]));
	}

	*ret = obj;
	return (ISC_R_SUCCESS);

cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

/*
 * Parse a tuple consisting of any kind of required field followed
 * by 2 or more optional keyvalues that can be in any order.
 */
static isc_result_t
cfg_parse_kv_tuple(cfg_parser_t *pctx, const cfg_type_t *type,
		   cfg_obj_t **ret) {
	const cfg_tuplefielddef_t *fields, *f;
	cfg_obj_t *obj = NULL;
	int fn;
	isc_result_t result;

	CHECK(cfg_create_tuple(pctx, type, &obj));

	/*
	 * The zone first field is required and always first.
	 */
	fields = type->of;
	CHECK(cfg_parse_obj(pctx, fields[0].type, &obj->value.tuple[0]));

	for (;;) {
		CHECK(cfg_peektoken(pctx, CFG_LEXOPT_QSTRING));
		if (pctx->token.type != isc_tokentype_string) {
			break;
		}

		for (fn = 1, f = &fields[1];; ++fn, ++f) {
			if (f->name == NULL) {
				cfg_parser_error(pctx, 0, "unexpected '%s'",
						 TOKEN_STRING(pctx));
				result = ISC_R_UNEXPECTEDTOKEN;
				goto cleanup;
			}
			if (obj->value.tuple[fn] == NULL &&
			    strcasecmp(f->name, TOKEN_STRING(pctx)) == 0)
			{
				break;
			}
		}

		CHECK(cfg_gettoken(pctx, 0));
		CHECK(cfg_parse_obj(pctx, f->type, &obj->value.tuple[fn]));
	}

	for (fn = 1, f = &fields[1]; f->name != NULL; ++fn, ++f) {
		if (obj->value.tuple[fn] == NULL) {
			CHECK(cfg_parse_void(pctx, NULL,
					     &obj->value.tuple[fn]));
		}
	}

	*ret = obj;
	return (ISC_R_SUCCESS);

cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

static void
cfg_print_kv_tuple(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	unsigned int i;
	const cfg_tuplefielddef_t *fields, *f;
	const cfg_obj_t *fieldobj;

	fields = obj->type->of;
	for (f = fields, i = 0; f->name != NULL; f++, i++) {
		fieldobj = obj->value.tuple[i];
		if (fieldobj->type->print == cfg_print_void) {
			continue;
		}
		if (i != 0) {
			cfg_print_cstr(pctx, " ");
			cfg_print_cstr(pctx, f->name);
			cfg_print_cstr(pctx, " ");
		}
		cfg_print_obj(pctx, fieldobj);
	}
}

static void
cfg_doc_kv_tuple(cfg_printer_t *pctx, const cfg_type_t *type) {
	const cfg_tuplefielddef_t *fields, *f;

	fields = type->of;
	for (f = fields; f->name != NULL; f++) {
		if (f != fields) {
			cfg_print_cstr(pctx, " [ ");
			cfg_print_cstr(pctx, f->name);
			if (f->type->doc != cfg_doc_void) {
				cfg_print_cstr(pctx, " ");
			}
		}
		cfg_doc_obj(pctx, f->type);
		if (f != fields) {
			cfg_print_cstr(pctx, " ]");
		}
	}
}

static keyword_type_t zone_kw = { "zone", &cfg_type_astring };
static cfg_type_t cfg_type_rpz_zone = { "zone",		 parse_keyvalue,
					print_keyvalue,	 doc_keyvalue,
					&cfg_rep_string, &zone_kw };
/*
 * "no-op" is an obsolete equivalent of "passthru".
 */
static const char *rpz_policies[] = { "cname",	  "disabled", "drop",
				      "given",	  "no-op",    "nodata",
				      "nxdomain", "passthru", "tcp-only",
				      NULL };
static cfg_type_t cfg_type_rpz_policy_name = {
	"policy name",	cfg_parse_enum,	 cfg_print_ustring,
	doc_rpz_policy, &cfg_rep_string, &rpz_policies
};
static cfg_type_t cfg_type_rpz_cname = {
	"quoted_string", cfg_parse_astring, NULL,
	doc_rpz_cname,	 &cfg_rep_string,   NULL
};
static cfg_tuplefielddef_t rpz_policy_fields[] = {
	{ "policy name", &cfg_type_rpz_policy_name, 0 },
	{ "cname", &cfg_type_rpz_cname, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_rpz_policy = { "policy tuple",  cfg_parse_rpz_policy,
					  cfg_print_tuple, cfg_doc_tuple,
					  &cfg_rep_tuple,  rpz_policy_fields };
static cfg_tuplefielddef_t rpz_zone_fields[] = {
	{ "zone name", &cfg_type_rpz_zone, 0 },
	{ "add-soa", &cfg_type_boolean, 0 },
	{ "log", &cfg_type_boolean, 0 },
	{ "max-policy-ttl", &cfg_type_duration, 0 },
	{ "min-update-interval", &cfg_type_duration, 0 },
	{ "policy", &cfg_type_rpz_policy, 0 },
	{ "recursive-only", &cfg_type_boolean, 0 },
	{ "nsip-enable", &cfg_type_boolean, 0 },
	{ "nsdname-enable", &cfg_type_boolean, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_rpz_tuple = { "rpz tuple",	     cfg_parse_kv_tuple,
					 cfg_print_kv_tuple, cfg_doc_kv_tuple,
					 &cfg_rep_tuple,     rpz_zone_fields };
static cfg_type_t cfg_type_rpz_list = { "zone list",
					cfg_parse_bracketed_list,
					cfg_print_bracketed_list,
					cfg_doc_bracketed_list,
					&cfg_rep_list,
					&cfg_type_rpz_tuple };
static cfg_tuplefielddef_t rpz_fields[] = {
	{ "zone list", &cfg_type_rpz_list, 0 },
	{ "add-soa", &cfg_type_boolean, 0 },
	{ "break-dnssec", &cfg_type_boolean, 0 },
	{ "max-policy-ttl", &cfg_type_duration, 0 },
	{ "min-update-interval", &cfg_type_duration, 0 },
	{ "min-ns-dots", &cfg_type_uint32, 0 },
	{ "nsip-wait-recurse", &cfg_type_boolean, 0 },
	{ "qname-wait-recurse", &cfg_type_boolean, 0 },
	{ "recursive-only", &cfg_type_boolean, 0 },
	{ "nsip-enable", &cfg_type_boolean, 0 },
	{ "nsdname-enable", &cfg_type_boolean, 0 },
#ifdef USE_DNSRPS
	{ "dnsrps-enable", &cfg_type_boolean, 0 },
	{ "dnsrps-options", &cfg_type_bracketed_text, 0 },
#else  /* ifdef USE_DNSRPS */
	{ "dnsrps-enable", &cfg_type_boolean, CFG_CLAUSEFLAG_NOTCONFIGURED },
	{ "dnsrps-options", &cfg_type_bracketed_text,
	  CFG_CLAUSEFLAG_NOTCONFIGURED },
#endif /* ifdef USE_DNSRPS */
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_rpz = { "rpz",
				   cfg_parse_kv_tuple,
				   cfg_print_kv_tuple,
				   cfg_doc_kv_tuple,
				   &cfg_rep_tuple,
				   rpz_fields };

/*
 * Catalog zones
 */
static cfg_type_t cfg_type_catz_zone = { "zone",	  parse_keyvalue,
					 print_keyvalue,  doc_keyvalue,
					 &cfg_rep_string, &zone_kw };

static cfg_tuplefielddef_t catz_zone_fields[] = {
	{ "zone name", &cfg_type_catz_zone, 0 },
	{ "default-masters", &cfg_type_namesockaddrkeylist, 0 },
	{ "zone-directory", &cfg_type_qstring, 0 },
	{ "in-memory", &cfg_type_boolean, 0 },
	{ "min-update-interval", &cfg_type_duration, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_catz_tuple = {
	"catz tuple",	  cfg_parse_kv_tuple, cfg_print_kv_tuple,
	cfg_doc_kv_tuple, &cfg_rep_tuple,     catz_zone_fields
};
static cfg_type_t cfg_type_catz_list = { "zone list",
					 cfg_parse_bracketed_list,
					 cfg_print_bracketed_list,
					 cfg_doc_bracketed_list,
					 &cfg_rep_list,
					 &cfg_type_catz_tuple };
static cfg_tuplefielddef_t catz_fields[] = {
	{ "zone list", &cfg_type_catz_list, 0 }, { NULL, NULL, 0 }
};
static cfg_type_t cfg_type_catz = {
	"catz",		  cfg_parse_kv_tuple, cfg_print_kv_tuple,
	cfg_doc_kv_tuple, &cfg_rep_tuple,     catz_fields
};

/*
 * rate-limit
 */
static cfg_clausedef_t rrl_clauses[] = {
	{ "all-per-second", &cfg_type_uint32, 0 },
	{ "errors-per-second", &cfg_type_uint32, 0 },
	{ "exempt-clients", &cfg_type_bracketed_aml, 0 },
	{ "ipv4-prefix-length", &cfg_type_uint32, 0 },
	{ "ipv6-prefix-length", &cfg_type_uint32, 0 },
	{ "log-only", &cfg_type_boolean, 0 },
	{ "max-table-size", &cfg_type_uint32, 0 },
	{ "min-table-size", &cfg_type_uint32, 0 },
	{ "nodata-per-second", &cfg_type_uint32, 0 },
	{ "nxdomains-per-second", &cfg_type_uint32, 0 },
	{ "qps-scale", &cfg_type_uint32, 0 },
	{ "referrals-per-second", &cfg_type_uint32, 0 },
	{ "responses-per-second", &cfg_type_uint32, 0 },
	{ "slip", &cfg_type_uint32, 0 },
	{ "window", &cfg_type_uint32, 0 },
	{ NULL, NULL, 0 }
};

static cfg_clausedef_t *rrl_clausesets[] = { rrl_clauses, NULL };

static cfg_type_t cfg_type_rrl = { "rate-limit", cfg_parse_map, cfg_print_map,
				   cfg_doc_map,	 &cfg_rep_map,	rrl_clausesets };

/*%
 * dnssec-lookaside
 */

static void
print_lookaside(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	const cfg_obj_t *domain = obj->value.tuple[0];

	if (domain->value.string.length == 4 &&
	    strncmp(domain->value.string.base, "auto", 4) == 0)
	{
		cfg_print_cstr(pctx, "auto");
	} else {
		cfg_print_tuple(pctx, obj);
	}
}

static void
doc_lookaside(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "( <string> trust-anchor <string> | auto | no )");
}

static keyword_type_t trustanchor_kw = { "trust-anchor", &cfg_type_astring };

static cfg_type_t cfg_type_optional_trustanchor = {
	"optional_trustanchor", parse_optional_keyvalue, print_keyvalue,
	doc_keyvalue,		&cfg_rep_string,	 &trustanchor_kw
};

static cfg_tuplefielddef_t lookaside_fields[] = {
	{ "domain", &cfg_type_astring, 0 },
	{ "trust-anchor", &cfg_type_optional_trustanchor, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_lookaside = { "lookaside",	  cfg_parse_tuple,
					 print_lookaside, doc_lookaside,
					 &cfg_rep_tuple,  lookaside_fields };

static isc_result_t
parse_optional_uint32(cfg_parser_t *pctx, const cfg_type_t *type,
		      cfg_obj_t **ret) {
	isc_result_t result;
	UNUSED(type);

	CHECK(cfg_peektoken(pctx, ISC_LEXOPT_NUMBER | ISC_LEXOPT_CNUMBER));
	if (pctx->token.type == isc_tokentype_number) {
		CHECK(cfg_parse_obj(pctx, &cfg_type_uint32, ret));
	} else {
		CHECK(cfg_parse_obj(pctx, &cfg_type_void, ret));
	}
cleanup:
	return (result);
}

static void
doc_optional_uint32(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "[ <integer> ]");
}

static cfg_type_t cfg_type_optional_uint32 = { "optional_uint32",
					       parse_optional_uint32,
					       NULL,
					       doc_optional_uint32,
					       NULL,
					       NULL };

static cfg_tuplefielddef_t prefetch_fields[] = {
	{ "trigger", &cfg_type_uint32, 0 },
	{ "eligible", &cfg_type_optional_uint32, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_prefetch = { "prefetch",	 cfg_parse_tuple,
					cfg_print_tuple, cfg_doc_tuple,
					&cfg_rep_tuple,	 prefetch_fields };
/*
 * DNS64.
 */
static cfg_clausedef_t dns64_clauses[] = {
	{ "break-dnssec", &cfg_type_boolean, 0 },
	{ "clients", &cfg_type_bracketed_aml, 0 },
	{ "exclude", &cfg_type_bracketed_aml, 0 },
	{ "mapped", &cfg_type_bracketed_aml, 0 },
	{ "recursive-only", &cfg_type_boolean, 0 },
	{ "suffix", &cfg_type_netaddr6, 0 },
	{ NULL, NULL, 0 },
};

static cfg_clausedef_t *dns64_clausesets[] = { dns64_clauses, NULL };

static cfg_type_t cfg_type_dns64 = { "dns64",	    cfg_parse_netprefix_map,
				     cfg_print_map, cfg_doc_map,
				     &cfg_rep_map,  dns64_clausesets };

static const char *staleanswerclienttimeout_enums[] = { "disabled", "off",
							NULL };
static isc_result_t
parse_staleanswerclienttimeout(cfg_parser_t *pctx, const cfg_type_t *type,
			       cfg_obj_t **ret) {
	return (cfg_parse_enum_or_other(pctx, type, &cfg_type_uint32, ret));
}

static void
doc_staleanswerclienttimeout(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_doc_enum_or_other(pctx, type, &cfg_type_uint32);
}

static cfg_type_t cfg_type_staleanswerclienttimeout = {
	"staleanswerclienttimeout",
	parse_staleanswerclienttimeout,
	cfg_print_ustring,
	doc_staleanswerclienttimeout,
	&cfg_rep_string,
	staleanswerclienttimeout_enums
};

/*%
 * Clauses that can be found within the 'view' statement,
 * with defaults in the 'options' statement.
 */

static cfg_clausedef_t view_clauses[] = {
	{ "acache-cleaning-interval", &cfg_type_uint32,
	  CFG_CLAUSEFLAG_OBSOLETE },
	{ "acache-enable", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "additional-from-auth", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "additional-from-cache", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "allow-new-zones", &cfg_type_boolean, 0 },
	{ "allow-query-cache", &cfg_type_bracketed_aml, 0 },
	{ "allow-query-cache-on", &cfg_type_bracketed_aml, 0 },
	{ "allow-recursion", &cfg_type_bracketed_aml, 0 },
	{ "allow-recursion-on", &cfg_type_bracketed_aml, 0 },
	{ "allow-v6-synthesis", &cfg_type_bracketed_aml,
	  CFG_CLAUSEFLAG_OBSOLETE },
	{ "attach-cache", &cfg_type_astring, 0 },
	{ "auth-nxdomain", &cfg_type_boolean, CFG_CLAUSEFLAG_NEWDEFAULT },
	{ "cache-file", &cfg_type_qstring, CFG_CLAUSEFLAG_DEPRECATED },
	{ "catalog-zones", &cfg_type_catz, 0 },
	{ "check-names", &cfg_type_checknames, CFG_CLAUSEFLAG_MULTI },
	{ "cleaning-interval", &cfg_type_uint32, CFG_CLAUSEFLAG_OBSOLETE },
	{ "clients-per-query", &cfg_type_uint32, 0 },
	{ "deny-answer-addresses", &cfg_type_denyaddresses, 0 },
	{ "deny-answer-aliases", &cfg_type_denyaliases, 0 },
	{ "disable-algorithms", &cfg_type_disablealgorithm,
	  CFG_CLAUSEFLAG_MULTI },
	{ "disable-ds-digests", &cfg_type_disabledsdigest,
	  CFG_CLAUSEFLAG_MULTI },
	{ "disable-empty-zone", &cfg_type_astring, CFG_CLAUSEFLAG_MULTI },
	{ "dns64", &cfg_type_dns64, CFG_CLAUSEFLAG_MULTI },
	{ "dns64-contact", &cfg_type_astring, 0 },
	{ "dns64-server", &cfg_type_astring, 0 },
#ifdef USE_DNSRPS
	{ "dnsrps-enable", &cfg_type_boolean, 0 },
	{ "dnsrps-options", &cfg_type_bracketed_text, 0 },
#else  /* ifdef USE_DNSRPS */
	{ "dnsrps-enable", &cfg_type_boolean, CFG_CLAUSEFLAG_NOTCONFIGURED },
	{ "dnsrps-options", &cfg_type_bracketed_text,
	  CFG_CLAUSEFLAG_NOTCONFIGURED },
#endif /* ifdef USE_DNSRPS */
	{ "dnssec-accept-expired", &cfg_type_boolean, 0 },
	{ "dnssec-enable", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "dnssec-lookaside", &cfg_type_lookaside,
	  CFG_CLAUSEFLAG_MULTI | CFG_CLAUSEFLAG_OBSOLETE },
	{ "dnssec-must-be-secure", &cfg_type_mustbesecure,
	  CFG_CLAUSEFLAG_MULTI },
	{ "dnssec-validation", &cfg_type_boolorauto, 0 },
#ifdef HAVE_DNSTAP
	{ "dnstap", &cfg_type_dnstap, 0 },
#else  /* ifdef HAVE_DNSTAP */
	{ "dnstap", &cfg_type_dnstap, CFG_CLAUSEFLAG_NOTCONFIGURED },
#endif /* HAVE_DNSTAP */
	{ "dual-stack-servers", &cfg_type_nameportiplist, 0 },
	{ "edns-udp-size", &cfg_type_uint32, 0 },
	{ "empty-contact", &cfg_type_astring, 0 },
	{ "empty-server", &cfg_type_astring, 0 },
	{ "empty-zones-enable", &cfg_type_boolean, 0 },
	{ "fetch-glue", &cfg_type_boolean, CFG_CLAUSEFLAG_ANCIENT },
	{ "fetch-quota-params", &cfg_type_fetchquota, 0 },
	{ "fetches-per-server", &cfg_type_fetchesper, 0 },
	{ "fetches-per-zone", &cfg_type_fetchesper, 0 },
	{ "filter-aaaa", &cfg_type_bracketed_aml, CFG_CLAUSEFLAG_OBSOLETE },
	{ "filter-aaaa-on-v4", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "filter-aaaa-on-v6", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "glue-cache", &cfg_type_boolean, 0 },
	{ "ixfr-from-differences", &cfg_type_ixfrdifftype, 0 },
	{ "lame-ttl", &cfg_type_duration, 0 },
#ifdef HAVE_LMDB
	{ "lmdb-mapsize", &cfg_type_sizeval, 0 },
#else  /* ifdef HAVE_LMDB */
	{ "lmdb-mapsize", &cfg_type_sizeval, CFG_CLAUSEFLAG_NOOP },
#endif /* ifdef HAVE_LMDB */
	{ "max-acache-size", &cfg_type_sizenodefault, CFG_CLAUSEFLAG_OBSOLETE },
	{ "max-cache-size", &cfg_type_sizeorpercent, 0 },
	{ "max-cache-ttl", &cfg_type_duration, 0 },
	{ "max-clients-per-query", &cfg_type_uint32, 0 },
	{ "max-ncache-ttl", &cfg_type_duration, 0 },
	{ "max-recursion-depth", &cfg_type_uint32, 0 },
	{ "max-recursion-queries", &cfg_type_uint32, 0 },
	{ "max-stale-ttl", &cfg_type_duration, 0 },
	{ "max-udp-size", &cfg_type_uint32, 0 },
	{ "message-compression", &cfg_type_boolean, 0 },
	{ "min-cache-ttl", &cfg_type_duration, 0 },
	{ "min-ncache-ttl", &cfg_type_duration, 0 },
	{ "min-roots", &cfg_type_uint32, CFG_CLAUSEFLAG_ANCIENT },
	{ "minimal-any", &cfg_type_boolean, 0 },
	{ "minimal-responses", &cfg_type_minimal, 0 },
	{ "new-zones-directory", &cfg_type_qstring, 0 },
	{ "no-case-compress", &cfg_type_bracketed_aml, 0 },
	{ "nocookie-udp-size", &cfg_type_uint32, 0 },
	{ "nosit-udp-size", &cfg_type_uint32, CFG_CLAUSEFLAG_OBSOLETE },
	{ "nta-lifetime", &cfg_type_duration, 0 },
	{ "nta-recheck", &cfg_type_duration, 0 },
	{ "nxdomain-redirect", &cfg_type_astring, 0 },
	{ "preferred-glue", &cfg_type_astring, 0 },
	{ "prefetch", &cfg_type_prefetch, 0 },
	{ "provide-ixfr", &cfg_type_boolean, 0 },
	{ "qname-minimization", &cfg_type_qminmethod, 0 },
	/*
	 * Note that the query-source option syntax is different
	 * from the other -source options.
	 */
	{ "query-source", &cfg_type_querysource4, 0 },
	{ "query-source-v6", &cfg_type_querysource6, 0 },
	{ "queryport-pool-ports", &cfg_type_uint32, CFG_CLAUSEFLAG_OBSOLETE },
	{ "queryport-pool-updateinterval", &cfg_type_uint32,
	  CFG_CLAUSEFLAG_OBSOLETE },
	{ "rate-limit", &cfg_type_rrl, 0 },
	{ "recursion", &cfg_type_boolean, 0 },
	{ "request-nsid", &cfg_type_boolean, 0 },
	{ "request-sit", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "require-server-cookie", &cfg_type_boolean, 0 },
	{ "resolver-nonbackoff-tries", &cfg_type_uint32, 0 },
	{ "resolver-query-timeout", &cfg_type_uint32, 0 },
	{ "resolver-retry-interval", &cfg_type_uint32, 0 },
	{ "response-padding", &cfg_type_resppadding, 0 },
	{ "response-policy", &cfg_type_rpz, 0 },
	{ "rfc2308-type1", &cfg_type_boolean, CFG_CLAUSEFLAG_ANCIENT },
	{ "root-delegation-only", &cfg_type_optional_exclude, 0 },
	{ "root-key-sentinel", &cfg_type_boolean, 0 },
	{ "rrset-order", &cfg_type_rrsetorder, 0 },
	{ "send-cookie", &cfg_type_boolean, 0 },
	{ "servfail-ttl", &cfg_type_duration, 0 },
	{ "sortlist", &cfg_type_bracketed_aml, 0 },
	{ "stale-answer-enable", &cfg_type_boolean, 0 },
	{ "stale-answer-client-timeout", &cfg_type_staleanswerclienttimeout,
	  0 },
	{ "stale-answer-ttl", &cfg_type_duration, 0 },
	{ "stale-cache-enable", &cfg_type_boolean, 0 },
	{ "stale-refresh-time", &cfg_type_duration, 0 },
	{ "suppress-initial-notify", &cfg_type_boolean, CFG_CLAUSEFLAG_NYI },
	{ "synth-from-dnssec", &cfg_type_boolean, 0 },
	{ "topology", &cfg_type_bracketed_aml, CFG_CLAUSEFLAG_ANCIENT },
	{ "transfer-format", &cfg_type_transferformat, 0 },
	{ "trust-anchor-telemetry", &cfg_type_boolean,
	  CFG_CLAUSEFLAG_EXPERIMENTAL },
	{ "use-queryport-pool", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "validate-except", &cfg_type_namelist, 0 },
	{ "v6-bias", &cfg_type_uint32, 0 },
	{ "zero-no-soa-ttl-cache", &cfg_type_boolean, 0 },
	{ NULL, NULL, 0 }
};

/*%
 * Clauses that can be found within the 'view' statement only.
 */
static cfg_clausedef_t view_only_clauses[] = {
	{ "match-clients", &cfg_type_bracketed_aml, 0 },
	{ "match-destinations", &cfg_type_bracketed_aml, 0 },
	{ "match-recursive-only", &cfg_type_boolean, 0 },
	{ NULL, NULL, 0 }
};

/*%
 * Sig-validity-interval.
 */

static cfg_tuplefielddef_t validityinterval_fields[] = {
	{ "validity", &cfg_type_uint32, 0 },
	{ "re-sign", &cfg_type_optional_uint32, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_validityinterval = {
	"validityinterval", cfg_parse_tuple, cfg_print_tuple,
	cfg_doc_tuple,	    &cfg_rep_tuple,  validityinterval_fields
};

/*%
 * Clauses that can be found in a 'dnssec-policy' statement.
 */
static cfg_clausedef_t dnssecpolicy_clauses[] = {
	{ "dnskey-ttl", &cfg_type_duration, 0 },
	{ "keys", &cfg_type_kaspkeys, 0 },
	{ "max-zone-ttl", &cfg_type_duration, 0 },
	{ "nsec3param", &cfg_type_nsec3, 0 },
	{ "parent-ds-ttl", &cfg_type_duration, 0 },
	{ "parent-propagation-delay", &cfg_type_duration, 0 },
	{ "parent-registration-delay", &cfg_type_duration,
	  CFG_CLAUSEFLAG_OBSOLETE },
	{ "publish-safety", &cfg_type_duration, 0 },
	{ "purge-keys", &cfg_type_duration, 0 },
	{ "retire-safety", &cfg_type_duration, 0 },
	{ "signatures-refresh", &cfg_type_duration, 0 },
	{ "signatures-validity", &cfg_type_duration, 0 },
	{ "signatures-validity-dnskey", &cfg_type_duration, 0 },
	{ "zone-propagation-delay", &cfg_type_duration, 0 },
	{ NULL, NULL, 0 }
};

/*%
 * Clauses that can be found in a 'zone' statement,
 * with defaults in the 'view' or 'options' statement.
 *
 * Note: CFG_ZONE_* options indicate in which zone types this clause is
 * legal.
 */
static cfg_clausedef_t zone_clauses[] = {
	{ "allow-notify", &cfg_type_bracketed_aml,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "allow-query", &cfg_type_bracketed_aml,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR |
		  CFG_ZONE_STUB | CFG_ZONE_REDIRECT | CFG_ZONE_STATICSTUB },
	{ "allow-query-on", &cfg_type_bracketed_aml,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR |
		  CFG_ZONE_STUB | CFG_ZONE_REDIRECT | CFG_ZONE_STATICSTUB },
	{ "allow-transfer", &cfg_type_bracketed_aml,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "allow-update", &cfg_type_bracketed_aml, CFG_ZONE_PRIMARY },
	{ "allow-update-forwarding", &cfg_type_bracketed_aml,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "also-notify", &cfg_type_namesockaddrkeylist,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "alt-transfer-source", &cfg_type_sockaddr4wild,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "alt-transfer-source-v6", &cfg_type_sockaddr6wild,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "auto-dnssec", &cfg_type_autodnssec,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_CLAUSEFLAG_DEPRECATED },
	{ "check-dup-records", &cfg_type_checkmode, CFG_ZONE_PRIMARY },
	{ "check-integrity", &cfg_type_boolean, CFG_ZONE_PRIMARY },
	{ "check-mx", &cfg_type_checkmode, CFG_ZONE_PRIMARY },
	{ "check-mx-cname", &cfg_type_checkmode, CFG_ZONE_PRIMARY },
	{ "check-sibling", &cfg_type_boolean, CFG_ZONE_PRIMARY },
	{ "check-spf", &cfg_type_warn, CFG_ZONE_PRIMARY },
	{ "check-srv-cname", &cfg_type_checkmode, CFG_ZONE_PRIMARY },
	{ "check-wildcard", &cfg_type_boolean, CFG_ZONE_PRIMARY },
	{ "dialup", &cfg_type_dialuptype,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_STUB },
	{ "dnssec-dnskey-kskonly", &cfg_type_boolean,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "dnssec-loadkeys-interval", &cfg_type_uint32,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "dnssec-policy", &cfg_type_astring,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "dnssec-secure-to-insecure", &cfg_type_boolean, CFG_ZONE_PRIMARY },
	{ "dnssec-update-mode", &cfg_type_dnssecupdatemode,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "forward", &cfg_type_forwardtype,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_STUB |
		  CFG_ZONE_STATICSTUB | CFG_ZONE_FORWARD },
	{ "forwarders", &cfg_type_portiplist,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_STUB |
		  CFG_ZONE_STATICSTUB | CFG_ZONE_FORWARD },
	{ "key-directory", &cfg_type_qstring,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "maintain-ixfr-base", &cfg_type_boolean, CFG_CLAUSEFLAG_ANCIENT },
	{ "masterfile-format", &cfg_type_masterformat,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR |
		  CFG_ZONE_STUB | CFG_ZONE_REDIRECT },
	{ "masterfile-style", &cfg_type_masterstyle,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR |
		  CFG_ZONE_STUB | CFG_ZONE_REDIRECT },
	{ "max-ixfr-log-size", &cfg_type_size, CFG_CLAUSEFLAG_ANCIENT },
	{ "max-ixfr-ratio", &cfg_type_ixfrratio,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "max-journal-size", &cfg_type_size,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "max-records", &cfg_type_uint32,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR |
		  CFG_ZONE_STUB | CFG_ZONE_STATICSTUB | CFG_ZONE_REDIRECT },
	{ "max-refresh-time", &cfg_type_uint32,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR | CFG_ZONE_STUB },
	{ "max-retry-time", &cfg_type_uint32,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR | CFG_ZONE_STUB },
	{ "max-transfer-idle-in", &cfg_type_uint32,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR | CFG_ZONE_STUB },
	{ "max-transfer-idle-out", &cfg_type_uint32,
	  CFG_ZONE_PRIMARY | CFG_ZONE_MIRROR | CFG_ZONE_SECONDARY },
	{ "max-transfer-time-in", &cfg_type_uint32,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR | CFG_ZONE_STUB },
	{ "max-transfer-time-out", &cfg_type_uint32,
	  CFG_ZONE_PRIMARY | CFG_ZONE_MIRROR | CFG_ZONE_SECONDARY },
	{ "max-zone-ttl", &cfg_type_maxduration,
	  CFG_ZONE_PRIMARY | CFG_ZONE_REDIRECT },
	{ "min-refresh-time", &cfg_type_uint32,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR | CFG_ZONE_STUB },
	{ "min-retry-time", &cfg_type_uint32,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR | CFG_ZONE_STUB },
	{ "multi-master", &cfg_type_boolean,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR | CFG_ZONE_STUB },
	{ "notify", &cfg_type_notifytype,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "notify-delay", &cfg_type_uint32,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "notify-source", &cfg_type_sockaddr4wild,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "notify-source-v6", &cfg_type_sockaddr6wild,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "notify-to-soa", &cfg_type_boolean,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "nsec3-test-zone", &cfg_type_boolean,
	  CFG_CLAUSEFLAG_TESTONLY | CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "parental-source", &cfg_type_sockaddr4wild,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "parental-source-v6", &cfg_type_sockaddr6wild,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "request-expire", &cfg_type_boolean,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "request-ixfr", &cfg_type_boolean,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "serial-update-method", &cfg_type_updatemethod, CFG_ZONE_PRIMARY },
	{ "sig-signing-nodes", &cfg_type_uint32,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "sig-signing-signatures", &cfg_type_uint32,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "sig-signing-type", &cfg_type_uint32,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "sig-validity-interval", &cfg_type_validityinterval,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "dnskey-sig-validity", &cfg_type_uint32,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "transfer-source", &cfg_type_sockaddr4wild,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR | CFG_ZONE_STUB },
	{ "transfer-source-v6", &cfg_type_sockaddr6wild,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR | CFG_ZONE_STUB },
	{ "try-tcp-refresh", &cfg_type_boolean,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "update-check-ksk", &cfg_type_boolean,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "use-alt-transfer-source", &cfg_type_boolean,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR | CFG_ZONE_STUB },
	{ "zero-no-soa-ttl", &cfg_type_boolean,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "zone-statistics", &cfg_type_zonestat,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR |
		  CFG_ZONE_STUB | CFG_ZONE_STATICSTUB | CFG_ZONE_REDIRECT },
	{ NULL, NULL, 0 }
};

/*%
 * Clauses that can be found in a 'zone' statement only.
 *
 * Note: CFG_ZONE_* options indicate in which zone types this clause is
 * legal.
 */
static cfg_clausedef_t zone_only_clauses[] = {
	/*
	 * Note that the format of the check-names option is different between
	 * the zone options and the global/view options.  Ugh.
	 */
	{ "type", &cfg_type_zonetype,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR |
		  CFG_ZONE_STUB | CFG_ZONE_STATICSTUB | CFG_ZONE_DELEGATION |
		  CFG_ZONE_HINT | CFG_ZONE_REDIRECT | CFG_ZONE_FORWARD },
	{ "check-names", &cfg_type_checkmode,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR |
		  CFG_ZONE_HINT | CFG_ZONE_STUB },
	{ "database", &cfg_type_astring,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR |
		  CFG_ZONE_STUB },
	{ "delegation-only", &cfg_type_boolean,
	  CFG_ZONE_HINT | CFG_ZONE_STUB | CFG_ZONE_FORWARD },
	{ "dlz", &cfg_type_astring,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_REDIRECT },
	{ "file", &cfg_type_qstring,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR |
		  CFG_ZONE_STUB | CFG_ZONE_HINT | CFG_ZONE_REDIRECT },
	{ "in-view", &cfg_type_astring, CFG_ZONE_INVIEW },
	{ "inline-signing", &cfg_type_boolean,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "ixfr-base", &cfg_type_qstring, CFG_CLAUSEFLAG_ANCIENT },
	{ "ixfr-from-differences", &cfg_type_boolean,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "ixfr-tmp-file", &cfg_type_qstring, CFG_CLAUSEFLAG_ANCIENT },
	{ "journal", &cfg_type_qstring,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR },
	{ "masters", &cfg_type_namesockaddrkeylist,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR | CFG_ZONE_STUB |
		  CFG_ZONE_REDIRECT },
	{ "parental-agents", &cfg_type_namesockaddrkeylist,
	  CFG_ZONE_PRIMARY | CFG_ZONE_SECONDARY },
	{ "primaries", &cfg_type_namesockaddrkeylist,
	  CFG_ZONE_SECONDARY | CFG_ZONE_MIRROR | CFG_ZONE_STUB |
		  CFG_ZONE_REDIRECT },
	{ "pubkey", &cfg_type_pubkey, CFG_CLAUSEFLAG_ANCIENT },
	{ "server-addresses", &cfg_type_bracketed_netaddrlist,
	  CFG_ZONE_STATICSTUB },
	{ "server-names", &cfg_type_namelist, CFG_ZONE_STATICSTUB },
	{ "update-policy", &cfg_type_updatepolicy, CFG_ZONE_PRIMARY },
	{ NULL, NULL, 0 }
};

/*% The top-level named.conf syntax. */

static cfg_clausedef_t *namedconf_clausesets[] = { namedconf_clauses,
						   namedconf_or_view_clauses,
						   NULL };
LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_namedconf = {
	"namedconf",	 cfg_parse_mapbody, cfg_print_mapbody,
	cfg_doc_mapbody, &cfg_rep_map,	    namedconf_clausesets
};

/*% The bind.keys syntax (trust-anchors/managed-keys/trusted-keys only). */
static cfg_clausedef_t *bindkeys_clausesets[] = { bindkeys_clauses, NULL };
LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_bindkeys = {
	"bindkeys",	 cfg_parse_mapbody, cfg_print_mapbody,
	cfg_doc_mapbody, &cfg_rep_map,	    bindkeys_clausesets
};

/*% The "options" statement syntax. */

static cfg_clausedef_t *options_clausesets[] = { options_clauses, view_clauses,
						 zone_clauses, NULL };
static cfg_type_t cfg_type_options = { "options",     cfg_parse_map,
				       cfg_print_map, cfg_doc_map,
				       &cfg_rep_map,  options_clausesets };

/*% The "view" statement syntax. */

static cfg_clausedef_t *view_clausesets[] = { view_only_clauses,
					      namedconf_or_view_clauses,
					      view_clauses, zone_clauses,
					      NULL };

static cfg_type_t cfg_type_viewopts = { "view",	       cfg_parse_map,
					cfg_print_map, cfg_doc_map,
					&cfg_rep_map,  view_clausesets };

/*% The "zone" statement syntax. */

static cfg_clausedef_t *zone_clausesets[] = { zone_only_clauses, zone_clauses,
					      NULL };
LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_zoneopts = {
	"zoneopts",  cfg_parse_map, cfg_print_map,
	cfg_doc_map, &cfg_rep_map,  zone_clausesets
};

/*% The "dnssec-policy" statement syntax. */
static cfg_clausedef_t *dnssecpolicy_clausesets[] = { dnssecpolicy_clauses,
						      NULL };
LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_dnssecpolicyopts = {
	"dnssecpolicyopts", cfg_parse_map, cfg_print_map,
	cfg_doc_map,	    &cfg_rep_map,  dnssecpolicy_clausesets
};

/*% The "dynamically loadable zones" statement syntax. */

static cfg_clausedef_t dlz_clauses[] = { { "database", &cfg_type_astring, 0 },
					 { "search", &cfg_type_boolean, 0 },
					 { NULL, NULL, 0 } };
static cfg_clausedef_t *dlz_clausesets[] = { dlz_clauses, NULL };
static cfg_type_t cfg_type_dlz = { "dlz",	  cfg_parse_named_map,
				   cfg_print_map, cfg_doc_map,
				   &cfg_rep_map,  dlz_clausesets };

/*%
 * The "dyndb" statement syntax.
 */

static cfg_tuplefielddef_t dyndb_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "library", &cfg_type_qstring, 0 },
	{ "parameters", &cfg_type_bracketed_text, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_dyndb = { "dyndb",	      cfg_parse_tuple,
				     cfg_print_tuple, cfg_doc_tuple,
				     &cfg_rep_tuple,  dyndb_fields };

/*%
 * The "plugin" statement syntax.
 * Currently only one plugin type is supported: query.
 */

static const char *plugin_enums[] = { "query", NULL };
static cfg_type_t cfg_type_plugintype = { "plugintype",	     cfg_parse_enum,
					  cfg_print_ustring, cfg_doc_enum,
					  &cfg_rep_string,   plugin_enums };
static cfg_tuplefielddef_t plugin_fields[] = {
	{ "type", &cfg_type_plugintype, 0 },
	{ "library", &cfg_type_astring, 0 },
	{ "parameters", &cfg_type_optional_bracketed_text, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_plugin = { "plugin",	       cfg_parse_tuple,
				      cfg_print_tuple, cfg_doc_tuple,
				      &cfg_rep_tuple,  plugin_fields };

/*%
 * Clauses that can be found within the 'key' statement.
 */
static cfg_clausedef_t key_clauses[] = { { "algorithm", &cfg_type_astring, 0 },
					 { "secret", &cfg_type_sstring, 0 },
					 { NULL, NULL, 0 } };

static cfg_clausedef_t *key_clausesets[] = { key_clauses, NULL };
static cfg_type_t cfg_type_key = { "key",	  cfg_parse_named_map,
				   cfg_print_map, cfg_doc_map,
				   &cfg_rep_map,  key_clausesets };

/*%
 * Clauses that can be found in a 'server' statement.
 */
static cfg_clausedef_t server_clauses[] = {
	{ "bogus", &cfg_type_boolean, 0 },
	{ "edns", &cfg_type_boolean, 0 },
	{ "edns-udp-size", &cfg_type_uint32, 0 },
	{ "edns-version", &cfg_type_uint32, 0 },
	{ "keys", &cfg_type_server_key_kludge, 0 },
	{ "max-udp-size", &cfg_type_uint32, 0 },
	{ "notify-source", &cfg_type_sockaddr4wild, 0 },
	{ "notify-source-v6", &cfg_type_sockaddr6wild, 0 },
	{ "padding", &cfg_type_uint32, 0 },
	{ "provide-ixfr", &cfg_type_boolean, 0 },
	{ "query-source", &cfg_type_querysource4, 0 },
	{ "query-source-v6", &cfg_type_querysource6, 0 },
	{ "request-expire", &cfg_type_boolean, 0 },
	{ "request-ixfr", &cfg_type_boolean, 0 },
	{ "request-nsid", &cfg_type_boolean, 0 },
	{ "request-sit", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "send-cookie", &cfg_type_boolean, 0 },
	{ "support-ixfr", &cfg_type_boolean, CFG_CLAUSEFLAG_OBSOLETE },
	{ "tcp-keepalive", &cfg_type_boolean, 0 },
	{ "tcp-only", &cfg_type_boolean, 0 },
	{ "transfer-format", &cfg_type_transferformat, 0 },
	{ "transfer-source", &cfg_type_sockaddr4wild, 0 },
	{ "transfer-source-v6", &cfg_type_sockaddr6wild, 0 },
	{ "transfers", &cfg_type_uint32, 0 },
	{ NULL, NULL, 0 }
};
static cfg_clausedef_t *server_clausesets[] = { server_clauses, NULL };
static cfg_type_t cfg_type_server = { "server",	     cfg_parse_netprefix_map,
				      cfg_print_map, cfg_doc_map,
				      &cfg_rep_map,  server_clausesets };

/*%
 * Clauses that can be found in a 'channel' clause in the
 * 'logging' statement.
 *
 * These have some additional constraints that need to be
 * checked after parsing:
 *  - There must exactly one of file/syslog/null/stderr
 */

static const char *printtime_enums[] = { "iso8601", "iso8601-utc", "local",
					 NULL };
static isc_result_t
parse_printtime(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (cfg_parse_enum_or_other(pctx, type, &cfg_type_boolean, ret));
}
static void
doc_printtime(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_doc_enum_or_other(pctx, type, &cfg_type_boolean);
}
static cfg_type_t cfg_type_printtime = { "printtime",	    parse_printtime,
					 cfg_print_ustring, doc_printtime,
					 &cfg_rep_string,   printtime_enums };

static cfg_clausedef_t channel_clauses[] = {
	/* Destinations.  We no longer require these to be first. */
	{ "file", &cfg_type_logfile, 0 },
	{ "syslog", &cfg_type_optional_facility, 0 },
	{ "null", &cfg_type_void, 0 },
	{ "stderr", &cfg_type_void, 0 },
	/* Options.  We now accept these for the null channel, too. */
	{ "severity", &cfg_type_logseverity, 0 },
	{ "print-time", &cfg_type_printtime, 0 },
	{ "print-severity", &cfg_type_boolean, 0 },
	{ "print-category", &cfg_type_boolean, 0 },
	{ "buffered", &cfg_type_boolean, 0 },
	{ NULL, NULL, 0 }
};
static cfg_clausedef_t *channel_clausesets[] = { channel_clauses, NULL };
static cfg_type_t cfg_type_channel = { "channel",     cfg_parse_named_map,
				       cfg_print_map, cfg_doc_map,
				       &cfg_rep_map,  channel_clausesets };

/*% A list of log destination, used in the "category" clause. */
static cfg_type_t cfg_type_destinationlist = { "destinationlist",
					       cfg_parse_bracketed_list,
					       cfg_print_bracketed_list,
					       cfg_doc_bracketed_list,
					       &cfg_rep_list,
					       &cfg_type_astring };

/*%
 * Clauses that can be found in a 'logging' statement.
 */
static cfg_clausedef_t logging_clauses[] = {
	{ "channel", &cfg_type_channel, CFG_CLAUSEFLAG_MULTI },
	{ "category", &cfg_type_category, CFG_CLAUSEFLAG_MULTI },
	{ NULL, NULL, 0 }
};
static cfg_clausedef_t *logging_clausesets[] = { logging_clauses, NULL };
static cfg_type_t cfg_type_logging = { "logging",     cfg_parse_map,
				       cfg_print_map, cfg_doc_map,
				       &cfg_rep_map,  logging_clausesets };

/*%
 * For parsing an 'addzone' statement
 */
static cfg_tuplefielddef_t addzone_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "class", &cfg_type_optional_class, 0 },
	{ "view", &cfg_type_optional_class, 0 },
	{ "options", &cfg_type_zoneopts, 0 },
	{ NULL, NULL, 0 }
};
static cfg_type_t cfg_type_addzone = { "zone",		cfg_parse_tuple,
				       cfg_print_tuple, cfg_doc_tuple,
				       &cfg_rep_tuple,	addzone_fields };

static cfg_clausedef_t addzoneconf_clauses[] = {
	{ "zone", &cfg_type_addzone, CFG_CLAUSEFLAG_MULTI }, { NULL, NULL, 0 }
};

static cfg_clausedef_t *addzoneconf_clausesets[] = { addzoneconf_clauses,
						     NULL };

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_addzoneconf = {
	"addzoneconf",	 cfg_parse_mapbody, cfg_print_mapbody,
	cfg_doc_mapbody, &cfg_rep_map,	    addzoneconf_clausesets
};

static isc_result_t
parse_unitstring(char *str, isc_resourcevalue_t *valuep) {
	char *endp;
	unsigned int len;
	uint64_t value;
	uint64_t unit;

	value = strtoull(str, &endp, 10);
	if (*endp == 0) {
		*valuep = value;
		return (ISC_R_SUCCESS);
	}

	len = strlen(str);
	if (len < 2 || endp[1] != '\0') {
		return (ISC_R_FAILURE);
	}

	switch (str[len - 1]) {
	case 'k':
	case 'K':
		unit = 1024;
		break;
	case 'm':
	case 'M':
		unit = 1024 * 1024;
		break;
	case 'g':
	case 'G':
		unit = 1024 * 1024 * 1024;
		break;
	default:
		return (ISC_R_FAILURE);
	}
	if (value > ((uint64_t)UINT64_MAX / unit)) {
		return (ISC_R_FAILURE);
	}
	*valuep = value * unit;
	return (ISC_R_SUCCESS);
}

static isc_result_t
parse_sizeval(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	uint64_t val;

	UNUSED(type);

	CHECK(cfg_gettoken(pctx, 0));
	if (pctx->token.type != isc_tokentype_string) {
		result = ISC_R_UNEXPECTEDTOKEN;
		goto cleanup;
	}
	CHECK(parse_unitstring(TOKEN_STRING(pctx), &val));

	CHECK(cfg_create_obj(pctx, &cfg_type_uint64, &obj));
	obj->value.uint64 = val;
	*ret = obj;
	return (ISC_R_SUCCESS);

cleanup:
	cfg_parser_error(pctx, CFG_LOG_NEAR,
			 "expected integer and optional unit");
	return (result);
}

static isc_result_t
parse_sizeval_percent(cfg_parser_t *pctx, const cfg_type_t *type,
		      cfg_obj_t **ret) {
	char *endp;
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	uint64_t val;
	uint64_t percent;

	UNUSED(type);

	CHECK(cfg_gettoken(pctx, 0));
	if (pctx->token.type != isc_tokentype_string) {
		result = ISC_R_UNEXPECTEDTOKEN;
		goto cleanup;
	}

	percent = strtoull(TOKEN_STRING(pctx), &endp, 10);

	if (*endp == '%' && *(endp + 1) == 0) {
		CHECK(cfg_create_obj(pctx, &cfg_type_percentage, &obj));
		obj->value.uint32 = (uint32_t)percent;
		*ret = obj;
		return (ISC_R_SUCCESS);
	} else {
		CHECK(parse_unitstring(TOKEN_STRING(pctx), &val));
		CHECK(cfg_create_obj(pctx, &cfg_type_uint64, &obj));
		obj->value.uint64 = val;
		*ret = obj;
		return (ISC_R_SUCCESS);
	}

cleanup:
	cfg_parser_error(pctx, CFG_LOG_NEAR,
			 "expected integer and optional unit or percent");
	return (result);
}

static void
doc_sizeval_percent(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);

	cfg_print_cstr(pctx, "( ");
	cfg_doc_terminal(pctx, &cfg_type_size);
	cfg_print_cstr(pctx, " | ");
	cfg_doc_terminal(pctx, &cfg_type_percentage);
	cfg_print_cstr(pctx, " )");
}

/*%
 * A size value (number + optional unit).
 */
static cfg_type_t cfg_type_sizeval = { "sizeval",	 parse_sizeval,
				       cfg_print_uint64, cfg_doc_terminal,
				       &cfg_rep_uint64,	 NULL };

/*%
 * A size, "unlimited", or "default".
 */

static isc_result_t
parse_size(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (cfg_parse_enum_or_other(pctx, type, &cfg_type_sizeval, ret));
}

static void
doc_size(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_doc_enum_or_other(pctx, type, &cfg_type_sizeval);
}

static const char *size_enums[] = { "default", "unlimited", NULL };
static cfg_type_t cfg_type_size = {
	"size",	  parse_size,	   cfg_print_ustring,
	doc_size, &cfg_rep_string, size_enums
};

/*%
 * A size or "unlimited", but not "default".
 */
static const char *sizenodefault_enums[] = { "unlimited", NULL };
static cfg_type_t cfg_type_sizenodefault = {
	"size_no_default", parse_size,	    cfg_print_ustring,
	doc_size,	   &cfg_rep_string, sizenodefault_enums
};

/*%
 * A size in absolute values or percents.
 */
static cfg_type_t cfg_type_sizeval_percent = {
	"sizeval_percent",   parse_sizeval_percent, cfg_print_ustring,
	doc_sizeval_percent, &cfg_rep_string,	    NULL
};

/*%
 * A size in absolute values or percents, or "unlimited", or "default"
 */

static isc_result_t
parse_size_or_percent(cfg_parser_t *pctx, const cfg_type_t *type,
		      cfg_obj_t **ret) {
	return (cfg_parse_enum_or_other(pctx, type, &cfg_type_sizeval_percent,
					ret));
}

static void
doc_parse_size_or_percent(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "( default | unlimited | ");
	cfg_doc_terminal(pctx, &cfg_type_sizeval);
	cfg_print_cstr(pctx, " | ");
	cfg_doc_terminal(pctx, &cfg_type_percentage);
	cfg_print_cstr(pctx, " )");
}

static const char *sizeorpercent_enums[] = { "default", "unlimited", NULL };
static cfg_type_t cfg_type_sizeorpercent = {
	"size_or_percent",	   parse_size_or_percent, cfg_print_ustring,
	doc_parse_size_or_percent, &cfg_rep_string,	  sizeorpercent_enums
};

/*%
 * An IXFR size ratio: percentage, or "unlimited".
 */

static isc_result_t
parse_ixfrratio(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (cfg_parse_enum_or_other(pctx, type, &cfg_type_percentage, ret));
}

static void
doc_ixfrratio(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "( unlimited | ");
	cfg_doc_terminal(pctx, &cfg_type_percentage);
	cfg_print_cstr(pctx, " )");
}

static const char *ixfrratio_enums[] = { "unlimited", NULL };
static cfg_type_t cfg_type_ixfrratio = { "ixfr_ratio", parse_ixfrratio,
					 NULL,	       doc_ixfrratio,
					 NULL,	       ixfrratio_enums };

/*%
 * optional_keyvalue
 */
static isc_result_t
parse_maybe_optional_keyvalue(cfg_parser_t *pctx, const cfg_type_t *type,
			      bool optional, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	const keyword_type_t *kw = type->of;

	CHECK(cfg_peektoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_string &&
	    strcasecmp(TOKEN_STRING(pctx), kw->name) == 0)
	{
		CHECK(cfg_gettoken(pctx, 0));
		CHECK(kw->type->parse(pctx, kw->type, &obj));
		obj->type = type; /* XXX kludge */
	} else {
		if (optional) {
			CHECK(cfg_parse_void(pctx, NULL, &obj));
		} else {
			cfg_parser_error(pctx, CFG_LOG_NEAR, "expected '%s'",
					 kw->name);
			result = ISC_R_UNEXPECTEDTOKEN;
			goto cleanup;
		}
	}
	*ret = obj;
cleanup:
	return (result);
}

static isc_result_t
parse_keyvalue(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (parse_maybe_optional_keyvalue(pctx, type, false, ret));
}

static isc_result_t
parse_optional_keyvalue(cfg_parser_t *pctx, const cfg_type_t *type,
			cfg_obj_t **ret) {
	return (parse_maybe_optional_keyvalue(pctx, type, true, ret));
}

static void
print_keyvalue(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	const keyword_type_t *kw = obj->type->of;
	cfg_print_cstr(pctx, kw->name);
	cfg_print_cstr(pctx, " ");
	kw->type->print(pctx, obj);
}

static void
doc_keyvalue(cfg_printer_t *pctx, const cfg_type_t *type) {
	const keyword_type_t *kw = type->of;
	cfg_print_cstr(pctx, kw->name);
	cfg_print_cstr(pctx, " ");
	cfg_doc_obj(pctx, kw->type);
}

static void
doc_optional_keyvalue(cfg_printer_t *pctx, const cfg_type_t *type) {
	const keyword_type_t *kw = type->of;
	cfg_print_cstr(pctx, "[ ");
	cfg_print_cstr(pctx, kw->name);
	cfg_print_cstr(pctx, " ");
	cfg_doc_obj(pctx, kw->type);
	cfg_print_cstr(pctx, " ]");
}

static const char *dialup_enums[] = { "notify", "notify-passive", "passive",
				      "refresh", NULL };
static isc_result_t
parse_dialup_type(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (cfg_parse_enum_or_other(pctx, type, &cfg_type_boolean, ret));
}
static void
doc_dialup_type(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_doc_enum_or_other(pctx, type, &cfg_type_boolean);
}
static cfg_type_t cfg_type_dialuptype = { "dialuptype",	     parse_dialup_type,
					  cfg_print_ustring, doc_dialup_type,
					  &cfg_rep_string,   dialup_enums };

static const char *notify_enums[] = { "explicit", "master-only", "primary-only",
				      NULL };
static isc_result_t
parse_notify_type(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (cfg_parse_enum_or_other(pctx, type, &cfg_type_boolean, ret));
}
static void
doc_notify_type(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_doc_enum_or_other(pctx, type, &cfg_type_boolean);
}
static cfg_type_t cfg_type_notifytype = {
	"notifytype",	 parse_notify_type, cfg_print_ustring,
	doc_notify_type, &cfg_rep_string,   notify_enums,
};

static const char *minimal_enums[] = { "no-auth", "no-auth-recursive", NULL };
static isc_result_t
parse_minimal(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (cfg_parse_enum_or_other(pctx, type, &cfg_type_boolean, ret));
}
static void
doc_minimal(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_doc_enum_or_other(pctx, type, &cfg_type_boolean);
}
static cfg_type_t cfg_type_minimal = {
	"minimal",   parse_minimal,   cfg_print_ustring,
	doc_minimal, &cfg_rep_string, minimal_enums,
};

static const char *ixfrdiff_enums[] = { "primary", "master", "secondary",
					"slave", NULL };
static isc_result_t
parse_ixfrdiff_type(cfg_parser_t *pctx, const cfg_type_t *type,
		    cfg_obj_t **ret) {
	return (cfg_parse_enum_or_other(pctx, type, &cfg_type_boolean, ret));
}
static void
doc_ixfrdiff_type(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_doc_enum_or_other(pctx, type, &cfg_type_boolean);
}
static cfg_type_t cfg_type_ixfrdifftype = {
	"ixfrdiff",	   parse_ixfrdiff_type, cfg_print_ustring,
	doc_ixfrdiff_type, &cfg_rep_string,	ixfrdiff_enums,
};

static keyword_type_t key_kw = { "key", &cfg_type_astring };

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_keyref = {
	"keyref",     parse_keyvalue,  print_keyvalue,
	doc_keyvalue, &cfg_rep_string, &key_kw
};

static cfg_type_t cfg_type_optional_keyref = {
	"optional_keyref",     parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_string,		&key_kw
};

static const char *qminmethod_enums[] = { "strict", "relaxed", "disabled",
					  "off", NULL };

static cfg_type_t cfg_type_qminmethod = { "qminmethod",	     cfg_parse_enum,
					  cfg_print_ustring, cfg_doc_enum,
					  &cfg_rep_string,   qminmethod_enums };

/*%
 * A "controls" statement is represented as a map with the multivalued
 * "inet" and "unix" clauses.
 */

static keyword_type_t controls_allow_kw = { "allow", &cfg_type_bracketed_aml };

static cfg_type_t cfg_type_controls_allow = {
	"controls_allow", parse_keyvalue, print_keyvalue,
	doc_keyvalue,	  &cfg_rep_list,  &controls_allow_kw
};

static keyword_type_t controls_keys_kw = { "keys", &cfg_type_keylist };

static cfg_type_t cfg_type_controls_keys = {
	"controls_keys",       parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_list,		&controls_keys_kw
};

static keyword_type_t controls_readonly_kw = { "read-only", &cfg_type_boolean };

static cfg_type_t cfg_type_controls_readonly = {
	"controls_readonly",   parse_optional_keyvalue, print_keyvalue,
	doc_optional_keyvalue, &cfg_rep_boolean,	&controls_readonly_kw
};

static cfg_tuplefielddef_t inetcontrol_fields[] = {
	{ "address", &cfg_type_controls_sockaddr, 0 },
	{ "allow", &cfg_type_controls_allow, 0 },
	{ "keys", &cfg_type_controls_keys, 0 },
	{ "read-only", &cfg_type_controls_readonly, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_inetcontrol = {
	"inetcontrol", cfg_parse_tuple, cfg_print_tuple,
	cfg_doc_tuple, &cfg_rep_tuple,	inetcontrol_fields
};

static keyword_type_t controls_perm_kw = { "perm", &cfg_type_uint32 };

static cfg_type_t cfg_type_controls_perm = {
	"controls_perm", parse_keyvalue,  print_keyvalue,
	doc_keyvalue,	 &cfg_rep_uint32, &controls_perm_kw
};

static keyword_type_t controls_owner_kw = { "owner", &cfg_type_uint32 };

static cfg_type_t cfg_type_controls_owner = {
	"controls_owner", parse_keyvalue,  print_keyvalue,
	doc_keyvalue,	  &cfg_rep_uint32, &controls_owner_kw
};

static keyword_type_t controls_group_kw = { "group", &cfg_type_uint32 };

static cfg_type_t cfg_type_controls_group = {
	"controls_allow", parse_keyvalue,  print_keyvalue,
	doc_keyvalue,	  &cfg_rep_uint32, &controls_group_kw
};

static cfg_tuplefielddef_t unixcontrol_fields[] = {
	{ "path", &cfg_type_qstring, 0 },
	{ "perm", &cfg_type_controls_perm, 0 },
	{ "owner", &cfg_type_controls_owner, 0 },
	{ "group", &cfg_type_controls_group, 0 },
	{ "keys", &cfg_type_controls_keys, 0 },
	{ "read-only", &cfg_type_controls_readonly, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_unixcontrol = {
	"unixcontrol", cfg_parse_tuple, cfg_print_tuple,
	cfg_doc_tuple, &cfg_rep_tuple,	unixcontrol_fields
};

static cfg_clausedef_t controls_clauses[] = {
	{ "inet", &cfg_type_inetcontrol, CFG_CLAUSEFLAG_MULTI },
	{ "unix", &cfg_type_unixcontrol, CFG_CLAUSEFLAG_MULTI },
	{ NULL, NULL, 0 }
};

static cfg_clausedef_t *controls_clausesets[] = { controls_clauses, NULL };
static cfg_type_t cfg_type_controls = { "controls",    cfg_parse_map,
					cfg_print_map, cfg_doc_map,
					&cfg_rep_map,  &controls_clausesets };

/*%
 * A "statistics-channels" statement is represented as a map with the
 * multivalued "inet" clauses.
 */
static void
doc_optional_bracketed_list(cfg_printer_t *pctx, const cfg_type_t *type) {
	const keyword_type_t *kw = type->of;
	cfg_print_cstr(pctx, "[ ");
	cfg_print_cstr(pctx, kw->name);
	cfg_print_cstr(pctx, " ");
	cfg_doc_obj(pctx, kw->type);
	cfg_print_cstr(pctx, " ]");
}

static cfg_type_t cfg_type_optional_allow = {
	"optional_allow", parse_optional_keyvalue,
	print_keyvalue,	  doc_optional_bracketed_list,
	&cfg_rep_list,	  &controls_allow_kw
};

static cfg_tuplefielddef_t statserver_fields[] = {
	{ "address", &cfg_type_controls_sockaddr, 0 }, /* reuse controls def */
	{ "allow", &cfg_type_optional_allow, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_statschannel = {
	"statschannel", cfg_parse_tuple, cfg_print_tuple,
	cfg_doc_tuple,	&cfg_rep_tuple,	 statserver_fields
};

static cfg_clausedef_t statservers_clauses[] = {
	{ "inet", &cfg_type_statschannel, CFG_CLAUSEFLAG_MULTI },
	{ NULL, NULL, 0 }
};

static cfg_clausedef_t *statservers_clausesets[] = { statservers_clauses,
						     NULL };

static cfg_type_t cfg_type_statschannels = {
	"statistics-channels", cfg_parse_map, cfg_print_map,
	cfg_doc_map,	       &cfg_rep_map,  &statservers_clausesets
};

/*%
 * An optional class, as used in view and zone statements.
 */
static isc_result_t
parse_optional_class(cfg_parser_t *pctx, const cfg_type_t *type,
		     cfg_obj_t **ret) {
	isc_result_t result;
	UNUSED(type);
	CHECK(cfg_peektoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_string) {
		CHECK(cfg_parse_obj(pctx, &cfg_type_ustring, ret));
	} else {
		CHECK(cfg_parse_obj(pctx, &cfg_type_void, ret));
	}
cleanup:
	return (result);
}

static void
doc_optional_class(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "[ <class> ]");
}

static cfg_type_t cfg_type_optional_class = { "optional_class",
					      parse_optional_class,
					      NULL,
					      doc_optional_class,
					      NULL,
					      NULL };

static isc_result_t
parse_querysource(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	isc_netaddr_t netaddr;
	in_port_t port = 0;
	isc_dscp_t dscp = -1;
	unsigned int have_address = 0;
	unsigned int have_port = 0;
	unsigned int have_dscp = 0;
	const unsigned int *flagp = type->of;

	if ((*flagp & CFG_ADDR_V4OK) != 0) {
		isc_netaddr_any(&netaddr);
	} else if ((*flagp & CFG_ADDR_V6OK) != 0) {
		isc_netaddr_any6(&netaddr);
	} else {
		UNREACHABLE();
	}

	for (;;) {
		CHECK(cfg_peektoken(pctx, 0));
		if (pctx->token.type == isc_tokentype_string) {
			if (strcasecmp(TOKEN_STRING(pctx), "address") == 0) {
				/* read "address" */
				CHECK(cfg_gettoken(pctx, 0));
				CHECK(cfg_parse_rawaddr(pctx, *flagp,
							&netaddr));
				have_address++;
			} else if (strcasecmp(TOKEN_STRING(pctx), "port") == 0)
			{
				/* read "port" */
				CHECK(cfg_gettoken(pctx, 0));
				CHECK(cfg_parse_rawport(pctx, CFG_ADDR_WILDOK,
							&port));
				have_port++;
			} else if (strcasecmp(TOKEN_STRING(pctx), "dscp") == 0)
			{
				if ((pctx->flags & CFG_PCTX_NODEPRECATED) == 0)
				{
					cfg_parser_warning(
						pctx, 0,
						"token 'dscp' is deprecated");
				}
				/* read "dscp" */
				CHECK(cfg_gettoken(pctx, 0));
				CHECK(cfg_parse_dscp(pctx, &dscp));
				have_dscp++;
			} else if (have_port == 0 && have_dscp == 0 &&
				   have_address == 0)
			{
				return (cfg_parse_sockaddr(pctx, type, ret));
			} else {
				cfg_parser_error(pctx, CFG_LOG_NEAR,
						 "expected 'address', 'port', "
						 "or 'dscp'");
				return (ISC_R_UNEXPECTEDTOKEN);
			}
		} else {
			break;
		}
	}
	if (have_address > 1 || have_port > 1 || have_address + have_port == 0)
	{
		cfg_parser_error(pctx, 0, "expected one address and/or port");
		return (ISC_R_UNEXPECTEDTOKEN);
	}

	if (have_dscp > 1) {
		cfg_parser_error(pctx, 0, "expected at most one dscp");
		return (ISC_R_UNEXPECTEDTOKEN);
	}

	CHECK(cfg_create_obj(pctx, &cfg_type_querysource, &obj));
	isc_sockaddr_fromnetaddr(&obj->value.sockaddr, &netaddr, port);
	obj->value.sockaddrdscp.dscp = dscp;
	*ret = obj;
	return (ISC_R_SUCCESS);

cleanup:
	cfg_parser_error(pctx, CFG_LOG_NEAR, "invalid query source");
	CLEANUP_OBJ(obj);
	return (result);
}

static void
print_querysource(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	isc_netaddr_t na;
	isc_netaddr_fromsockaddr(&na, &obj->value.sockaddr);
	cfg_print_cstr(pctx, "address ");
	cfg_print_rawaddr(pctx, &na);
	cfg_print_cstr(pctx, " port ");
	cfg_print_rawuint(pctx, isc_sockaddr_getport(&obj->value.sockaddr));
	if (obj->value.sockaddrdscp.dscp != -1) {
		cfg_print_cstr(pctx, " dscp ");
		cfg_print_rawuint(pctx, obj->value.sockaddrdscp.dscp);
	}
}

static void
doc_querysource(cfg_printer_t *pctx, const cfg_type_t *type) {
	const unsigned int *flagp = type->of;

	cfg_print_cstr(pctx, "( ( [ address ] ( ");
	if ((*flagp & CFG_ADDR_V4OK) != 0) {
		cfg_print_cstr(pctx, "<ipv4_address>");
	} else if ((*flagp & CFG_ADDR_V6OK) != 0) {
		cfg_print_cstr(pctx, "<ipv6_address>");
	} else {
		UNREACHABLE();
	}
	cfg_print_cstr(pctx, " | * ) [ port ( <integer> | * ) ] ) | "
			     "( [ [ address ] ( ");
	if ((*flagp & CFG_ADDR_V4OK) != 0) {
		cfg_print_cstr(pctx, "<ipv4_address>");
	} else if ((*flagp & CFG_ADDR_V6OK) != 0) {
		cfg_print_cstr(pctx, "<ipv6_address>");
	} else {
		UNREACHABLE();
	}
	cfg_print_cstr(pctx, " | * ) ] port ( <integer> | * ) ) )"
			     " [ dscp <integer> ]");
}

static unsigned int sockaddr4wild_flags = CFG_ADDR_WILDOK | CFG_ADDR_V4OK |
					  CFG_ADDR_DSCPOK;
static unsigned int sockaddr6wild_flags = CFG_ADDR_WILDOK | CFG_ADDR_V6OK |
					  CFG_ADDR_DSCPOK;

static cfg_type_t cfg_type_querysource4 = {
	"querysource4", parse_querysource,   NULL, doc_querysource,
	NULL,		&sockaddr4wild_flags
};

static cfg_type_t cfg_type_querysource6 = {
	"querysource6", parse_querysource,   NULL, doc_querysource,
	NULL,		&sockaddr6wild_flags
};

static cfg_type_t cfg_type_querysource = { "querysource",     NULL,
					   print_querysource, NULL,
					   &cfg_rep_sockaddr, NULL };

/*%
 * The socket address syntax in the "controls" statement is silly.
 * It allows both socket address families, but also allows "*",
 * which is gratuitously interpreted as the IPv4 wildcard address.
 */
static unsigned int controls_sockaddr_flags = CFG_ADDR_V4OK | CFG_ADDR_V6OK |
					      CFG_ADDR_WILDOK;
static cfg_type_t cfg_type_controls_sockaddr = {
	"controls_sockaddr", cfg_parse_sockaddr, cfg_print_sockaddr,
	cfg_doc_sockaddr,    &cfg_rep_sockaddr,	 &controls_sockaddr_flags
};

/*%
 * Handle the special kludge syntax of the "keys" clause in the "server"
 * statement, which takes a single key with or without braces and semicolon.
 */
static isc_result_t
parse_server_key_kludge(cfg_parser_t *pctx, const cfg_type_t *type,
			cfg_obj_t **ret) {
	isc_result_t result;
	bool braces = false;
	UNUSED(type);

	/* Allow opening brace. */
	CHECK(cfg_peektoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_special &&
	    pctx->token.value.as_char == '{')
	{
		CHECK(cfg_gettoken(pctx, 0));
		braces = true;
	}

	CHECK(cfg_parse_obj(pctx, &cfg_type_astring, ret));

	if (braces) {
		/* Skip semicolon if present. */
		CHECK(cfg_peektoken(pctx, 0));
		if (pctx->token.type == isc_tokentype_special &&
		    pctx->token.value.as_char == ';')
		{
			CHECK(cfg_gettoken(pctx, 0));
		}

		CHECK(cfg_parse_special(pctx, '}'));
	}
cleanup:
	return (result);
}
static cfg_type_t cfg_type_server_key_kludge = {
	"server_key", parse_server_key_kludge, NULL, cfg_doc_terminal, NULL,
	NULL
};

/*%
 * An optional logging facility.
 */

static isc_result_t
parse_optional_facility(cfg_parser_t *pctx, const cfg_type_t *type,
			cfg_obj_t **ret) {
	isc_result_t result;
	UNUSED(type);

	CHECK(cfg_peektoken(pctx, CFG_LEXOPT_QSTRING));
	if (pctx->token.type == isc_tokentype_string ||
	    pctx->token.type == isc_tokentype_qstring)
	{
		CHECK(cfg_parse_obj(pctx, &cfg_type_astring, ret));
	} else {
		CHECK(cfg_parse_obj(pctx, &cfg_type_void, ret));
	}
cleanup:
	return (result);
}

static void
doc_optional_facility(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "[ <syslog_facility> ]");
}

static cfg_type_t cfg_type_optional_facility = { "optional_facility",
						 parse_optional_facility,
						 NULL,
						 doc_optional_facility,
						 NULL,
						 NULL };

/*%
 * A log severity.  Return as a string, except "debug N",
 * which is returned as a keyword object.
 */

static keyword_type_t debug_kw = { "debug", &cfg_type_uint32 };
static cfg_type_t cfg_type_debuglevel = { "debuglevel",	   parse_keyvalue,
					  print_keyvalue,  doc_keyvalue,
					  &cfg_rep_uint32, &debug_kw };

static isc_result_t
parse_logseverity(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	UNUSED(type);

	CHECK(cfg_peektoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_string &&
	    strcasecmp(TOKEN_STRING(pctx), "debug") == 0)
	{
		CHECK(cfg_gettoken(pctx, 0)); /* read "debug" */
		CHECK(cfg_peektoken(pctx, ISC_LEXOPT_NUMBER));
		if (pctx->token.type == isc_tokentype_number) {
			CHECK(cfg_parse_uint32(pctx, NULL, ret));
		} else {
			/*
			 * The debug level is optional and defaults to 1.
			 * This makes little sense, but we support it for
			 * compatibility with BIND 8.
			 */
			CHECK(cfg_create_obj(pctx, &cfg_type_uint32, ret));
			(*ret)->value.uint32 = 1;
		}
		(*ret)->type = &cfg_type_debuglevel; /* XXX kludge */
	} else {
		CHECK(cfg_parse_obj(pctx, &cfg_type_loglevel, ret));
	}
cleanup:
	return (result);
}

static cfg_type_t cfg_type_logseverity = { "log_severity", parse_logseverity,
					   NULL,	   cfg_doc_terminal,
					   NULL,	   NULL };

/*%
 * The "file" clause of the "channel" statement.
 * This is yet another special case.
 */

static const char *logversions_enums[] = { "unlimited", NULL };
static isc_result_t
parse_logversions(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	return (cfg_parse_enum_or_other(pctx, type, &cfg_type_uint32, ret));
}

static void
doc_logversions(cfg_printer_t *pctx, const cfg_type_t *type) {
	cfg_doc_enum_or_other(pctx, type, &cfg_type_uint32);
}

static cfg_type_t cfg_type_logversions = {
	"logversions",	 parse_logversions, cfg_print_ustring,
	doc_logversions, &cfg_rep_string,   logversions_enums
};

static const char *logsuffix_enums[] = { "increment", "timestamp", NULL };
static cfg_type_t cfg_type_logsuffix = { "logsuffix",	    cfg_parse_enum,
					 cfg_print_ustring, cfg_doc_enum,
					 &cfg_rep_string,   &logsuffix_enums };

static cfg_tuplefielddef_t logfile_fields[] = {
	{ "file", &cfg_type_qstring, 0 },
	{ "versions", &cfg_type_logversions, 0 },
	{ "size", &cfg_type_size, 0 },
	{ "suffix", &cfg_type_logsuffix, 0 },
	{ NULL, NULL, 0 }
};

static isc_result_t
parse_logfile(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	const cfg_tuplefielddef_t *fields = type->of;

	CHECK(cfg_create_tuple(pctx, type, &obj));

	/* Parse the mandatory "file" field */
	CHECK(cfg_parse_obj(pctx, fields[0].type, &obj->value.tuple[0]));

	/* Parse "versions" and "size" fields in any order. */
	for (;;) {
		CHECK(cfg_peektoken(pctx, 0));
		if (pctx->token.type == isc_tokentype_string) {
			CHECK(cfg_gettoken(pctx, 0));
			if (strcasecmp(TOKEN_STRING(pctx), "versions") == 0 &&
			    obj->value.tuple[1] == NULL)
			{
				CHECK(cfg_parse_obj(pctx, fields[1].type,
						    &obj->value.tuple[1]));
			} else if (strcasecmp(TOKEN_STRING(pctx), "size") ==
					   0 &&
				   obj->value.tuple[2] == NULL)
			{
				CHECK(cfg_parse_obj(pctx, fields[2].type,
						    &obj->value.tuple[2]));
			} else if (strcasecmp(TOKEN_STRING(pctx), "suffix") ==
					   0 &&
				   obj->value.tuple[3] == NULL)
			{
				CHECK(cfg_parse_obj(pctx, fields[3].type,
						    &obj->value.tuple[3]));
			} else {
				break;
			}
		} else {
			break;
		}
	}

	/* Create void objects for missing optional values. */
	if (obj->value.tuple[1] == NULL) {
		CHECK(cfg_parse_void(pctx, NULL, &obj->value.tuple[1]));
	}
	if (obj->value.tuple[2] == NULL) {
		CHECK(cfg_parse_void(pctx, NULL, &obj->value.tuple[2]));
	}
	if (obj->value.tuple[3] == NULL) {
		CHECK(cfg_parse_void(pctx, NULL, &obj->value.tuple[3]));
	}

	*ret = obj;
	return (ISC_R_SUCCESS);

cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

static void
print_logfile(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	cfg_print_obj(pctx, obj->value.tuple[0]); /* file */
	if (obj->value.tuple[1]->type->print != cfg_print_void) {
		cfg_print_cstr(pctx, " versions ");
		cfg_print_obj(pctx, obj->value.tuple[1]);
	}
	if (obj->value.tuple[2]->type->print != cfg_print_void) {
		cfg_print_cstr(pctx, " size ");
		cfg_print_obj(pctx, obj->value.tuple[2]);
	}
	if (obj->value.tuple[3]->type->print != cfg_print_void) {
		cfg_print_cstr(pctx, " suffix ");
		cfg_print_obj(pctx, obj->value.tuple[3]);
	}
}

static void
doc_logfile(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "<quoted_string>");
	cfg_print_cstr(pctx, " ");
	cfg_print_cstr(pctx, "[ versions ( unlimited | <integer> ) ]");
	cfg_print_cstr(pctx, " ");
	cfg_print_cstr(pctx, "[ size <size> ]");
	cfg_print_cstr(pctx, " ");
	cfg_print_cstr(pctx, "[ suffix ( increment | timestamp ) ]");
}

static cfg_type_t cfg_type_logfile = { "log_file",     parse_logfile,
				       print_logfile,  doc_logfile,
				       &cfg_rep_tuple, logfile_fields };

/*% An IPv4 address with optional dscp and port, "*" accepted as wildcard. */
static cfg_type_t cfg_type_sockaddr4wild = {
	"sockaddr4wild",  cfg_parse_sockaddr, cfg_print_sockaddr,
	cfg_doc_sockaddr, &cfg_rep_sockaddr,  &sockaddr4wild_flags
};

/*% An IPv6 address with optional port, "*" accepted as wildcard. */
static cfg_type_t cfg_type_sockaddr6wild = {
	"v6addrportwild", cfg_parse_sockaddr, cfg_print_sockaddr,
	cfg_doc_sockaddr, &cfg_rep_sockaddr,  &sockaddr6wild_flags
};

/*%
 * rndc
 */

static cfg_clausedef_t rndcconf_options_clauses[] = {
	{ "default-key", &cfg_type_astring, 0 },
	{ "default-port", &cfg_type_uint32, 0 },
	{ "default-server", &cfg_type_astring, 0 },
	{ "default-source-address", &cfg_type_netaddr4wild, 0 },
	{ "default-source-address-v6", &cfg_type_netaddr6wild, 0 },
	{ NULL, NULL, 0 }
};

static cfg_clausedef_t *rndcconf_options_clausesets[] = {
	rndcconf_options_clauses, NULL
};

static cfg_type_t cfg_type_rndcconf_options = {
	"rndcconf_options", cfg_parse_map, cfg_print_map,
	cfg_doc_map,	    &cfg_rep_map,  rndcconf_options_clausesets
};

static cfg_clausedef_t rndcconf_server_clauses[] = {
	{ "key", &cfg_type_astring, 0 },
	{ "port", &cfg_type_uint32, 0 },
	{ "source-address", &cfg_type_netaddr4wild, 0 },
	{ "source-address-v6", &cfg_type_netaddr6wild, 0 },
	{ "addresses", &cfg_type_bracketed_sockaddrnameportlist, 0 },
	{ NULL, NULL, 0 }
};

static cfg_clausedef_t *rndcconf_server_clausesets[] = {
	rndcconf_server_clauses, NULL
};

static cfg_type_t cfg_type_rndcconf_server = {
	"rndcconf_server", cfg_parse_named_map, cfg_print_map,
	cfg_doc_map,	   &cfg_rep_map,	rndcconf_server_clausesets
};

static cfg_clausedef_t rndcconf_clauses[] = {
	{ "key", &cfg_type_key, CFG_CLAUSEFLAG_MULTI },
	{ "server", &cfg_type_rndcconf_server, CFG_CLAUSEFLAG_MULTI },
	{ "options", &cfg_type_rndcconf_options, 0 },
	{ NULL, NULL, 0 }
};

static cfg_clausedef_t *rndcconf_clausesets[] = { rndcconf_clauses, NULL };

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_rndcconf = {
	"rndcconf",	 cfg_parse_mapbody, cfg_print_mapbody,
	cfg_doc_mapbody, &cfg_rep_map,	    rndcconf_clausesets
};

static cfg_clausedef_t rndckey_clauses[] = { { "key", &cfg_type_key, 0 },
					     { NULL, NULL, 0 } };

static cfg_clausedef_t *rndckey_clausesets[] = { rndckey_clauses, NULL };

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_rndckey = {
	"rndckey",	 cfg_parse_mapbody, cfg_print_mapbody,
	cfg_doc_mapbody, &cfg_rep_map,	    rndckey_clausesets
};

/*
 * session.key has exactly the same syntax as rndc.key, but it's defined
 * separately for clarity (and so we can extend it someday, if needed).
 */
LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_sessionkey = {
	"sessionkey",	 cfg_parse_mapbody, cfg_print_mapbody,
	cfg_doc_mapbody, &cfg_rep_map,	    rndckey_clausesets
};

static cfg_tuplefielddef_t nameport_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "port", &cfg_type_optional_port, 0 },
	{ "dscp", &cfg_type_optional_dscp, CFG_CLAUSEFLAG_DEPRECATED },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_nameport = { "nameport",	 cfg_parse_tuple,
					cfg_print_tuple, cfg_doc_tuple,
					&cfg_rep_tuple,	 nameport_fields };

static void
doc_sockaddrnameport(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "( ");
	cfg_print_cstr(pctx, "<quoted_string>");
	cfg_print_cstr(pctx, " ");
	cfg_print_cstr(pctx, "[ port <integer> ]");
	cfg_print_cstr(pctx, " ");
	cfg_print_cstr(pctx, "[ dscp <integer> ]");
	cfg_print_cstr(pctx, " | ");
	cfg_print_cstr(pctx, "<ipv4_address>");
	cfg_print_cstr(pctx, " ");
	cfg_print_cstr(pctx, "[ port <integer> ]");
	cfg_print_cstr(pctx, " ");
	cfg_print_cstr(pctx, "[ dscp <integer> ]");
	cfg_print_cstr(pctx, " | ");
	cfg_print_cstr(pctx, "<ipv6_address>");
	cfg_print_cstr(pctx, " ");
	cfg_print_cstr(pctx, "[ port <integer> ]");
	cfg_print_cstr(pctx, " ");
	cfg_print_cstr(pctx, "[ dscp <integer> ]");
	cfg_print_cstr(pctx, " )");
}

static isc_result_t
parse_sockaddrnameport(cfg_parser_t *pctx, const cfg_type_t *type,
		       cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	UNUSED(type);

	CHECK(cfg_peektoken(pctx, CFG_LEXOPT_QSTRING));
	if (pctx->token.type == isc_tokentype_string ||
	    pctx->token.type == isc_tokentype_qstring)
	{
		if (cfg_lookingat_netaddr(pctx, CFG_ADDR_V4OK | CFG_ADDR_V6OK))
		{
			CHECK(cfg_parse_sockaddr(pctx, &cfg_type_sockaddr,
						 ret));
		} else {
			const cfg_tuplefielddef_t *fields =
				cfg_type_nameport.of;
			CHECK(cfg_create_tuple(pctx, &cfg_type_nameport, &obj));
			CHECK(cfg_parse_obj(pctx, fields[0].type,
					    &obj->value.tuple[0]));
			CHECK(cfg_parse_obj(pctx, fields[1].type,
					    &obj->value.tuple[1]));
			CHECK(cfg_parse_obj(pctx, fields[2].type,
					    &obj->value.tuple[2]));
			*ret = obj;
			obj = NULL;
		}
	} else {
		cfg_parser_error(pctx, CFG_LOG_NEAR,
				 "expected IP address or hostname");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

static cfg_type_t cfg_type_sockaddrnameport = { "sockaddrnameport_element",
						parse_sockaddrnameport,
						NULL,
						doc_sockaddrnameport,
						NULL,
						NULL };

static cfg_type_t cfg_type_bracketed_sockaddrnameportlist = {
	"bracketed_sockaddrnameportlist",
	cfg_parse_bracketed_list,
	cfg_print_bracketed_list,
	cfg_doc_bracketed_list,
	&cfg_rep_list,
	&cfg_type_sockaddrnameport
};

/*%
 * A list of socket addresses or name with an optional default port,
 * as used in the dual-stack-servers option.  E.g.,
 * "port 1234 { dual-stack-servers.net; 10.0.0.1; 1::2 port 69; }"
 */
static cfg_tuplefielddef_t nameportiplist_fields[] = {
	{ "port", &cfg_type_optional_port, 0 },
	{ "addresses", &cfg_type_bracketed_sockaddrnameportlist, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_nameportiplist = {
	"nameportiplist", cfg_parse_tuple, cfg_print_tuple,
	cfg_doc_tuple,	  &cfg_rep_tuple,  nameportiplist_fields
};

/*%
 * primaries element.
 */

static void
doc_remoteselement(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "( ");
	cfg_print_cstr(pctx, "<remote-servers>");
	cfg_print_cstr(pctx, " | ");
	cfg_print_cstr(pctx, "<ipv4_address>");
	cfg_print_cstr(pctx, " ");
	cfg_print_cstr(pctx, "[ port <integer> ]");
	cfg_print_cstr(pctx, " | ");
	cfg_print_cstr(pctx, "<ipv6_address>");
	cfg_print_cstr(pctx, " ");
	cfg_print_cstr(pctx, "[ port <integer> ]");
	cfg_print_cstr(pctx, " )");
}

static isc_result_t
parse_remoteselement(cfg_parser_t *pctx, const cfg_type_t *type,
		     cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	UNUSED(type);

	CHECK(cfg_peektoken(pctx, CFG_LEXOPT_QSTRING));
	if (pctx->token.type == isc_tokentype_string ||
	    pctx->token.type == isc_tokentype_qstring)
	{
		if (cfg_lookingat_netaddr(pctx, CFG_ADDR_V4OK | CFG_ADDR_V6OK))
		{
			CHECK(cfg_parse_sockaddr(pctx, &cfg_type_sockaddr,
						 ret));
		} else {
			CHECK(cfg_parse_astring(pctx, &cfg_type_astring, ret));
		}
	} else {
		cfg_parser_error(pctx, CFG_LOG_NEAR,
				 "expected IP address or remote servers list "
				 "name");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

static cfg_type_t cfg_type_remoteselement = { "remotes_element",
					      parse_remoteselement,
					      NULL,
					      doc_remoteselement,
					      NULL,
					      NULL };

static int
cmp_clause(const void *ap, const void *bp) {
	const cfg_clausedef_t *a = (const cfg_clausedef_t *)ap;
	const cfg_clausedef_t *b = (const cfg_clausedef_t *)bp;
	return (strcmp(a->name, b->name));
}

bool
cfg_clause_validforzone(const char *name, unsigned int ztype) {
	const cfg_clausedef_t *clause;
	bool valid = false;

	for (clause = zone_clauses; clause->name != NULL; clause++) {
		if ((clause->flags & ztype) == 0 ||
		    strcmp(clause->name, name) != 0)
		{
			continue;
		}
		valid = true;
	}
	for (clause = zone_only_clauses; clause->name != NULL; clause++) {
		if ((clause->flags & ztype) == 0 ||
		    strcmp(clause->name, name) != 0)
		{
			continue;
		}
		valid = true;
	}

	return (valid);
}

void
cfg_print_zonegrammar(const unsigned int zonetype, unsigned int flags,
		      void (*f)(void *closure, const char *text, int textlen),
		      void *closure) {
#define NCLAUSES                                               \
	(((sizeof(zone_clauses) + sizeof(zone_only_clauses)) / \
	  sizeof(clause[0])) -                                 \
	 1)

	cfg_printer_t pctx;
	cfg_clausedef_t *clause = NULL;
	cfg_clausedef_t clauses[NCLAUSES];

	pctx.f = f;
	pctx.closure = closure;
	pctx.indent = 0;
	pctx.flags = flags;

	memmove(clauses, zone_clauses, sizeof(zone_clauses));
	memmove(clauses + sizeof(zone_clauses) / sizeof(zone_clauses[0]) - 1,
		zone_only_clauses, sizeof(zone_only_clauses));
	qsort(clauses, NCLAUSES - 1, sizeof(clause[0]), cmp_clause);

	cfg_print_cstr(&pctx, "zone <string> [ <class> ] {\n");
	pctx.indent++;

	switch (zonetype) {
	case CFG_ZONE_PRIMARY:
		cfg_print_indent(&pctx);
		cfg_print_cstr(&pctx, "type ( master | primary );\n");
		break;
	case CFG_ZONE_SECONDARY:
		cfg_print_indent(&pctx);
		cfg_print_cstr(&pctx, "type ( slave | secondary );\n");
		break;
	case CFG_ZONE_MIRROR:
		cfg_print_indent(&pctx);
		cfg_print_cstr(&pctx, "type mirror;\n");
		break;
	case CFG_ZONE_STUB:
		cfg_print_indent(&pctx);
		cfg_print_cstr(&pctx, "type stub;\n");
		break;
	case CFG_ZONE_HINT:
		cfg_print_indent(&pctx);
		cfg_print_cstr(&pctx, "type hint;\n");
		break;
	case CFG_ZONE_FORWARD:
		cfg_print_indent(&pctx);
		cfg_print_cstr(&pctx, "type forward;\n");
		break;
	case CFG_ZONE_STATICSTUB:
		cfg_print_indent(&pctx);
		cfg_print_cstr(&pctx, "type static-stub;\n");
		break;
	case CFG_ZONE_REDIRECT:
		cfg_print_indent(&pctx);
		cfg_print_cstr(&pctx, "type redirect;\n");
		break;
	case CFG_ZONE_DELEGATION:
		cfg_print_indent(&pctx);
		cfg_print_cstr(&pctx, "type delegation-only;\n");
		break;
	case CFG_ZONE_INVIEW:
		/* no zone type is specified for these */
		break;
	default:
		UNREACHABLE();
	}

	for (clause = clauses; clause->name != NULL; clause++) {
		if (((pctx.flags & CFG_PRINTER_ACTIVEONLY) != 0) &&
		    (((clause->flags & CFG_CLAUSEFLAG_OBSOLETE) != 0) ||
		     ((clause->flags & CFG_CLAUSEFLAG_ANCIENT) != 0) ||
		     ((clause->flags & CFG_CLAUSEFLAG_NYI) != 0) ||
		     ((clause->flags & CFG_CLAUSEFLAG_TESTONLY) != 0)))
		{
			continue;
		}
		if ((clause->flags & zonetype) == 0 ||
		    strcasecmp(clause->name, "type") == 0)
		{
			continue;
		}
		cfg_print_indent(&pctx);
		cfg_print_cstr(&pctx, clause->name);
		cfg_print_cstr(&pctx, " ");
		cfg_doc_obj(&pctx, clause->type);
		cfg_print_cstr(&pctx, ";");
		cfg_print_clauseflags(&pctx, clause->flags);
		cfg_print_cstr(&pctx, "\n");
	}

	pctx.indent--;
	cfg_print_cstr(&pctx, "};\n");
}
