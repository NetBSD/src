/*	$NetBSD: config.c,v 1.4 2019/02/24 20:01:27 christos Exp $	*/

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

#include <inttypes.h>
#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/parseint.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/string.h>
#include <isc/util.h>

#include <pk11/site.h>

#include <isccfg/grammar.h>
#include <isccfg/namedconf.h>

#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/rdataclass.h>
#include <dns/rdatatype.h>
#include <dns/tsig.h>
#include <dns/zone.h>

#include <dst/dst.h>

#include <named/config.h>
#include <named/globals.h>

#include <bind.keys.h>

/*% default configuration */
static char defaultconf[] = "\
options {\n\
	answer-cookie true;\n\
	automatic-interface-scan yes;\n\
	bindkeys-file \"" NAMED_SYSCONFDIR "/bind.keys\";\n\
#	blackhole {none;};\n"
"	cookie-algorithm aes;\n"
#ifndef WIN32
"	coresize default;\n\
	datasize default;\n"
#endif
"\
#	deallocate-on-exit <obsolete>;\n\
#	directory <none>\n\
	dump-file \"named_dump.db\";\n\
	edns-udp-size 4096;\n\
#	fake-iquery <obsolete>;\n"
#ifndef WIN32
"	files unlimited;\n"
#endif
"\
#	has-old-clients <obsolete>;\n\
	heartbeat-interval 60;\n\
#	host-statistics <obsolete>;\n\
	interface-interval 60;\n\
#	keep-response-order {none;};\n\
	listen-on {any;};\n\
	listen-on-v6 {any;};\n\
#	lock-file \"" NAMED_LOCALSTATEDIR "/run/named/named.lock\";\n\
	match-mapped-addresses no;\n\
	max-rsa-exponent-size 0; /* no limit */\n\
	max-udp-size 4096;\n\
	memstatistics-file \"named.memstats\";\n\
#	multiple-cnames <obsolete>;\n\
#	named-xfer <obsolete>;\n\
	nocookie-udp-size 4096;\n\
	notify-rate 20;\n\
	nta-lifetime 3600;\n\
	nta-recheck 300;\n\
#	pid-file \"" NAMED_LOCALSTATEDIR "/run/named/named.pid\"; \n\
	port 53;\n\
	prefetch 2 9;\n\
	recursing-file \"named.recursing\";\n\
	recursive-clients 1000;\n\
	request-nsid false;\n\
	reserved-sockets 512;\n\
	resolver-query-timeout 10;\n\
	rrset-order { order random; };\n\
	secroots-file \"named.secroots\";\n\
	send-cookie true;\n\
#	serial-queries <obsolete>;\n\
	serial-query-rate 20;\n\
	server-id none;\n\
	session-keyalg hmac-sha256;\n\
#	session-keyfile \"" NAMED_LOCALSTATEDIR "/run/named/session.key\";\n\
	session-keyname local-ddns;\n"
#ifndef WIN32
"	stacksize default;\n"
#endif
"	startup-notify-rate 20;\n\
	statistics-file \"named.stats\";\n\
#	statistics-interval <obsolete>;\n\
	tcp-advertised-timeout 300;\n\
	tcp-clients 150;\n\
	tcp-idle-timeout 300;\n\
	tcp-initial-timeout 300;\n\
	tcp-keepalive-timeout 300;\n\
	tcp-listen-queue 10;\n\
#	tkey-dhkey <none>\n\
#	tkey-domain <none>\n\
#	tkey-gssapi-credential <none>\n\
	transfer-message-size 20480;\n\
	transfers-in 10;\n\
	transfers-out 10;\n\
	transfers-per-ns 2;\n\
#	treat-cr-as-space <obsolete>;\n\
	trust-anchor-telemetry yes;\n\
#	use-id-pool <obsolete>;\n\
#	use-ixfr <obsolete>;\n\
\n\
	/* view */\n\
	allow-new-zones no;\n\
	allow-notify {none;};\n\
	allow-query-cache { localnets; localhost; };\n\
	allow-query-cache-on { any; };\n\
	allow-recursion { localnets; localhost; };\n\
	allow-recursion-on { any; };\n\
	allow-update-forwarding {none;};\n\
#	allow-v6-synthesis <obsolete>;\n\
	auth-nxdomain false;\n\
	check-dup-records warn;\n\
	check-mx warn;\n\
	check-names master fail;\n\
	check-names response ignore;\n\
	check-names slave warn;\n\
	check-spf warn;\n\
	cleaning-interval 0;  /* now meaningless */\n\
	clients-per-query 10;\n\
	dnssec-accept-expired no;\n\
	dnssec-enable yes;\n\
	dnssec-validation " VALIDATION_DEFAULT "; \n"
