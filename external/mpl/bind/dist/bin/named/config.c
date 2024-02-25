/*	$NetBSD: config.c,v 1.13.2.2 2024/02/25 15:43:05 martin Exp $	*/

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

#include <bind.keys.h>
#include <inttypes.h>
#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/parseint.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/kasp.h>
#include <dns/name.h>
#include <dns/rdataclass.h>
#include <dns/rdatatype.h>
#include <dns/tsig.h>
#include <dns/zone.h>

#include <dst/dst.h>

#include <isccfg/grammar.h>
#include <isccfg/namedconf.h>

#include <named/config.h>
#include <named/globals.h>

/*% default configuration */
static char defaultconf[] = "\
options {\n\
	answer-cookie true;\n\
	automatic-interface-scan yes;\n\
	bindkeys-file \"" NAMED_SYSCONFDIR "/bind.keys\";\n\
#	blackhole {none;};\n"
			    "	cookie-algorithm siphash24;\n"
			    "	coresize default;\n\
	datasize default;\n"
			    "\
#	directory <none>\n\
	dnssec-policy \"none\";\n\
	dump-file \"named_dump.db\";\n\
	edns-udp-size 1232;\n\
	files unlimited;\n"
#if defined(HAVE_GEOIP2)
			    "\
	geoip-directory \"" MAXMINDDB_PREFIX "/share/GeoIP\";\n"
#elif defined(HAVE_GEOIP2)
			    "\
	geoip-directory \".\";\n"
#endif /* if defined(HAVE_GEOIP2) */
			    "\
	heartbeat-interval 60;\n\
	interface-interval 60;\n\
#	keep-response-order {none;};\n\
	listen-on {any;};\n\
	listen-on-v6 {any;};\n\
#	lock-file \"" NAMED_LOCALSTATEDIR "/run/named/named.lock\";\n\
	match-mapped-addresses no;\n\
	max-ixfr-ratio 100%;\n\
	max-rsa-exponent-size 0; /* no limit */\n\
	max-udp-size 1232;\n\
	memstatistics-file \"named.memstats\";\n\
	nocookie-udp-size 4096;\n\
	notify-rate 20;\n\
	nta-lifetime 3600;\n\
	nta-recheck 300;\n\
#	pid-file \"" NAMED_LOCALSTATEDIR "/run/named/named.pid\"; \n\
	port 53;\n"
#if HAVE_SO_REUSEPORT_LB
			    "\
	reuseport yes;\n"
#else
			    "\
	reuseport no;\n"
#endif
			    "\
	tls-port 853;\n"
#if HAVE_LIBNGHTTP2
			    "\
	http-port 80;\n\
	https-port 443;\n\
	http-listener-clients 300;\n\
	http-streams-per-connection 100;\n"
#endif
			    "\
	prefetch 2 9;\n\
	recursing-file \"named.recursing\";\n\
	recursive-clients 1000;\n\
	request-nsid false;\n\
	reserved-sockets 512;\n\
	resolver-query-timeout 10;\n\
	rrset-order { order random; };\n\
	secroots-file \"named.secroots\";\n\
	send-cookie true;\n\
	serial-query-rate 20;\n\
	server-id none;\n\
	session-keyalg hmac-sha256;\n\
#	session-keyfile \"" NAMED_LOCALSTATEDIR "/run/named/session.key\";\n\
	session-keyname local-ddns;\n\
	stacksize default;\n\
	startup-notify-rate 20;\n\
	statistics-file \"named.stats\";\n\
	tcp-advertised-timeout 300;\n\
	tcp-clients 150;\n\
	tcp-idle-timeout 300;\n\
	tcp-initial-timeout 300;\n\
	tcp-keepalive-timeout 300;\n\
	tcp-listen-queue 10;\n\
	tcp-receive-buffer 0;\n\
	tcp-send-buffer 0;\n\
#	tkey-dhkey <none>\n\
#	tkey-domain <none>\n\
#	tkey-gssapi-credential <none>\n\
	transfer-message-size 20480;\n\
	transfers-in 10;\n\
	transfers-out 10;\n\
	transfers-per-ns 2;\n\
	trust-anchor-telemetry yes;\n\
	udp-receive-buffer 0;\n\
	udp-send-buffer 0;\n\
	update-quota 100;\n\
\n\
	/* view */\n\
	allow-new-zones no;\n\
	allow-notify {none;};\n\
	allow-query-cache { localnets; localhost; };\n\
	allow-query-cache-on { any; };\n\
	allow-recursion { localnets; localhost; };\n\
	allow-recursion-on { any; };\n\
	allow-update-forwarding {none;};\n\
	auth-nxdomain false;\n\
	check-dup-records warn;\n\
	check-mx warn;\n\
	check-names primary fail;\n\
	check-names response ignore;\n\
	check-names secondary warn;\n\
	check-spf warn;\n\
	clients-per-query 10;\n\
	dnssec-accept-expired no;\n\
	dnssec-validation " VALIDATION_DEFAULT "; \n"
