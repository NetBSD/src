/*	$NetBSD: dnsconf.c,v 1.2 2018/08/12 13:02:40 christos Exp $	*/

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

#include <isccfg/cfg.h>
#include <isccfg/grammar.h>

/*%
 * A trusted key, as used in the "trusted-keys" statement.
 */
static cfg_tuplefielddef_t trustedkey_fields[] = {
	{ "name", &cfg_type_astring, 0 },
	{ "flags", &cfg_type_uint32, 0 },
	{ "protocol", &cfg_type_uint32, 0 },
	{ "algorithm", &cfg_type_uint32, 0 },
	{ "key", &cfg_type_qstring, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_trustedkey = {
	"trustedkey", cfg_parse_tuple, cfg_print_tuple, cfg_doc_tuple,
	&cfg_rep_tuple, trustedkey_fields
};

static cfg_type_t cfg_type_trustedkeys = {
	"trusted-keys", cfg_parse_bracketed_list, cfg_print_bracketed_list,
	cfg_doc_bracketed_list, &cfg_rep_list, &cfg_type_trustedkey
};

/*%
 * Clauses that can be found within the top level of the dns.conf
 * file only.
 */
static cfg_clausedef_t
dnsconf_clauses[] = {
	{ "trusted-keys", &cfg_type_trustedkeys, CFG_CLAUSEFLAG_MULTI },
	{ NULL, NULL, 0 }
};

/*% The top-level dns.conf syntax. */

static cfg_clausedef_t *
dnsconf_clausesets[] = {
	dnsconf_clauses,
	NULL
};

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_dnsconf = {
	"dnsconf", cfg_parse_mapbody, cfg_print_mapbody, cfg_doc_mapbody,
	&cfg_rep_map, dnsconf_clausesets
};