#ifdef HAVE_DNSTAP
"	dnstap-identity hostname;\n"
#endif
"\
#	fetch-glue <obsolete>;\n\
	fetch-quota-params 100 0.1 0.3 0.7;\n\
	fetches-per-server 0;\n\
	fetches-per-zone 0;\n"
#ifdef HAVE_GEOIP
"	geoip-use-ecs yes;\n"
#endif
"	glue-cache yes;\n\
	lame-ttl 600;\n"
#ifdef HAVE_LMDB
"	lmdb-mapsize 32M;\n"
#endif
"	max-cache-size 90%;\n\
	max-cache-ttl 604800; /* 1 week */\n\
	max-clients-per-query 100;\n\
	max-ncache-ttl 10800; /* 3 hours */\n\
	max-recursion-depth 7;\n\
	max-recursion-queries 75;\n\
	max-stale-ttl 604800; /* 1 week */\n\
	message-compression yes;\n\
	min-ncache-ttl 0; /* 0 hours */\n\
	min-cache-ttl 0; /* 0 seconds */\n\
#	min-roots <obsolete>;\n\
	minimal-any false;\n\
	minimal-responses no-auth-recursive;\n\
	notify-source *;\n\
	notify-source-v6 *;\n\
	nsec3-test-zone no;\n\
	provide-ixfr true;\n\
	qname-minimization relaxed;\n\
	query-source address *;\n\
	query-source-v6 address *;\n\
	recursion true;\n\
	request-expire true;\n\
	request-ixfr true;\n\
	require-server-cookie no;\n\
	resolver-nonbackoff-tries 3;\n\
	resolver-retry-interval 800; /* in milliseconds */\n\
#	rfc2308-type1 <obsolete>;\n\
	root-key-sentinel yes;\n\
	servfail-ttl 1;\n\
#	sortlist <none>\n\
	stale-answer-enable false;\n\
	stale-answer-ttl 1; /* 1 second */\n\
	synth-from-dnssec yes;\n\
#	topology <none>\n\
	transfer-format many-answers;\n\
	v6-bias 50;\n\
	zero-no-soa-ttl-cache no;\n\
\n\
	/* zone */\n\
	allow-query {any;};\n\
	allow-query-on {any;};\n\
	allow-transfer {any;};\n\
#	also-notify <none>\n\
	alt-transfer-source *;\n\
	alt-transfer-source-v6 *;\n\
	check-integrity yes;\n\
	check-mx-cname warn;\n\
	check-sibling yes;\n\
	check-srv-cname warn;\n\
	check-wildcard yes;\n\
	dialup no;\n\
	dnssec-dnskey-kskonly no;\n\
	dnssec-loadkeys-interval 60;\n\
	dnssec-secure-to-insecure no;\n\
	dnssec-update-mode maintain;\n\
#	forward <none>\n\
#	forwarders <none>\n\
	inline-signing no;\n\
	ixfr-from-differences false;\n\
#	maintain-ixfr-base <obsolete>;\n\
#	max-ixfr-log-size <obsolete>\n\
	max-journal-size default;\n\
	max-records 0;\n\
	max-refresh-time 2419200; /* 4 weeks */\n\
	max-retry-time 1209600; /* 2 weeks */\n\
	max-transfer-idle-in 60;\n\
	max-transfer-idle-out 60;\n\
	max-transfer-time-in 120;\n\
	max-transfer-time-out 120;\n\
	min-refresh-time 300;\n\
	min-retry-time 500;\n\
	multi-master no;\n\
	notify yes;\n\
	notify-delay 5;\n\
	notify-to-soa no;\n\
	serial-update-method increment;\n\
	sig-signing-nodes 100;\n\
	sig-signing-signatures 10;\n\
	sig-signing-type 65534;\n\
	sig-validity-interval 30; /* days */\n\
	dnskey-sig-validity 0; /* default: sig-validity-interval */\n\
	transfer-source *;\n\
	transfer-source-v6 *;\n\
	try-tcp-refresh yes; /* BIND 8 compat */\n\
	update-check-ksk yes;\n\
	zero-no-soa-ttl yes;\n\
	zone-statistics terse;\n\
};\n\
"