#ifdef HAVE_DNSTAP
			    "	dnstap-identity hostname;\n"
#endif /* ifdef HAVE_DNSTAP */
			    "\
	fetch-quota-params 100 0.1 0.3 0.7;\n\
	fetches-per-server 0;\n\
	fetches-per-zone 0;\n\
	glue-cache yes;\n\
	lame-ttl 0;\n"
#ifdef HAVE_LMDB
			    "	lmdb-mapsize 32M;\n"
#endif /* ifdef HAVE_LMDB */
			    "	max-cache-size 90%;\n\
	max-cache-ttl 604800; /* 1 week */\n\
	max-clients-per-query 100;\n\
	max-ncache-ttl 10800; /* 3 hours */\n\
	max-recursion-depth 7;\n\
	max-recursion-queries 100;\n\
	max-stale-ttl 86400; /* 1 day */\n\
	message-compression yes;\n\
	min-ncache-ttl 0; /* 0 hours */\n\
	min-cache-ttl 0; /* 0 seconds */\n\
	minimal-any false;\n\
	minimal-responses no-auth-recursive;\n\
	notify-source *;\n\
	notify-source-v6 *;\n\
	nsec3-test-zone no;\n\
	parental-source *;\n\
	parental-source-v6 *;\n\
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
	root-key-sentinel yes;\n\
	servfail-ttl 1;\n\
#	sortlist <none>\n\
	stale-answer-client-timeout off;\n\
	stale-answer-enable false;\n\
	stale-answer-ttl 30; /* 30 seconds */\n\
	stale-cache-enable false;\n\
	stale-refresh-time 30; /* 30 seconds */\n\
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
	dnssec-dnskey-kskonly yes;\n\
	dnssec-loadkeys-interval 60;\n\
	dnssec-secure-to-insecure no;\n\
	dnssec-update-mode maintain;\n\
#	forward <none>\n\
#	forwarders <none>\n\
#	inline-signing no;\n\
	ixfr-from-differences false;\n\
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
	max-cache-size 2M;\n\
\n\
	# Prevent use of this zone in DNS amplified reflection DoS attacks\n\
	rate-limit {\n\
		responses-per-second 3;\n\
		slip 0;\n\
		min-table-size 10;\n\
	};\n\
