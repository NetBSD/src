/*	$NetBSD: ssu.c,v 1.7 2023/01/25 21:43:30 christos Exp $	*/

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

#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/print.h>
#include <isc/refcount.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/dlz.h>
#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/ssu.h>

#include <dst/dst.h>
#include <dst/gssapi.h>

#define SSUTABLEMAGIC	      ISC_MAGIC('S', 'S', 'U', 'T')
#define VALID_SSUTABLE(table) ISC_MAGIC_VALID(table, SSUTABLEMAGIC)

#define SSURULEMAGIC	     ISC_MAGIC('S', 'S', 'U', 'R')
#define VALID_SSURULE(table) ISC_MAGIC_VALID(table, SSURULEMAGIC)

struct dns_ssurule {
	unsigned int magic;
	bool grant;		      /*%< is this a grant or a deny? */
	dns_ssumatchtype_t matchtype; /*%< which type of pattern match?
				       * */
	dns_name_t *identity;	      /*%< the identity to match */
	dns_name_t *name;	      /*%< the name being updated */
	unsigned int ntypes;	      /*%< number of data types covered */
	dns_rdatatype_t *types;	      /*%< the data types.  Can include */
				      /*   ANY. if NULL, defaults to all */
				      /*   types except SIG, SOA, and NS */
	ISC_LINK(dns_ssurule_t) link;
};

struct dns_ssutable {
	unsigned int magic;
	isc_mem_t *mctx;
	isc_refcount_t references;
	dns_dlzdb_t *dlzdatabase;
	ISC_LIST(dns_ssurule_t) rules;
};

isc_result_t
dns_ssutable_create(isc_mem_t *mctx, dns_ssutable_t **tablep) {
	dns_ssutable_t *table;

	REQUIRE(tablep != NULL && *tablep == NULL);
	REQUIRE(mctx != NULL);

	table = isc_mem_get(mctx, sizeof(dns_ssutable_t));
	isc_refcount_init(&table->references, 1);
	table->mctx = NULL;
	isc_mem_attach(mctx, &table->mctx);
	ISC_LIST_INIT(table->rules);
	table->magic = SSUTABLEMAGIC;
	*tablep = table;
	return (ISC_R_SUCCESS);
}

static void
destroy(dns_ssutable_t *table) {
	isc_mem_t *mctx;

	REQUIRE(VALID_SSUTABLE(table));

	mctx = table->mctx;
	while (!ISC_LIST_EMPTY(table->rules)) {
		dns_ssurule_t *rule = ISC_LIST_HEAD(table->rules);
		if (rule->identity != NULL) {
			dns_name_free(rule->identity, mctx);
			isc_mem_put(mctx, rule->identity, sizeof(dns_name_t));
		}
		if (rule->name != NULL) {
			dns_name_free(rule->name, mctx);
			isc_mem_put(mctx, rule->name, sizeof(dns_name_t));
		}
		if (rule->types != NULL) {
			isc_mem_put(mctx, rule->types,
				    rule->ntypes * sizeof(dns_rdatatype_t));
		}
		ISC_LIST_UNLINK(table->rules, rule, link);
		rule->magic = 0;
		isc_mem_put(mctx, rule, sizeof(dns_ssurule_t));
	}
	isc_refcount_destroy(&table->references);
	table->magic = 0;
	isc_mem_putanddetach(&table->mctx, table, sizeof(dns_ssutable_t));
}