"#\n\
#  Zones in the \"_bind\" view are NOT counted in the count of zones.\n\
#\n\
view \"_bind\" chaos {\n\
	recursion no;\n\
	notify no;\n\
	allow-new-zones no;\n\
\n\
	# Prevent use of this zone in DNS amplified reflection DoS attacks\n\
	rate-limit {\n\
		responses-per-second 3;\n\
		slip 0;\n\
		min-table-size 10;\n\
	};\n\
\n\
	zone \"version.bind\" chaos {\n\
		type master;\n\
		database \"_builtin version\";\n\
	};\n\
\n\
	zone \"hostname.bind\" chaos {\n\
		type master;\n\
		database \"_builtin hostname\";\n\
	};\n\
\n\
	zone \"authors.bind\" chaos {\n\
		type master;\n\
		database \"_builtin authors\";\n\
	};\n\
\n\
	zone \"id.server\" chaos {\n\
		type master;\n\
		database \"_builtin id\";\n\
	};\n\
};\n\
"
"#\n\
#  Default trusted key(s), used if \n\
# \"dnssec-validation auto;\" is set and\n\
#  sysconfdir/bind.keys doesn't exist).\n\
#\n\
# BEGIN MANAGED KEYS\n"

/* Imported from bind.keys.h: */
MANAGED_KEYS

"# END MANAGED KEYS\n\
\n\
masters " DEFAULT_IANA_ROOT_ZONE_MASTERS " {\n\
	2001:500:84::b;		# b.root-servers.net\n\
	2001:500:2f::f;		# f.root-servers.net\n\
	2001:7fd::1;		# k.root-servers.net\n\
	2620:0:2830:202::132;	# xfr.cjr.dns.icann.org\n\
	2620:0:2d0:202::132;	# xfr.lax.dns.icann.org\n\
	192.228.79.201;		# b.root-servers.net\n\
	192.33.4.12;		# c.root-servers.net\n\
	192.5.5.241;		# f.root-servers.net\n\
	192.112.36.4;		# g.root-servers.net\n\
	193.0.14.129;		# k.root-servers.net\n\
	192.0.47.132;		# xfr.cjr.dns.icann.org\n\
	192.0.32.132;		# xfr.lax.dns.icann.org\n\
};\n\
";

isc_result_t
named_config_parsedefaults(cfg_parser_t *parser, cfg_obj_t **conf) {
	isc_buffer_t b;

	isc_buffer_init(&b, defaultconf, sizeof(defaultconf) - 1);
	isc_buffer_add(&b, sizeof(defaultconf) - 1);
	return (cfg_parse_buffer(parser, &b, __FILE__, 0,
				 &cfg_type_namedconf,
				 CFG_PCTX_NODEPRECATED, conf));
}

isc_result_t
named_config_get(cfg_obj_t const * const *maps, const char *name,
		 const cfg_obj_t **obj)
{
	int i;

	for (i = 0;; i++) {
		if (maps[i] == NULL)
			return (ISC_R_NOTFOUND);
		if (cfg_map_get(maps[i], name, obj) == ISC_R_SUCCESS)
			return (ISC_R_SUCCESS);
	}
}

isc_result_t
named_checknames_get(const cfg_obj_t **maps, const char *which,
		     const cfg_obj_t **obj)
{
	const cfg_listelt_t *element;
	const cfg_obj_t *checknames;
	const cfg_obj_t *type;
	const cfg_obj_t *value;
	int i;

	for (i = 0;; i++) {
		if (maps[i] == NULL)
			return (ISC_R_NOTFOUND);
		checknames = NULL;
		if (cfg_map_get(maps[i], "check-names",
				&checknames) == ISC_R_SUCCESS) {
			/*
			 * Zone map entry is not a list.
			 */
			if (checknames != NULL && !cfg_obj_islist(checknames)) {
				*obj = checknames;
				return (ISC_R_SUCCESS);
			}
			for (element = cfg_list_first(checknames);
			     element != NULL;
			     element = cfg_list_next(element)) {
				value = cfg_listelt_value(element);
				type = cfg_tuple_get(value, "type");
				if (strcasecmp(cfg_obj_asstring(type),
					       which) == 0) {
					*obj = cfg_tuple_get(value, "mode");
					return (ISC_R_SUCCESS);
				}
			}

		}
	}
}

int
named_config_listcount(const cfg_obj_t *list) {
	const cfg_listelt_t *e;
	int i = 0;

	for (e = cfg_list_first(list); e != NULL; e = cfg_list_next(e))
		i++;

	return (i);
}