\n\
	zone \"version.bind\" chaos {\n\
		type primary;\n\
		database \"_builtin version\";\n\
	};\n\
\n\
	zone \"hostname.bind\" chaos {\n\
		type primary;\n\
		database \"_builtin hostname\";\n\
	};\n\
\n\
	zone \"authors.bind\" chaos {\n\
		type primary;\n\
		database \"_builtin authors\";\n\
	};\n\
\n\
	zone \"id.server\" chaos {\n\
		type primary;\n\
		database \"_builtin id\";\n\
	};\n\
};\n\
"
			    "#\n\
#  Built-in DNSSEC key and signing policies.\n\
#\n\
dnssec-policy \"default\" {\n\
	keys {\n\
		csk key-directory lifetime unlimited algorithm 13;\n\
	};\n\
\n\
	dnskey-ttl " DNS_KASP_KEY_TTL ";\n\
	publish-safety " DNS_KASP_PUBLISH_SAFETY "; \n\
	retire-safety " DNS_KASP_RETIRE_SAFETY "; \n\
	purge-keys " DNS_KASP_PURGE_KEYS "; \n\
	signatures-refresh " DNS_KASP_SIG_REFRESH "; \n\
	signatures-validity " DNS_KASP_SIG_VALIDITY "; \n\
	signatures-validity-dnskey " DNS_KASP_SIG_VALIDITY_DNSKEY "; \n\
	max-zone-ttl " DNS_KASP_ZONE_MAXTTL "; \n\
	zone-propagation-delay " DNS_KASP_ZONE_PROPDELAY "; \n\
	parent-ds-ttl " DNS_KASP_DS_TTL "; \n\
	parent-propagation-delay " DNS_KASP_PARENT_PROPDELAY "; \n\
};\n\
\n\
dnssec-policy \"insecure\" {\n\
	max-zone-ttl 0; \n\
	keys { };\n\
};\n\
\n\
"
			    "#\n\
#  Default trusted key(s), used if \n\
# \"dnssec-validation auto;\" is set and\n\
#  " NAMED_SYSCONFDIR "/bind.keys doesn't exist).\n\
#\n\
# BEGIN TRUST ANCHORS\n"

	/* Imported from bind.keys.h: */
	TRUST_ANCHORS

			    "# END TRUST ANCHORS\n\
\n\
primaries " DEFAULT_IANA_ROOT_ZONE_PRIMARIES " {\n\
	2801:1b8:10::b;		# b.root-servers.net\n\
	2001:500:2::c;		# c.root-servers.net\n\
	2001:500:2f::f;		# f.root-servers.net\n\
	2001:500:12::d0d;	# g.root-servers.net\n\
	2001:7fd::1;		# k.root-servers.net\n\
	2620:0:2830:202::132;	# xfr.cjr.dns.icann.org\n\
	2620:0:2d0:202::132;	# xfr.lax.dns.icann.org\n\
	170.247.170.2;		# b.root-servers.net\n\
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
	return (cfg_parse_buffer(parser, &b, __FILE__, 0, &cfg_type_namedconf,
				 CFG_PCTX_NODEPRECATED, conf));
}

const char *
named_config_getdefault(void) {
	return (defaultconf);
}

isc_result_t
named_config_get(cfg_obj_t const *const *maps, const char *name,
		 const cfg_obj_t **obj) {
	int i;

	for (i = 0; maps[i] != NULL; i++) {
		if (cfg_map_get(maps[i], name, obj) == ISC_R_SUCCESS) {
			return (ISC_R_SUCCESS);
		}
	}
	return (ISC_R_NOTFOUND);
}

isc_result_t
named_checknames_get(const cfg_obj_t **maps, const char *const names[],
		     const cfg_obj_t **obj) {
	const cfg_listelt_t *element;
	const cfg_obj_t *checknames;
	const cfg_obj_t *type;
	const cfg_obj_t *value;
	int i;

	REQUIRE(maps != NULL);
	REQUIRE(names != NULL);
	REQUIRE(obj != NULL && *obj == NULL);

	for (i = 0; maps[i] != NULL; i++) {
		checknames = NULL;
		if (cfg_map_get(maps[i], "check-names", &checknames) ==
		    ISC_R_SUCCESS)
		{
			/*
			 * Zone map entry is not a list.
			 */
			if (checknames != NULL && !cfg_obj_islist(checknames)) {
				*obj = checknames;
				return (ISC_R_SUCCESS);
			}
			for (element = cfg_list_first(checknames);
			     element != NULL; element = cfg_list_next(element))
			{
				value = cfg_listelt_value(element);
				type = cfg_tuple_get(value, "type");

				for (size_t j = 0; names[j] != NULL; j++) {
					if (strcasecmp(cfg_obj_asstring(type),
						       names[j]) == 0)
					{
						*obj = cfg_tuple_get(value,
								     "mode");
						return (ISC_R_SUCCESS);
					}
				}
			}
		}
	}
	return (ISC_R_NOTFOUND);
}