void
dns_ssutable_attach(dns_ssutable_t *source, dns_ssutable_t **targetp) {
	REQUIRE(VALID_SSUTABLE(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);

	*targetp = source;
}

void
dns_ssutable_detach(dns_ssutable_t **tablep) {
	dns_ssutable_t *table;

	REQUIRE(tablep != NULL);
	table = *tablep;
	*tablep = NULL;
	REQUIRE(VALID_SSUTABLE(table));

	if (isc_refcount_decrement(&table->references) == 1) {
		destroy(table);
	}
}

isc_result_t
dns_ssutable_addrule(dns_ssutable_t *table, bool grant,
		     const dns_name_t *identity, dns_ssumatchtype_t matchtype,
		     const dns_name_t *name, unsigned int ntypes,
		     dns_rdatatype_t *types) {
	dns_ssurule_t *rule;
	isc_mem_t *mctx;

	REQUIRE(VALID_SSUTABLE(table));
	REQUIRE(dns_name_isabsolute(identity));
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(matchtype <= dns_ssumatchtype_max);
	if (matchtype == dns_ssumatchtype_wildcard) {
		REQUIRE(dns_name_iswildcard(name));
	}
	if (ntypes > 0) {
		REQUIRE(types != NULL);
	}

	mctx = table->mctx;
	rule = isc_mem_get(mctx, sizeof(dns_ssurule_t));

	rule->identity = NULL;
	rule->name = NULL;
	rule->types = NULL;

	rule->grant = grant;

	rule->identity = isc_mem_get(mctx, sizeof(dns_name_t));
	dns_name_init(rule->identity, NULL);
	dns_name_dup(identity, mctx, rule->identity);

	rule->name = isc_mem_get(mctx, sizeof(dns_name_t));
	dns_name_init(rule->name, NULL);
	dns_name_dup(name, mctx, rule->name);

	rule->matchtype = matchtype;

	rule->ntypes = ntypes;
	if (ntypes > 0) {
		rule->types = isc_mem_get(mctx,
					  ntypes * sizeof(dns_rdatatype_t));
		memmove(rule->types, types, ntypes * sizeof(dns_rdatatype_t));
	} else {
		rule->types = NULL;
	}

	rule->magic = SSURULEMAGIC;
	ISC_LIST_INITANDAPPEND(table->rules, rule, link);

	return (ISC_R_SUCCESS);
}

static bool
isusertype(dns_rdatatype_t type) {
	return (type != dns_rdatatype_ns && type != dns_rdatatype_soa &&
		type != dns_rdatatype_rrsig);
}

static void
reverse_from_address(dns_name_t *tcpself, const isc_netaddr_t *tcpaddr) {
	char buf[16 * 4 + sizeof("IP6.ARPA.")];
	isc_result_t result;
	const unsigned char *ap;
	isc_buffer_t b;
	unsigned long l;

	switch (tcpaddr->family) {
	case AF_INET:
		l = ntohl(tcpaddr->type.in.s_addr);
		result = snprintf(buf, sizeof(buf),
				  "%lu.%lu.%lu.%lu.IN-ADDR.ARPA.",
				  (l >> 0) & 0xff, (l >> 8) & 0xff,
				  (l >> 16) & 0xff, (l >> 24) & 0xff);
		RUNTIME_CHECK(result < sizeof(buf));
		break;
	case AF_INET6:
		ap = tcpaddr->type.in6.s6_addr;
		result = snprintf(
			buf, sizeof(buf),
			"%x.%x.%x.%x.%x.%x.%x.%x."
			"%x.%x.%x.%x.%x.%x.%x.%x."
			"%x.%x.%x.%x.%x.%x.%x.%x."
			"%x.%x.%x.%x.%x.%x.%x.%x."
			"IP6.ARPA.",
			ap[15] & 0x0f, (ap[15] >> 4) & 0x0f, ap[14] & 0x0f,
			(ap[14] >> 4) & 0x0f, ap[13] & 0x0f,
			(ap[13] >> 4) & 0x0f, ap[12] & 0x0f,
			(ap[12] >> 4) & 0x0f, ap[11] & 0x0f,
			(ap[11] >> 4) & 0x0f, ap[10] & 0x0f,
			(ap[10] >> 4) & 0x0f, ap[9] & 0x0f, (ap[9] >> 4) & 0x0f,
			ap[8] & 0x0f, (ap[8] >> 4) & 0x0f, ap[7] & 0x0f,
			(ap[7] >> 4) & 0x0f, ap[6] & 0x0f, (ap[6] >> 4) & 0x0f,
			ap[5] & 0x0f, (ap[5] >> 4) & 0x0f, ap[4] & 0x0f,
			(ap[4] >> 4) & 0x0f, ap[3] & 0x0f, (ap[3] >> 4) & 0x0f,
			ap[2] & 0x0f, (ap[2] >> 4) & 0x0f, ap[1] & 0x0f,
			(ap[1] >> 4) & 0x0f, ap[0] & 0x0f, (ap[0] >> 4) & 0x0f);
		RUNTIME_CHECK(result < sizeof(buf));
		break;
	default:
		UNREACHABLE();
	}
	isc_buffer_init(&b, buf, strlen(buf));
	isc_buffer_add(&b, strlen(buf));
	result = dns_name_fromtext(tcpself, &b, dns_rootname, 0, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
}

static void
stf_from_address(dns_name_t *stfself, const isc_netaddr_t *tcpaddr) {
	char buf[sizeof("X.X.X.X.Y.Y.Y.Y.2.0.0.2.IP6.ARPA.")];
	isc_result_t result;
	const unsigned char *ap;
	isc_buffer_t b;
	unsigned long l;

	switch (tcpaddr->family) {
	case AF_INET:
		l = ntohl(tcpaddr->type.in.s_addr);
		result = snprintf(buf, sizeof(buf),
				  "%lx.%lx.%lx.%lx.%lx.%lx.%lx.%lx"
				  "2.0.0.2.IP6.ARPA.",
				  l & 0xf, (l >> 4) & 0xf, (l >> 8) & 0xf,
				  (l >> 12) & 0xf, (l >> 16) & 0xf,
				  (l >> 20) & 0xf, (l >> 24) & 0xf,
				  (l >> 28) & 0xf);
		RUNTIME_CHECK(result < sizeof(buf));
		break;
	case AF_INET6:
		ap = tcpaddr->type.in6.s6_addr;
		result = snprintf(
			buf, sizeof(buf),
			"%x.%x.%x.%x.%x.%x.%x.%x."
			"%x.%x.%x.%x.IP6.ARPA.",
			ap[5] & 0x0f, (ap[5] >> 4) & 0x0f, ap[4] & 0x0f,
			(ap[4] >> 4) & 0x0f, ap[3] & 0x0f, (ap[3] >> 4) & 0x0f,
			ap[2] & 0x0f, (ap[2] >> 4) & 0x0f, ap[1] & 0x0f,
			(ap[1] >> 4) & 0x0f, ap[0] & 0x0f, (ap[0] >> 4) & 0x0f);
		RUNTIME_CHECK(result < sizeof(buf));
		break;
	default:
		UNREACHABLE();
	}
	isc_buffer_init(&b, buf, strlen(buf));
	isc_buffer_add(&b, strlen(buf));
	result = dns_name_fromtext(stfself, &b, dns_rootname, 0, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
}

bool
dns_ssutable_checkrules(dns_ssutable_t *table, const dns_name_t *signer,
			const dns_name_t *name, const isc_netaddr_t *addr,
			bool tcp, const dns_aclenv_t *env, dns_rdatatype_t type,
			const dst_key_t *key) {
	dns_ssurule_t *rule;
	unsigned int i;
	dns_fixedname_t fixed;
	dns_name_t *wildcard;
	dns_name_t *tcpself;
	dns_name_t *stfself;
	isc_result_t result;
	int match;

	REQUIRE(VALID_SSUTABLE(table));
	REQUIRE(signer == NULL || dns_name_isabsolute(signer));
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(addr == NULL || env != NULL);

	if (signer == NULL && addr == NULL) {
		return (false);
	}

	for (rule = ISC_LIST_HEAD(table->rules); rule != NULL;
	     rule = ISC_LIST_NEXT(rule, link))
	{
		switch (rule->matchtype) {
		case dns_ssumatchtype_name:
		case dns_ssumatchtype_local:
		case dns_ssumatchtype_subdomain:
		case dns_ssumatchtype_wildcard:
		case dns_ssumatchtype_self:
		case dns_ssumatchtype_selfsub:
		case dns_ssumatchtype_selfwild:
			if (signer == NULL) {
				continue;
			}
			if (dns_name_iswildcard(rule->identity)) {
				if (!dns_name_matcheswildcard(signer,
							      rule->identity))
				{
					continue;
				}
			} else {
				if (!dns_name_equal(signer, rule->identity)) {
					continue;
				}
			}
			break;
		case dns_ssumatchtype_selfkrb5:
		case dns_ssumatchtype_selfms:
		case dns_ssumatchtype_selfsubkrb5:
		case dns_ssumatchtype_selfsubms:
		case dns_ssumatchtype_subdomainkrb5:
		case dns_ssumatchtype_subdomainms:
			if (signer == NULL) {
				continue;
			}
			break;
		case dns_ssumatchtype_tcpself:
		case dns_ssumatchtype_6to4self:
			if (!tcp || addr == NULL) {
				continue;
			}
			break;
		case dns_ssumatchtype_external:
		case dns_ssumatchtype_dlz:
			break;
		}

		switch (rule->matchtype) {
		case dns_ssumatchtype_name:
			if (!dns_name_equal(name, rule->name)) {
				continue;
			}
			break;
		case dns_ssumatchtype_subdomain:
			if (!dns_name_issubdomain(name, rule->name)) {
				continue;
			}
			break;
		case dns_ssumatchtype_local:
			if (addr == NULL) {
				continue;
			}
			if (!dns_name_issubdomain(name, rule->name)) {
				continue;
			}
			dns_acl_match(addr, NULL, env->localhost, NULL, &match,
				      NULL);
			if (match == 0) {
				if (signer != NULL) {
					isc_log_write(dns_lctx,
						      DNS_LOGCATEGORY_GENERAL,
						      DNS_LOGMODULE_SSU,
						      ISC_LOG_WARNING,
						      "update-policy local: "
						      "match on session "
						      "key not from "
						      "localhost");
				}
				continue;
			}
			break;
		case dns_ssumatchtype_wildcard:
			if (!dns_name_matcheswildcard(name, rule->name)) {
				continue;
			}
			break;
		case dns_ssumatchtype_self:
			if (!dns_name_equal(signer, name)) {
				continue;
			}
			break;
		case dns_ssumatchtype_selfsub:
			if (!dns_name_issubdomain(name, signer)) {
				continue;
			}
			break;
		case dns_ssumatchtype_selfwild:
			wildcard = dns_fixedname_initname(&fixed);
			result = dns_name_concatenate(dns_wildcardname, signer,
						      wildcard, NULL);
			if (result != ISC_R_SUCCESS) {
				continue;
			}
			if (!dns_name_matcheswildcard(name, wildcard)) {
				continue;
			}
			break;
		case dns_ssumatchtype_selfkrb5:
			if (dst_gssapi_identitymatchesrealmkrb5(
				    signer, name, rule->identity, false))
			{
				break;
			}
			continue;
		case dns_ssumatchtype_selfms:
			if (dst_gssapi_identitymatchesrealmms(
				    signer, name, rule->identity, false))
			{
				break;
			}
			continue;
		case dns_ssumatchtype_selfsubkrb5:
			if (dst_gssapi_identitymatchesrealmkrb5(
				    signer, name, rule->identity, true))
			{
				break;
			}
			continue;
		case dns_ssumatchtype_selfsubms:
			if (dst_gssapi_identitymatchesrealmms(
				    signer, name, rule->identity, true))
			{
				break;
			}
			continue;
		case dns_ssumatchtype_subdomainkrb5:
			if (!dns_name_issubdomain(name, rule->name)) {
				continue;
			}
			if (dst_gssapi_identitymatchesrealmkrb5(
				    signer, NULL, rule->identity, false))
			{
				break;
			}
			continue;
		case dns_ssumatchtype_subdomainms:
			if (!dns_name_issubdomain(name, rule->name)) {
				continue;
			}
			if (dst_gssapi_identitymatchesrealmms(
				    signer, NULL, rule->identity, false))
			{
				break;
			}
			continue;
		case dns_ssumatchtype_tcpself:
			tcpself = dns_fixedname_initname(&fixed);
			reverse_from_address(tcpself, addr);
			if (dns_name_iswildcard(rule->identity)) {
				if (!dns_name_matcheswildcard(tcpself,
							      rule->identity))
				{
					continue;
				}
			} else {
				if (!dns_name_equal(tcpself, rule->identity)) {
					continue;
				}
			}
			if (!dns_name_equal(tcpself, name)) {
				continue;
			}
			break;
		case dns_ssumatchtype_6to4self:
			stfself = dns_fixedname_initname(&fixed);
			stf_from_address(stfself, addr);
			if (dns_name_iswildcard(rule->identity)) {
				if (!dns_name_matcheswildcard(stfself,
							      rule->identity))
				{
					continue;
				}
			} else {
				if (!dns_name_equal(stfself, rule->identity)) {
					continue;
				}
			}
			if (!dns_name_equal(stfself, name)) {
				continue;
			}
			break;
		case dns_ssumatchtype_external:
			if (!dns_ssu_external_match(rule->identity, signer,
						    name, addr, type, key,
						    table->mctx))
			{
				continue;
			}
			break;
		case dns_ssumatchtype_dlz:
			if (!dns_dlz_ssumatch(table->dlzdatabase, signer, name,
					      addr, type, key))
			{
				continue;
			}
			break;
		}

		if (rule->ntypes == 0) {
			/*
			 * If this is a DLZ rule, then the DLZ ssu
			 * checks will have already checked
			 * the type.
			 */
			if (rule->matchtype != dns_ssumatchtype_dlz &&
			    !isusertype(type))
			{
				continue;
			}
		} else {
			for (i = 0; i < rule->ntypes; i++) {
				if (rule->types[i] == dns_rdatatype_any ||
				    rule->types[i] == type)
				{
					break;
				}
			}
			if (i == rule->ntypes) {
				continue;
			}
		}
		return (rule->grant);
	}

	return (false);
}

bool
dns_ssurule_isgrant(const dns_ssurule_t *rule) {
	REQUIRE(VALID_SSURULE(rule));
	return (rule->grant);
}

dns_name_t *
dns_ssurule_identity(const dns_ssurule_t *rule) {
	REQUIRE(VALID_SSURULE(rule));
	return (rule->identity);
}

unsigned int
dns_ssurule_matchtype(const dns_ssurule_t *rule) {
	REQUIRE(VALID_SSURULE(rule));
	return (rule->matchtype);
}

dns_name_t *
dns_ssurule_name(const dns_ssurule_t *rule) {
	REQUIRE(VALID_SSURULE(rule));
	return (rule->name);
}

unsigned int
dns_ssurule_types(const dns_ssurule_t *rule, dns_rdatatype_t **types) {
	REQUIRE(VALID_SSURULE(rule));
	REQUIRE(types != NULL && *types != NULL);
	*types = rule->types;
	return (rule->ntypes);
}

isc_result_t
dns_ssutable_firstrule(const dns_ssutable_t *table, dns_ssurule_t **rule) {
	REQUIRE(VALID_SSUTABLE(table));
	REQUIRE(rule != NULL && *rule == NULL);
	*rule = ISC_LIST_HEAD(table->rules);
	return (*rule != NULL ? ISC_R_SUCCESS : ISC_R_NOMORE);
}

isc_result_t
dns_ssutable_nextrule(dns_ssurule_t *rule, dns_ssurule_t **nextrule) {
	REQUIRE(VALID_SSURULE(rule));
	REQUIRE(nextrule != NULL && *nextrule == NULL);
	*nextrule = ISC_LIST_NEXT(rule, link);
	return (*nextrule != NULL ? ISC_R_SUCCESS : ISC_R_NOMORE);
}

/*
 * Create a specialised SSU table that points at an external DLZ database
 */
isc_result_t
dns_ssutable_createdlz(isc_mem_t *mctx, dns_ssutable_t **tablep,
		       dns_dlzdb_t *dlzdatabase) {
	isc_result_t result;
	dns_ssurule_t *rule;
	dns_ssutable_t *table = NULL;

	REQUIRE(tablep != NULL && *tablep == NULL);

	result = dns_ssutable_create(mctx, &table);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	table->dlzdatabase = dlzdatabase;

	rule = isc_mem_get(table->mctx, sizeof(dns_ssurule_t));

	rule->identity = NULL;
	rule->name = NULL;
	rule->types = NULL;
	rule->grant = true;
	rule->matchtype = dns_ssumatchtype_dlz;
	rule->ntypes = 0;
	rule->types = NULL;
	rule->magic = SSURULEMAGIC;

	ISC_LIST_INITANDAPPEND(table->rules, rule, link);
	*tablep = table;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_ssu_mtypefromstring(const char *str, dns_ssumatchtype_t *mtype) {
	REQUIRE(str != NULL);
	REQUIRE(mtype != NULL);

	if (strcasecmp(str, "name") == 0) {
		*mtype = dns_ssumatchtype_name;
	} else if (strcasecmp(str, "subdomain") == 0) {
		*mtype = dns_ssumatchtype_subdomain;
	} else if (strcasecmp(str, "wildcard") == 0) {
		*mtype = dns_ssumatchtype_wildcard;
	} else if (strcasecmp(str, "self") == 0) {
		*mtype = dns_ssumatchtype_self;
	} else if (strcasecmp(str, "selfsub") == 0) {
		*mtype = dns_ssumatchtype_selfsub;
	} else if (strcasecmp(str, "selfwild") == 0) {
		*mtype = dns_ssumatchtype_selfwild;
	} else if (strcasecmp(str, "ms-self") == 0) {
		*mtype = dns_ssumatchtype_selfms;
	} else if (strcasecmp(str, "ms-selfsub") == 0) {
		*mtype = dns_ssumatchtype_selfsubms;
	} else if (strcasecmp(str, "krb5-self") == 0) {
		*mtype = dns_ssumatchtype_selfkrb5;
	} else if (strcasecmp(str, "krb5-selfsub") == 0) {
		*mtype = dns_ssumatchtype_selfsubkrb5;
	} else if (strcasecmp(str, "ms-subdomain") == 0) {
		*mtype = dns_ssumatchtype_subdomainms;
	} else if (strcasecmp(str, "krb5-subdomain") == 0) {
		*mtype = dns_ssumatchtype_subdomainkrb5;
	} else if (strcasecmp(str, "tcp-self") == 0) {
		*mtype = dns_ssumatchtype_tcpself;
	} else if (strcasecmp(str, "6to4-self") == 0) {
		*mtype = dns_ssumatchtype_6to4self;
	} else if (strcasecmp(str, "zonesub") == 0) {
		*mtype = dns_ssumatchtype_subdomain;
	} else if (strcasecmp(str, "external") == 0) {
		*mtype = dns_ssumatchtype_external;
	} else {
		return (ISC_R_NOTFOUND);
	}
	return (ISC_R_SUCCESS);
}