isc_result_t
named_config_getclass(const cfg_obj_t *classobj, dns_rdataclass_t defclass,
		      dns_rdataclass_t *classp)
{
	isc_textregion_t r;
	isc_result_t result;

	if (!cfg_obj_isstring(classobj)) {
		*classp = defclass;
		return (ISC_R_SUCCESS);
	}
	DE_CONST(cfg_obj_asstring(classobj), r.base);
	r.length = strlen(r.base);
	result = dns_rdataclass_fromtext(classp, &r);
	if (result != ISC_R_SUCCESS)
		cfg_obj_log(classobj, named_g_lctx, ISC_LOG_ERROR,
			    "unknown class '%s'", r.base);
	return (result);
}

isc_result_t
named_config_gettype(const cfg_obj_t *typeobj, dns_rdatatype_t deftype,
		     dns_rdatatype_t *typep)
{
	isc_textregion_t r;
	isc_result_t result;

	if (!cfg_obj_isstring(typeobj)) {
		*typep = deftype;
		return (ISC_R_SUCCESS);
	}
	DE_CONST(cfg_obj_asstring(typeobj), r.base);
	r.length = strlen(r.base);
	result = dns_rdatatype_fromtext(typep, &r);
	if (result != ISC_R_SUCCESS)
		cfg_obj_log(typeobj, named_g_lctx, ISC_LOG_ERROR,
			    "unknown type '%s'", r.base);
	return (result);
}

dns_zonetype_t
named_config_getzonetype(const cfg_obj_t *zonetypeobj) {
	dns_zonetype_t ztype = dns_zone_none;
	const char *str;

	str = cfg_obj_asstring(zonetypeobj);
	if (strcasecmp(str, "primary") == 0 ||
	    strcasecmp(str, "master") == 0)
	{
		ztype = dns_zone_master;
	} else if (strcasecmp(str, "secondary") == 0 ||
		   strcasecmp(str, "slave") == 0)
	{
		ztype = dns_zone_slave;
	} else if (strcasecmp(str, "mirror") == 0) {
		ztype = dns_zone_mirror;
	} else if (strcasecmp(str, "stub") == 0) {
		ztype = dns_zone_stub;
	} else if (strcasecmp(str, "static-stub") == 0) {
		ztype = dns_zone_staticstub;
	} else if (strcasecmp(str, "redirect") == 0) {
		ztype = dns_zone_redirect;
	} else {
		INSIST(0);
		ISC_UNREACHABLE();
	}
	return (ztype);
}