int
named_config_listcount(const cfg_obj_t *list) {
	const cfg_listelt_t *e;
	int i = 0;

	for (e = cfg_list_first(list); e != NULL; e = cfg_list_next(e)) {
		i++;
	}

	return (i);
}

isc_result_t
named_config_getclass(const cfg_obj_t *classobj, dns_rdataclass_t defclass,
		      dns_rdataclass_t *classp) {
	isc_textregion_t r;
	isc_result_t result;

	if (!cfg_obj_isstring(classobj)) {
		*classp = defclass;
		return (ISC_R_SUCCESS);
	}
	DE_CONST(cfg_obj_asstring(classobj), r.base);
	r.length = strlen(r.base);
	result = dns_rdataclass_fromtext(classp, &r);
	if (result != ISC_R_SUCCESS) {
		cfg_obj_log(classobj, named_g_lctx, ISC_LOG_ERROR,
			    "unknown class '%s'", r.base);
	}
	return (result);
}

isc_result_t
named_config_gettype(const cfg_obj_t *typeobj, dns_rdatatype_t deftype,
		     dns_rdatatype_t *typep) {
	isc_textregion_t r;
	isc_result_t result;

	if (!cfg_obj_isstring(typeobj)) {
		*typep = deftype;
		return (ISC_R_SUCCESS);
	}
	DE_CONST(cfg_obj_asstring(typeobj), r.base);
	r.length = strlen(r.base);
	result = dns_rdatatype_fromtext(typep, &r);
	if (result != ISC_R_SUCCESS) {
		cfg_obj_log(typeobj, named_g_lctx, ISC_LOG_ERROR,
			    "unknown type '%s'", r.base);
	}
	return (result);
}

dns_zonetype_t
named_config_getzonetype(const cfg_obj_t *zonetypeobj) {
	dns_zonetype_t ztype = dns_zone_none;
	const char *str;

	str = cfg_obj_asstring(zonetypeobj);
	if (strcasecmp(str, "primary") == 0 || strcasecmp(str, "master") == 0) {
		ztype = dns_zone_primary;
	} else if (strcasecmp(str, "secondary") == 0 ||
		   strcasecmp(str, "slave") == 0)
	{
		ztype = dns_zone_secondary;
	} else if (strcasecmp(str, "mirror") == 0) {
		ztype = dns_zone_mirror;
	} else if (strcasecmp(str, "stub") == 0) {
		ztype = dns_zone_stub;
	} else if (strcasecmp(str, "static-stub") == 0) {
		ztype = dns_zone_staticstub;
	} else if (strcasecmp(str, "redirect") == 0) {
		ztype = dns_zone_redirect;
	} else {
		UNREACHABLE();
	}
	return (ztype);
}

isc_result_t
named_config_getiplist(const cfg_obj_t *config, const cfg_obj_t *list,
		       in_port_t defport, isc_mem_t *mctx,
		       isc_sockaddr_t **addrsp, uint32_t *countp) {
	int count, i = 0;
	const cfg_obj_t *addrlist = NULL;
	const cfg_obj_t *portobj = NULL;
	const cfg_listelt_t *element = NULL;
	isc_sockaddr_t *addrs = NULL;
	in_port_t port;
	isc_result_t result;

	INSIST(addrsp != NULL && *addrsp == NULL);
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
		port = (in_port_t)val;
	} else if (defport != 0) {
		port = defport;
	} else {
		result = named_config_getport(config, "port", &port);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}

	addrs = isc_mem_get(mctx, count * sizeof(isc_sockaddr_t));

	for (element = cfg_list_first(addrlist); element != NULL;
	     element = cfg_list_next(element), i++)
	{
		const cfg_obj_t *addr;
		INSIST(i < count);
		addr = cfg_listelt_value(element);
		addrs[i] = *cfg_obj_assockaddr(addr);
		if (isc_sockaddr_getport(&addrs[i]) == 0) {
			isc_sockaddr_setport(&addrs[i], port);
		}
	}
	INSIST(i == count);

	*addrsp = addrs;
	*countp = count;

	return (ISC_R_SUCCESS);
}

void
named_config_putiplist(isc_mem_t *mctx, isc_sockaddr_t **addrsp,
		       uint32_t count) {
	INSIST(addrsp != NULL && *addrsp != NULL);

	isc_mem_put(mctx, *addrsp, count * sizeof(isc_sockaddr_t));
	*addrsp = NULL;
}

static isc_result_t
getremotesdef(const cfg_obj_t *cctx, const char *list, const char *name,
	      const cfg_obj_t **ret) {
	isc_result_t result;
	const cfg_obj_t *obj = NULL;
	const cfg_listelt_t *elt;

	REQUIRE(cctx != NULL);
	REQUIRE(name != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	result = cfg_map_get(cctx, list, &obj);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	elt = cfg_list_first(obj);
	while (elt != NULL) {
		obj = cfg_listelt_value(elt);
		if (strcasecmp(cfg_obj_asstring(cfg_tuple_get(obj, "name")),
			       name) == 0)
		{
			*ret = obj;
			return (ISC_R_SUCCESS);
		}
		elt = cfg_list_next(elt);
	}
	return (ISC_R_NOTFOUND);
}

isc_result_t
named_config_getremotesdef(const cfg_obj_t *cctx, const char *list,
			   const char *name, const cfg_obj_t **ret) {
	isc_result_t result;

	if (strcmp(list, "parental-agents") == 0) {
		return (getremotesdef(cctx, list, name, ret));
	} else if (strcmp(list, "primaries") == 0) {
		result = getremotesdef(cctx, list, name, ret);
		if (result != ISC_R_SUCCESS) {
			result = getremotesdef(cctx, "masters", name, ret);
		}
		return (result);
	}
	return (ISC_R_NOTFOUND);
}

static isc_result_t
named_config_getname(isc_mem_t *mctx, const cfg_obj_t *obj,
		     dns_name_t **namep) {
	REQUIRE(namep != NULL && *namep == NULL);

	const char *objstr;
	isc_result_t result;
	isc_buffer_t b;
	dns_fixedname_t fname;

	if (!cfg_obj_isstring(obj)) {
		*namep = NULL;
		return (ISC_R_SUCCESS);
	}

	*namep = isc_mem_get(mctx, sizeof(**namep));
	dns_name_init(*namep, NULL);

	objstr = cfg_obj_asstring(obj);
	isc_buffer_constinit(&b, objstr, strlen(objstr));
	isc_buffer_add(&b, strlen(objstr));
	dns_fixedname_init(&fname);
	result = dns_name_fromtext(dns_fixedname_name(&fname), &b, dns_rootname,
				   0, NULL);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(mctx, *namep, sizeof(**namep));
		*namep = NULL;
		return (result);
	}
	dns_name_dup(dns_fixedname_name(&fname), mctx, *namep);

	return (ISC_R_SUCCESS);
}

#define grow_array(mctx, array, newlen, oldlen)                    \
	if (newlen >= oldlen) {                                    \
		size_t newsize = (newlen + 16) * sizeof(array[0]); \
		size_t oldsize = oldlen * sizeof(array[0]);        \
		void *tmp = isc_mem_get(mctx, newsize);            \
		memset(tmp, 0, newsize);                           \
		if (oldlen != 0) {                                 \
			memmove(tmp, array, oldsize);              \
			isc_mem_put(mctx, array, oldsize);         \
		}                                                  \
		array = tmp;                                       \
		oldlen = newlen + 16;                              \
	}