isc_result_t
named_config_getiplist(const cfg_obj_t *config, const cfg_obj_t *list,
		    in_port_t defport, isc_mem_t *mctx,
		    isc_sockaddr_t **addrsp, isc_dscp_t **dscpsp,
		    uint32_t *countp)
{
	int count, i = 0;
	const cfg_obj_t *addrlist;
	const cfg_obj_t *portobj, *dscpobj;
	const cfg_listelt_t *element;
	isc_sockaddr_t *addrs;
	in_port_t port;
	isc_dscp_t dscp = -1, *dscps = NULL;
	isc_result_t result;

	INSIST(addrsp != NULL && *addrsp == NULL);
	INSIST(dscpsp == NULL || *dscpsp == NULL);
	INSIST(countp != NULL);

	addrlist = cfg_tuple_get(list, "addresses");
	count = named_config_listcount(addrlist);

	portobj = cfg_tuple_get(list, "port");
	if (cfg_obj_isuint32(portobj)) {
		uint32_t val = cfg_obj_asuint32(portobj);
		if (val > UINT16_MAX) {
			cfg_obj_log(portobj, named_g_lctx, ISC_LOG_ERROR,
				    "port '%u' out of range", val);
			return (ISC_R_RANGE);
		}
		port = (in_port_t) val;
	} else if (defport != 0)
		port = defport;
	else {
		result = named_config_getport(config, &port);
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	if (dscpsp != NULL) {
		dscpobj = cfg_tuple_get(list, "dscp");
		if (dscpobj != NULL && cfg_obj_isuint32(dscpobj)) {
			if (cfg_obj_asuint32(dscpobj) > 63) {
				cfg_obj_log(dscpobj, named_g_lctx,
					    ISC_LOG_ERROR,
					    "dscp value '%u' is out of range",
					    cfg_obj_asuint32(dscpobj));
				return (ISC_R_RANGE);
			}
			dscp = (isc_dscp_t)cfg_obj_asuint32(dscpobj);
		}

		dscps = isc_mem_get(mctx, count * sizeof(isc_dscp_t));
		if (dscps == NULL)
			return (ISC_R_NOMEMORY);
	}

	addrs = isc_mem_get(mctx, count * sizeof(isc_sockaddr_t));
	if (addrs == NULL) {
		if (dscps != NULL)
			isc_mem_put(mctx, dscps, count * sizeof(isc_dscp_t));
		return (ISC_R_NOMEMORY);
	}

	for (element = cfg_list_first(addrlist);
	     element != NULL;
	     element = cfg_list_next(element), i++)
	{
		const cfg_obj_t *addr;
		INSIST(i < count);
		addr = cfg_listelt_value(element);
		addrs[i] = *cfg_obj_assockaddr(addr);
		if (dscpsp != NULL) {
			isc_dscp_t innerdscp;
			innerdscp = cfg_obj_getdscp(addr);
			if (innerdscp == -1)
				innerdscp = dscp;
			dscps[i] = innerdscp;
		}
		if (isc_sockaddr_getport(&addrs[i]) == 0)
			isc_sockaddr_setport(&addrs[i], port);
	}
	INSIST(i == count);

	*addrsp = addrs;
	*countp = count;

	if (dscpsp != NULL)
		*dscpsp = dscps;

	return (ISC_R_SUCCESS);
}

void
named_config_putiplist(isc_mem_t *mctx, isc_sockaddr_t **addrsp,
		       isc_dscp_t **dscpsp, uint32_t count)
{
	INSIST(addrsp != NULL && *addrsp != NULL);
	INSIST(dscpsp == NULL || *dscpsp != NULL);

	isc_mem_put(mctx, *addrsp, count * sizeof(isc_sockaddr_t));
	*addrsp = NULL;

	if (dscpsp != NULL) {
		isc_mem_put(mctx, *dscpsp, count * sizeof(isc_dscp_t));
		*dscpsp = NULL;
	}
}

isc_result_t
named_config_getmastersdef(const cfg_obj_t *cctx, const char *name,
			   const cfg_obj_t **ret)
{
	isc_result_t result;
	const cfg_obj_t *masters = NULL;
	const cfg_listelt_t *elt;

	result = cfg_map_get(cctx, "masters", &masters);
	if (result != ISC_R_SUCCESS)
		return (result);
	for (elt = cfg_list_first(masters);
	     elt != NULL;
	     elt = cfg_list_next(elt)) {
		const cfg_obj_t *list;
		const char *listname;

		list = cfg_listelt_value(elt);
		listname = cfg_obj_asstring(cfg_tuple_get(list, "name"));

		if (strcasecmp(listname, name) == 0) {
			*ret = list;
			return (ISC_R_SUCCESS);
		}
	}
	return (ISC_R_NOTFOUND);
}

isc_result_t
named_config_getipandkeylist(const cfg_obj_t *config, const cfg_obj_t *list,
			  isc_mem_t *mctx, dns_ipkeylist_t *ipkl)
{
	uint32_t addrcount = 0, dscpcount = 0, keycount = 0, i = 0;
	uint32_t listcount = 0, l = 0, j;
	uint32_t stackcount = 0, pushed = 0;
	isc_result_t result;
	const cfg_listelt_t *element;
	const cfg_obj_t *addrlist;
	const cfg_obj_t *portobj;
	const cfg_obj_t *dscpobj;
	in_port_t port;
	isc_dscp_t dscp = -1;
	dns_fixedname_t fname;
	isc_sockaddr_t *addrs = NULL;
	isc_dscp_t *dscps = NULL;
	dns_name_t **keys = NULL;
	struct { const char *name; } *lists = NULL;
	struct {
		const cfg_listelt_t *element;
		in_port_t port;
		isc_dscp_t dscp;
	} *stack = NULL;

	REQUIRE(ipkl != NULL);
	REQUIRE(ipkl->count == 0);
	REQUIRE(ipkl->addrs == NULL);
	REQUIRE(ipkl->keys == NULL);
	REQUIRE(ipkl->dscps == NULL);
	REQUIRE(ipkl->labels == NULL);
	REQUIRE(ipkl->allocated == 0);

	/*
	 * Get system defaults.
	 */
	result = named_config_getport(config, &port);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = named_config_getdscp(config, &dscp);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

 newlist:
	addrlist = cfg_tuple_get(list, "addresses");
	portobj = cfg_tuple_get(list, "port");
	dscpobj = cfg_tuple_get(list, "dscp");

	if (cfg_obj_isuint32(portobj)) {
		uint32_t val = cfg_obj_asuint32(portobj);
		if (val > UINT16_MAX) {
			cfg_obj_log(portobj, named_g_lctx, ISC_LOG_ERROR,
				    "port '%u' out of range", val);
			result = ISC_R_RANGE;
			goto cleanup;
		}
		port = (in_port_t) val;
	}

	if (dscpobj != NULL && cfg_obj_isuint32(dscpobj)) {
		if (cfg_obj_asuint32(dscpobj) > 63) {
			cfg_obj_log(dscpobj, named_g_lctx, ISC_LOG_ERROR,
				    "dscp value '%u' is out of range",
				    cfg_obj_asuint32(dscpobj));
			result = ISC_R_RANGE;
			goto cleanup;
		}
		dscp = (isc_dscp_t)cfg_obj_asuint32(dscpobj);
	}

	result = ISC_R_NOMEMORY;

	element = cfg_list_first(addrlist);
 resume:
	for ( ;
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *addr;
		const cfg_obj_t *key;
		const char *keystr;
		isc_buffer_t b;

		addr = cfg_tuple_get(cfg_listelt_value(element),
				     "masterselement");
		key = cfg_tuple_get(cfg_listelt_value(element), "key");

		if (!cfg_obj_issockaddr(addr)) {
			const char *listname = cfg_obj_asstring(addr);
			isc_result_t tresult;

			/* Grow lists? */
			if (listcount == l) {
				void * tmp;
				uint32_t newlen = listcount + 16;
				size_t newsize, oldsize;

				newsize = newlen * sizeof(*lists);
				oldsize = listcount * sizeof(*lists);
				tmp = isc_mem_get(mctx, newsize);
				if (tmp == NULL)
					goto cleanup;
				if (listcount != 0) {
					memmove(tmp, lists, oldsize);
					isc_mem_put(mctx, lists, oldsize);
				}
				lists = tmp;
				listcount = newlen;
			}
			/* Seen? */
			for (j = 0; j < l; j++)
				if (strcasecmp(lists[j].name, listname) == 0)
					break;
			if (j < l)
				continue;
			tresult = named_config_getmastersdef(config, listname,
							     &list);
			if (tresult == ISC_R_NOTFOUND) {
				cfg_obj_log(addr, named_g_lctx, ISC_LOG_ERROR,
				    "masters \"%s\" not found", listname);

				result = tresult;
				goto cleanup;
			}
			if (tresult != ISC_R_SUCCESS)
				goto cleanup;
			lists[l++].name = listname;
			/* Grow stack? */
			if (stackcount == pushed) {
				void * tmp;
				uint32_t newlen = stackcount + 16;
				size_t newsize, oldsize;

				newsize = newlen * sizeof(*stack);
				oldsize = stackcount * sizeof(*stack);
				tmp = isc_mem_get(mctx, newsize);
				if (tmp == NULL)
					goto cleanup;
				if (stackcount != 0) {
					memmove(tmp, stack, oldsize);
					isc_mem_put(mctx, stack, oldsize);
				}
				stack = tmp;
				stackcount = newlen;
			}
			/*
			 * We want to resume processing this list on the
			 * next element.
			 */
			stack[pushed].element = cfg_list_next(element);
			stack[pushed].port = port;
			stack[pushed].dscp = dscp;
			pushed++;
			goto newlist;
		}

		if (i == addrcount) {
			void * tmp;
			uint32_t newlen = addrcount + 16;
			size_t newsize, oldsize;

			newsize = newlen * sizeof(isc_sockaddr_t);
			oldsize = addrcount * sizeof(isc_sockaddr_t);
			tmp = isc_mem_get(mctx, newsize);
			if (tmp == NULL)
				goto cleanup;
			if (addrcount != 0) {
				memmove(tmp, addrs, oldsize);
				isc_mem_put(mctx, addrs, oldsize);
			}
			addrs = tmp;
			addrcount = newlen;

			newsize = newlen * sizeof(isc_dscp_t);
			oldsize = dscpcount * sizeof(isc_dscp_t);
			tmp = isc_mem_get(mctx, newsize);
			if (tmp == NULL)
				goto cleanup;
			if (dscpcount != 0) {
				memmove(tmp, dscps, oldsize);
				isc_mem_put(mctx, dscps, oldsize);
			}
			dscps = tmp;
			dscpcount = newlen;

			newsize = newlen * sizeof(dns_name_t *);
			oldsize = keycount * sizeof(dns_name_t *);
			tmp = isc_mem_get(mctx, newsize);
			if (tmp == NULL)
				goto cleanup;
			if (keycount != 0) {
				memmove(tmp, keys, oldsize);
				isc_mem_put(mctx, keys, oldsize);
			}
			keys = tmp;
			keycount = newlen;
		}

		addrs[i] = *cfg_obj_assockaddr(addr);
		if (isc_sockaddr_getport(&addrs[i]) == 0)
			isc_sockaddr_setport(&addrs[i], port);
		dscps[i] = cfg_obj_getdscp(addr);
		if (dscps[i] == -1)
			dscps[i] = dscp;
		keys[i] = NULL;
		i++;	/* Increment here so that cleanup on error works. */
		if (!cfg_obj_isstring(key))
			continue;
		keys[i - 1] = isc_mem_get(mctx, sizeof(dns_name_t));
		if (keys[i - 1] == NULL)
			goto cleanup;
		dns_name_init(keys[i - 1], NULL);

		keystr = cfg_obj_asstring(key);
		isc_buffer_constinit(&b, keystr, strlen(keystr));
		isc_buffer_add(&b, strlen(keystr));
		dns_fixedname_init(&fname);
		result = dns_name_fromtext(dns_fixedname_name(&fname), &b,
					   dns_rootname, 0, NULL);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		result = dns_name_dup(dns_fixedname_name(&fname), mctx,
				      keys[i - 1]);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
	}
	if (pushed != 0) {
		pushed--;
		element = stack[pushed].element;
		port = stack[pushed].port;
		dscp = stack[pushed].dscp;
		goto resume;
	}
	if (i < addrcount) {
		void * tmp;
		size_t newsize, oldsize;

		newsize = i * sizeof(isc_sockaddr_t);
		oldsize = addrcount * sizeof(isc_sockaddr_t);
		if (i != 0) {
			tmp = isc_mem_get(mctx, newsize);
			if (tmp == NULL)
				goto cleanup;
			memmove(tmp, addrs, newsize);
		} else
			tmp = NULL;
		isc_mem_put(mctx, addrs, oldsize);
		addrs = tmp;
		addrcount = i;

		newsize = i * sizeof(isc_dscp_t);
		oldsize = dscpcount * sizeof(isc_dscp_t);
		if (i != 0) {
			tmp = isc_mem_get(mctx, newsize);
			if (tmp == NULL)
				goto cleanup;
			memmove(tmp, dscps, newsize);
		} else
			tmp = NULL;
		isc_mem_put(mctx, dscps, oldsize);
		dscps = tmp;
		dscpcount = i;

		newsize = i * sizeof(dns_name_t *);
		oldsize = keycount * sizeof(dns_name_t *);
		if (i != 0) {
			tmp = isc_mem_get(mctx, newsize);
			if (tmp == NULL)
				goto cleanup;
			memmove(tmp, keys,  newsize);
		} else
			tmp = NULL;
		isc_mem_put(mctx, keys, oldsize);
		keys = tmp;
		keycount = i;
	}

	if (lists != NULL)
		isc_mem_put(mctx, lists, listcount * sizeof(*lists));
	if (stack != NULL)
		isc_mem_put(mctx, stack, stackcount * sizeof(*stack));

	INSIST(keycount == addrcount);

	ipkl->addrs = addrs;
	ipkl->dscps = dscps;
	ipkl->keys = keys;
	ipkl->count = addrcount;
	ipkl->allocated = addrcount;

	return (ISC_R_SUCCESS);

 cleanup:
	if (addrs != NULL)
		isc_mem_put(mctx, addrs, addrcount * sizeof(isc_sockaddr_t));
	if (dscps != NULL)
		isc_mem_put(mctx, dscps, dscpcount * sizeof(isc_dscp_t));
	if (keys != NULL) {
		for (j = 0; j < i; j++) {
			if (keys[j] == NULL)
				continue;
			if (dns_name_dynamic(keys[j]))
				dns_name_free(keys[j], mctx);
			isc_mem_put(mctx, keys[j], sizeof(dns_name_t));
		}
		isc_mem_put(mctx, keys, keycount * sizeof(dns_name_t *));
	}
	if (lists != NULL)
		isc_mem_put(mctx, lists, listcount * sizeof(*lists));
	if (stack != NULL)
		isc_mem_put(mctx, stack, stackcount * sizeof(*stack));
	return (result);
}

isc_result_t
named_config_getport(const cfg_obj_t *config, in_port_t *portp) {
	const cfg_obj_t *maps[3];
	const cfg_obj_t *options = NULL;
	const cfg_obj_t *portobj = NULL;
	isc_result_t result;
	int i;

	(void)cfg_map_get(config, "options", &options);
	i = 0;
	if (options != NULL)
		maps[i++] = options;
	maps[i++] = named_g_defaults;
	maps[i] = NULL;

	result = named_config_get(maps, "port", &portobj);
	INSIST(result == ISC_R_SUCCESS);
	if (cfg_obj_asuint32(portobj) >= UINT16_MAX) {
		cfg_obj_log(portobj, named_g_lctx, ISC_LOG_ERROR,
			    "port '%u' out of range",
			    cfg_obj_asuint32(portobj));
		return (ISC_R_RANGE);
	}
	*portp = (in_port_t)cfg_obj_asuint32(portobj);
	return (ISC_R_SUCCESS);
}

isc_result_t
named_config_getdscp(const cfg_obj_t *config, isc_dscp_t *dscpp) {
	const cfg_obj_t *options = NULL;
	const cfg_obj_t *dscpobj = NULL;
	isc_result_t result;

	(void)cfg_map_get(config, "options", &options);
	if (options == NULL)
		return (ISC_R_SUCCESS);

	result = cfg_map_get(options, "dscp", &dscpobj);
	if (result != ISC_R_SUCCESS || dscpobj == NULL) {
		*dscpp = -1;
		return (ISC_R_SUCCESS);
	}
	if (cfg_obj_asuint32(dscpobj) >= 64) {
		cfg_obj_log(dscpobj, named_g_lctx, ISC_LOG_ERROR,
			    "dscp '%u' out of range",
			    cfg_obj_asuint32(dscpobj));
		return (ISC_R_RANGE);
	}
	*dscpp = (isc_dscp_t)cfg_obj_asuint32(dscpobj);
	return (ISC_R_SUCCESS);
}

struct keyalgorithms {
	const char *str;
	enum { hmacnone, hmacmd5, hmacsha1, hmacsha224,
	       hmacsha256, hmacsha384, hmacsha512 } hmac;
	unsigned int type;
	uint16_t size;
} algorithms[] = {
	{ "hmac-md5", hmacmd5, DST_ALG_HMACMD5, 128 },
	{ "hmac-md5.sig-alg.reg.int", hmacmd5, DST_ALG_HMACMD5, 0 },
	{ "hmac-md5.sig-alg.reg.int.", hmacmd5, DST_ALG_HMACMD5, 0 },
	{ "hmac-sha1", hmacsha1, DST_ALG_HMACSHA1, 160 },
	{ "hmac-sha224", hmacsha224, DST_ALG_HMACSHA224, 224 },
	{ "hmac-sha256", hmacsha256, DST_ALG_HMACSHA256, 256 },
	{ "hmac-sha384", hmacsha384, DST_ALG_HMACSHA384, 384 },
	{ "hmac-sha512", hmacsha512, DST_ALG_HMACSHA512, 512 },
	{  NULL, hmacnone, DST_ALG_UNKNOWN, 0 }
};

isc_result_t
named_config_getkeyalgorithm(const char *str, const dns_name_t **name,
			     uint16_t *digestbits)
{
	return (named_config_getkeyalgorithm2(str, name, NULL, digestbits));
}

isc_result_t
named_config_getkeyalgorithm2(const char *str, const dns_name_t **name,
			      unsigned int *typep, uint16_t *digestbits)
{
	int i;
	size_t len = 0;
	uint16_t bits;
	isc_result_t result;

	for (i = 0; algorithms[i].str != NULL; i++) {
		len = strlen(algorithms[i].str);
		if (strncasecmp(algorithms[i].str, str, len) == 0 &&
		    (str[len] == '\0' ||
		     (algorithms[i].size != 0 && str[len] == '-')))
			break;
	}
	if (algorithms[i].str == NULL)
		return (ISC_R_NOTFOUND);
	if (str[len] == '-') {
		result = isc_parse_uint16(&bits, str + len + 1, 10);
		if (result != ISC_R_SUCCESS)
			return (result);
		if (bits > algorithms[i].size)
			return (ISC_R_RANGE);
	} else if (algorithms[i].size == 0)
		bits = 128;
	else
		bits = algorithms[i].size;

	if (name != NULL) {
		switch (algorithms[i].hmac) {
		case hmacmd5: *name = dns_tsig_hmacmd5_name; break;
		case hmacsha1: *name = dns_tsig_hmacsha1_name; break;
		case hmacsha224: *name = dns_tsig_hmacsha224_name; break;
		case hmacsha256: *name = dns_tsig_hmacsha256_name; break;
		case hmacsha384: *name = dns_tsig_hmacsha384_name; break;
		case hmacsha512: *name = dns_tsig_hmacsha512_name; break;
		default:
			INSIST(0);
			ISC_UNREACHABLE();
		}
	}
	if (typep != NULL)
		*typep = algorithms[i].type;
	if (digestbits != NULL)
		*digestbits = bits;
	return (ISC_R_SUCCESS);
}