#define shrink_array(mctx, array, newlen, oldlen)           \
	if (newlen < oldlen) {                              \
		void *tmp = NULL;                           \
		size_t newsize = newlen * sizeof(array[0]); \
		size_t oldsize = oldlen * sizeof(array[0]); \
		if (newlen != 0) {                          \
			tmp = isc_mem_get(mctx, newsize);   \
			memset(tmp, 0, newsize);            \
			memmove(tmp, array, newsize);       \
		} else {                                    \
			tmp = NULL;                         \
		}                                           \
		isc_mem_put(mctx, array, oldsize);          \
		array = tmp;                                \
		oldlen = newlen;                            \
	}

isc_result_t
named_config_getipandkeylist(const cfg_obj_t *config, const char *listtype,
			     const cfg_obj_t *list, isc_mem_t *mctx,
			     dns_ipkeylist_t *ipkl) {
	uint32_t addrcount = 0, keycount = 0, tlscount = 0, i = 0;
	uint32_t listcount = 0, l = 0, j;
	uint32_t stackcount = 0, pushed = 0;
	isc_result_t result;
	const cfg_listelt_t *element;
	const cfg_obj_t *addrlist;
	const cfg_obj_t *portobj;
	in_port_t port = (in_port_t)0;
	in_port_t def_port;
	in_port_t def_tlsport;
	isc_sockaddr_t *addrs = NULL;
	dns_name_t **keys = NULL;
	dns_name_t **tlss = NULL;
	struct {
		const char *name;
		in_port_t port;
		isc_sockaddr_t *src4s;
		isc_sockaddr_t *src6s;
	} *lists = NULL;
	struct {
		const cfg_listelt_t *element;
		in_port_t port;
	} *stack = NULL;

	REQUIRE(ipkl != NULL);
	REQUIRE(ipkl->count == 0);
	REQUIRE(ipkl->addrs == NULL);
	REQUIRE(ipkl->keys == NULL);
	REQUIRE(ipkl->tlss == NULL);
	REQUIRE(ipkl->labels == NULL);
	REQUIRE(ipkl->allocated == 0);

	/*
	 * Get system defaults.
	 */
	result = named_config_getport(config, "port", &def_port);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = named_config_getport(config, "tls-port", &def_tlsport);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

newlist:
	addrlist = cfg_tuple_get(list, "addresses");
	portobj = cfg_tuple_get(list, "port");

	if (cfg_obj_isuint32(portobj)) {
		uint32_t val = cfg_obj_asuint32(portobj);
		if (val > UINT16_MAX) {
			cfg_obj_log(portobj, named_g_lctx, ISC_LOG_ERROR,
				    "port '%u' out of range", val);
			result = ISC_R_RANGE;
			goto cleanup;
		}
		port = (in_port_t)val;
	}

	result = ISC_R_NOMEMORY;

	element = cfg_list_first(addrlist);
resume:
	for (; element != NULL; element = cfg_list_next(element)) {
		const cfg_obj_t *addr;
		const cfg_obj_t *key;
		const cfg_obj_t *tls;

		addr = cfg_tuple_get(cfg_listelt_value(element),
				     "remoteselement");
		key = cfg_tuple_get(cfg_listelt_value(element), "key");
		tls = cfg_tuple_get(cfg_listelt_value(element), "tls");

		if (!cfg_obj_issockaddr(addr)) {
			const char *listname = cfg_obj_asstring(addr);
			isc_result_t tresult;

			/* Grow lists? */
			grow_array(mctx, lists, l, listcount);

			/* Seen? */
			for (j = 0; j < l; j++) {
				if (strcasecmp(lists[j].name, listname) == 0) {
					break;
				}
			}
			if (j < l) {
				continue;
			}
			list = NULL;
			tresult = named_config_getremotesdef(config, listtype,
							     listname, &list);
			if (tresult == ISC_R_NOTFOUND) {
				cfg_obj_log(addr, named_g_lctx, ISC_LOG_ERROR,
					    "%s \"%s\" not found", listtype,
					    listname);

				result = tresult;
				goto cleanup;
			}
			if (tresult != ISC_R_SUCCESS) {
				goto cleanup;
			}
			lists[l++].name = listname;
			/* Grow stack? */
			grow_array(mctx, stack, pushed, stackcount);
			/*
			 * We want to resume processing this list on the
			 * next element.
			 */
			stack[pushed].element = cfg_list_next(element);
			stack[pushed].port = port;
			pushed++;
			goto newlist;
		}

		grow_array(mctx, addrs, i, addrcount);
		grow_array(mctx, keys, i, keycount);
		grow_array(mctx, tlss, i, tlscount);

		addrs[i] = *cfg_obj_assockaddr(addr);

		result = named_config_getname(mctx, key, &keys[i]);
		if (result != ISC_R_SUCCESS) {
			i++; /* Increment here so that cleanup on error works.
			      */
			goto cleanup;
		}

		result = named_config_getname(mctx, tls, &tlss[i]);
		if (result != ISC_R_SUCCESS) {
			i++; /* Increment here so that cleanup on error works.
			      */
			goto cleanup;
		}

		/* If the port is unset, take it from one of the upper levels */
		if (isc_sockaddr_getport(&addrs[i]) == 0) {
			in_port_t addr_port = port;

			/* If unset, use the default port or tls-port */
			if (addr_port == 0) {
				if (tlss[i] != NULL) {
					addr_port = def_tlsport;
				} else {
					addr_port = def_port;
				}
			}

			isc_sockaddr_setport(&addrs[i], addr_port);
		}

		i++;
	}
	if (pushed != 0) {
		pushed--;
		element = stack[pushed].element;
		port = stack[pushed].port;
		goto resume;
	}

	shrink_array(mctx, addrs, i, addrcount);
	shrink_array(mctx, keys, i, keycount);
	shrink_array(mctx, tlss, i, tlscount);

	if (lists != NULL) {
		isc_mem_put(mctx, lists, listcount * sizeof(lists[0]));
	}
	if (stack != NULL) {
		isc_mem_put(mctx, stack, stackcount * sizeof(stack[0]));
	}

	INSIST(keycount == addrcount);
	INSIST(tlscount == addrcount);

	ipkl->addrs = addrs;
	ipkl->keys = keys;
	ipkl->tlss = tlss;
	ipkl->count = addrcount;
	ipkl->allocated = addrcount;

	return (ISC_R_SUCCESS);

cleanup:
	if (addrs != NULL) {
		isc_mem_put(mctx, addrs, addrcount * sizeof(addrs[0]));
	}
	if (keys != NULL) {
		for (j = 0; j < i; j++) {
			if (keys[j] == NULL) {
				continue;
			}
			if (dns_name_dynamic(keys[j])) {
				dns_name_free(keys[j], mctx);
			}
			isc_mem_put(mctx, keys[j], sizeof(*keys[j]));
		}
		isc_mem_put(mctx, keys, keycount * sizeof(keys[0]));
	}
	if (tlss != NULL) {
		for (j = 0; j < i; j++) {
			if (tlss[j] == NULL) {
				continue;
			}
			if (dns_name_dynamic(tlss[j])) {
				dns_name_free(tlss[j], mctx);
			}
			isc_mem_put(mctx, tlss[j], sizeof(*tlss[j]));
		}
		isc_mem_put(mctx, tlss, tlscount * sizeof(tlss[0]));
	}
	if (lists != NULL) {
		isc_mem_put(mctx, lists, listcount * sizeof(lists[0]));
	}
	if (stack != NULL) {
		isc_mem_put(mctx, stack, stackcount * sizeof(stack[0]));
	}
	return (result);
}

isc_result_t
named_config_getport(const cfg_obj_t *config, const char *type,
		     in_port_t *portp) {
	const cfg_obj_t *maps[3];
	const cfg_obj_t *options = NULL;
	const cfg_obj_t *portobj = NULL;
	isc_result_t result;
	int i;

	(void)cfg_map_get(config, "options", &options);
	i = 0;
	if (options != NULL) {
		maps[i++] = options;
	}
	maps[i++] = named_g_defaults;
	maps[i] = NULL;

	result = named_config_get(maps, type, &portobj);
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

struct keyalgorithms {
	const char *str;
	enum {
		hmacnone,
		hmacmd5,
		hmacsha1,
		hmacsha224,
		hmacsha256,
		hmacsha384,
		hmacsha512
	} hmac;
	unsigned int type;
	uint16_t size;
} algorithms[] = { { "hmac-md5", hmacmd5, DST_ALG_HMACMD5, 128 },
		   { "hmac-md5.sig-alg.reg.int", hmacmd5, DST_ALG_HMACMD5, 0 },
		   { "hmac-md5.sig-alg.reg.int.", hmacmd5, DST_ALG_HMACMD5, 0 },
		   { "hmac-sha1", hmacsha1, DST_ALG_HMACSHA1, 160 },
		   { "hmac-sha224", hmacsha224, DST_ALG_HMACSHA224, 224 },
		   { "hmac-sha256", hmacsha256, DST_ALG_HMACSHA256, 256 },
		   { "hmac-sha384", hmacsha384, DST_ALG_HMACSHA384, 384 },
		   { "hmac-sha512", hmacsha512, DST_ALG_HMACSHA512, 512 },
		   { NULL, hmacnone, DST_ALG_UNKNOWN, 0 } };

isc_result_t
named_config_getkeyalgorithm(const char *str, const dns_name_t **name,
			     uint16_t *digestbits) {
	return (named_config_getkeyalgorithm2(str, name, NULL, digestbits));
}

isc_result_t
named_config_getkeyalgorithm2(const char *str, const dns_name_t **name,
			      unsigned int *typep, uint16_t *digestbits) {
	int i;
	size_t len = 0;
	uint16_t bits;
	isc_result_t result;

	for (i = 0; algorithms[i].str != NULL; i++) {
		len = strlen(algorithms[i].str);
		if (strncasecmp(algorithms[i].str, str, len) == 0 &&
		    (str[len] == '\0' ||
		     (algorithms[i].size != 0 && str[len] == '-')))
		{
			break;
		}
	}
	if (algorithms[i].str == NULL) {
		return (ISC_R_NOTFOUND);
	}
	if (str[len] == '-') {
		result = isc_parse_uint16(&bits, str + len + 1, 10);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		if (bits > algorithms[i].size) {
			return (ISC_R_RANGE);
		}
	} else if (algorithms[i].size == 0) {
		bits = 128;
	} else {
		bits = algorithms[i].size;
	}

	if (name != NULL) {
		switch (algorithms[i].hmac) {
		case hmacmd5:
			*name = dns_tsig_hmacmd5_name;
			break;
		case hmacsha1:
			*name = dns_tsig_hmacsha1_name;
			break;
		case hmacsha224:
			*name = dns_tsig_hmacsha224_name;
			break;
		case hmacsha256:
			*name = dns_tsig_hmacsha256_name;
			break;
		case hmacsha384:
			*name = dns_tsig_hmacsha384_name;
			break;
		case hmacsha512:
			*name = dns_tsig_hmacsha512_name;
			break;
		default:
			UNREACHABLE();
		}
	}
	if (typep != NULL) {
		*typep = algorithms[i].type;
	}
	if (digestbits != NULL) {
		*digestbits = bits;
	}
	return (ISC_R_SUCCESS);
}
