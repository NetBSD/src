/*	$NetBSD: zoneconf.c,v 1.13 2023/01/25 21:43:23 christos Exp $	*/

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
#include <stdbool.h>

#include <isc/buffer.h>
#include <isc/file.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/stats.h>
#include <isc/string.h> /* Required for HP/UX (and others?) */
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/db.h>
#include <dns/fixedname.h>
#include <dns/ipkeylist.h>
#include <dns/journal.h>
#include <dns/kasp.h>
#include <dns/log.h>
#include <dns/masterdump.h>
#include <dns/name.h>
#include <dns/nsec3.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatatype.h>
#include <dns/result.h>
#include <dns/sdlz.h>
#include <dns/ssu.h>
#include <dns/stats.h>
#include <dns/tsig.h>
#include <dns/view.h>
#include <dns/zone.h>

#include <ns/client.h>

#include <named/config.h>
#include <named/globals.h>
#include <named/log.h>
#include <named/server.h>
#include <named/zoneconf.h>

/* ACLs associated with zone */
typedef enum {
	allow_notify,
	allow_query,
	allow_query_on,
	allow_transfer,
	allow_update,
	allow_update_forwarding
} acl_type_t;

#define RETERR(x)                        \
	do {                             \
		isc_result_t _r = (x);   \
		if (_r != ISC_R_SUCCESS) \
			return ((_r));   \
	} while (0)

#define CHECK(x)                             \
	do {                                 \
		result = (x);                \
		if (result != ISC_R_SUCCESS) \
			goto cleanup;        \
	} while (0)

/*%
 * Convenience function for configuring a single zone ACL.
 */
static isc_result_t
configure_zone_acl(const cfg_obj_t *zconfig, const cfg_obj_t *vconfig,
		   const cfg_obj_t *config, acl_type_t acltype,
		   cfg_aclconfctx_t *actx, dns_zone_t *zone,
		   void (*setzacl)(dns_zone_t *, dns_acl_t *),
		   void (*clearzacl)(dns_zone_t *)) {
	isc_result_t result;
	const cfg_obj_t *maps[5] = { NULL, NULL, NULL, NULL, NULL };
	const cfg_obj_t *aclobj = NULL;
	int i = 0;
	dns_acl_t **aclp = NULL, *acl = NULL;
	const char *aclname;
	dns_view_t *view;

	view = dns_zone_getview(zone);

	switch (acltype) {
	case allow_notify:
		if (view != NULL) {
			aclp = &view->notifyacl;
		}
		aclname = "allow-notify";
		break;
	case allow_query:
		if (view != NULL) {
			aclp = &view->queryacl;
		}
		aclname = "allow-query";
		break;
	case allow_query_on:
		if (view != NULL) {
			aclp = &view->queryonacl;
		}
		aclname = "allow-query-on";
		break;
	case allow_transfer:
		if (view != NULL) {
			aclp = &view->transferacl;
		}
		aclname = "allow-transfer";
		break;
	case allow_update:
		if (view != NULL) {
			aclp = &view->updateacl;
		}
		aclname = "allow-update";
		break;
	case allow_update_forwarding:
		if (view != NULL) {
			aclp = &view->upfwdacl;
		}
		aclname = "allow-update-forwarding";
		break;
	default:
		UNREACHABLE();
	}

	/* First check to see if ACL is defined within the zone */
	if (zconfig != NULL) {
		maps[0] = cfg_tuple_get(zconfig, "options");
		(void)named_config_get(maps, aclname, &aclobj);
		if (aclobj != NULL) {
			aclp = NULL;
			goto parse_acl;
		}
	}

	/* Failing that, see if there's a default ACL already in the view */
	if (aclp != NULL && *aclp != NULL) {
		(*setzacl)(zone, *aclp);
		return (ISC_R_SUCCESS);
	}

	/* Check for default ACLs that haven't been parsed yet */
	if (vconfig != NULL) {
		const cfg_obj_t *options = cfg_tuple_get(vconfig, "options");
		if (options != NULL) {
			maps[i++] = options;
		}
	}
	if (config != NULL) {
		const cfg_obj_t *options = NULL;
		(void)cfg_map_get(config, "options", &options);
		if (options != NULL) {
			maps[i++] = options;
		}
	}
	maps[i++] = named_g_defaults;
	maps[i] = NULL;

	(void)named_config_get(maps, aclname, &aclobj);
	if (aclobj == NULL) {
		(*clearzacl)(zone);
		return (ISC_R_SUCCESS);
	}

parse_acl:
	result = cfg_acl_fromconfig(aclobj, config, named_g_lctx, actx,
				    named_g_mctx, 0, &acl);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	(*setzacl)(zone, acl);

	/* Set the view default now */
	if (aclp != NULL) {
		dns_acl_attach(acl, aclp);
	}

	dns_acl_detach(&acl);
	return (ISC_R_SUCCESS);
}

/*%
 * Parse the zone update-policy statement.
 */
static isc_result_t
configure_zone_ssutable(const cfg_obj_t *zconfig, dns_zone_t *zone,
			const char *zname) {
	const cfg_obj_t *updatepolicy = NULL;
	const cfg_listelt_t *element, *element2;
	dns_ssutable_t *table = NULL;
	isc_mem_t *mctx = dns_zone_getmctx(zone);
	bool autoddns = false;
	isc_result_t result;

	(void)cfg_map_get(zconfig, "update-policy", &updatepolicy);

	if (updatepolicy == NULL) {
		dns_zone_setssutable(zone, NULL);
		return (ISC_R_SUCCESS);
	}

	if (cfg_obj_isstring(updatepolicy) &&
	    strcmp("local", cfg_obj_asstring(updatepolicy)) == 0)
	{
		autoddns = true;
		updatepolicy = NULL;
	}

	result = dns_ssutable_create(mctx, &table);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	for (element = cfg_list_first(updatepolicy); element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *stmt = cfg_listelt_value(element);
		const cfg_obj_t *mode = cfg_tuple_get(stmt, "mode");
		const cfg_obj_t *identity = cfg_tuple_get(stmt, "identity");
		const cfg_obj_t *matchtype = cfg_tuple_get(stmt, "matchtype");
		const cfg_obj_t *dname = cfg_tuple_get(stmt, "name");
		const cfg_obj_t *typelist = cfg_tuple_get(stmt, "types");
		const char *str;
		bool grant = false;
		bool usezone = false;
		dns_ssumatchtype_t mtype = dns_ssumatchtype_name;
		dns_fixedname_t fname, fident;
		isc_buffer_t b;
		dns_rdatatype_t *types;
		unsigned int i, n;

		str = cfg_obj_asstring(mode);
		if (strcasecmp(str, "grant") == 0) {
			grant = true;
		} else if (strcasecmp(str, "deny") == 0) {
			grant = false;
		} else {
			UNREACHABLE();
		}

		str = cfg_obj_asstring(matchtype);
		CHECK(dns_ssu_mtypefromstring(str, &mtype));
		if (mtype == dns_ssumatchtype_subdomain &&
		    strcasecmp(str, "zonesub") == 0)
		{
			usezone = true;
		}

		dns_fixedname_init(&fident);
		str = cfg_obj_asstring(identity);
		isc_buffer_constinit(&b, str, strlen(str));
		isc_buffer_add(&b, strlen(str));
		result = dns_name_fromtext(dns_fixedname_name(&fident), &b,
					   dns_rootname, 0, NULL);
		if (result != ISC_R_SUCCESS) {
			cfg_obj_log(identity, named_g_lctx, ISC_LOG_ERROR,
				    "'%s' is not a valid name", str);
			goto cleanup;
		}

		dns_fixedname_init(&fname);
		if (usezone) {
			dns_name_copynf(dns_zone_getorigin(zone),
					dns_fixedname_name(&fname));
		} else {
			str = cfg_obj_asstring(dname);
			isc_buffer_constinit(&b, str, strlen(str));
			isc_buffer_add(&b, strlen(str));
			result = dns_name_fromtext(dns_fixedname_name(&fname),
						   &b, dns_rootname, 0, NULL);
			if (result != ISC_R_SUCCESS) {
				cfg_obj_log(identity, named_g_lctx,
					    ISC_LOG_ERROR,
					    "'%s' is not a valid name", str);
				goto cleanup;
			}
		}

		n = named_config_listcount(typelist);
		if (n == 0) {
			types = NULL;
		} else {
			types = isc_mem_get(mctx, n * sizeof(dns_rdatatype_t));
		}

		i = 0;
		for (element2 = cfg_list_first(typelist); element2 != NULL;
		     element2 = cfg_list_next(element2))
		{
			const cfg_obj_t *typeobj;
			isc_textregion_t r;

			INSIST(i < n);

			typeobj = cfg_listelt_value(element2);
			str = cfg_obj_asstring(typeobj);
			DE_CONST(str, r.base);
			r.length = strlen(str);

			result = dns_rdatatype_fromtext(&types[i++], &r);
			if (result != ISC_R_SUCCESS) {
				cfg_obj_log(identity, named_g_lctx,
					    ISC_LOG_ERROR,
					    "'%s' is not a valid type", str);
				isc_mem_put(mctx, types,
					    n * sizeof(dns_rdatatype_t));
				goto cleanup;
			}
		}
		INSIST(i == n);

		result = dns_ssutable_addrule(
			table, grant, dns_fixedname_name(&fident), mtype,
			dns_fixedname_name(&fname), n, types);
		if (types != NULL) {
			isc_mem_put(mctx, types, n * sizeof(dns_rdatatype_t));
		}
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
	}

	/*
	 * If "update-policy local;" and a session key exists,
	 * then use the default policy, which is equivalent to:
	 * update-policy { grant <session-keyname> zonesub any; };
	 */
	if (autoddns) {
		dns_rdatatype_t any = dns_rdatatype_any;

		if (named_g_server->session_keyname == NULL) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "failed to enable auto DDNS policy "
				      "for zone %s: session key not found",
				      zname);
			result = ISC_R_NOTFOUND;
			goto cleanup;
		}

		result = dns_ssutable_addrule(
			table, true, named_g_server->session_keyname,
			dns_ssumatchtype_local, dns_zone_getorigin(zone), 1,
			&any);

		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
	}

	result = ISC_R_SUCCESS;
	dns_zone_setssutable(zone, table);

cleanup:
	dns_ssutable_detach(&table);
	return (result);
}

/*
 * This is the TTL used for internally generated RRsets for static-stub zones.
 * The value doesn't matter because the mapping is static, but needs to be
 * defined for the sake of implementation.
 */
#define STATICSTUB_SERVER_TTL 86400

/*%
 * Configure an apex NS with glues for a static-stub zone.
 * For example, for the zone named "example.com", the following RRs will be
 * added to the zone DB:
 * example.com. NS example.com.
 * example.com. A 192.0.2.1
 * example.com. AAAA 2001:db8::1
 */
static isc_result_t
configure_staticstub_serveraddrs(const cfg_obj_t *zconfig, dns_zone_t *zone,
				 dns_rdatalist_t *rdatalist_ns,
				 dns_rdatalist_t *rdatalist_a,
				 dns_rdatalist_t *rdatalist_aaaa) {
	const cfg_listelt_t *element;
	isc_mem_t *mctx = dns_zone_getmctx(zone);
	isc_region_t region, sregion;
	dns_rdata_t *rdata;
	isc_result_t result = ISC_R_SUCCESS;

	for (element = cfg_list_first(zconfig); element != NULL;
	     element = cfg_list_next(element))
	{
		const isc_sockaddr_t *sa;
		isc_netaddr_t na;
		const cfg_obj_t *address = cfg_listelt_value(element);
		dns_rdatalist_t *rdatalist;

		sa = cfg_obj_assockaddr(address);
		if (isc_sockaddr_getport(sa) != 0) {
			cfg_obj_log(zconfig, named_g_lctx, ISC_LOG_ERROR,
				    "port is not configurable for "
				    "static stub server-addresses");
			return (ISC_R_FAILURE);
		}
		isc_netaddr_fromsockaddr(&na, sa);
		if (isc_netaddr_getzone(&na) != 0) {
			cfg_obj_log(zconfig, named_g_lctx, ISC_LOG_ERROR,
				    "scoped address is not allowed "
				    "for static stub "
				    "server-addresses");
			return (ISC_R_FAILURE);
		}

		switch (na.family) {
		case AF_INET:
			region.length = sizeof(na.type.in);
			rdatalist = rdatalist_a;
			break;
		default:
			INSIST(na.family == AF_INET6);
			region.length = sizeof(na.type.in6);
			rdatalist = rdatalist_aaaa;
			break;
		}

		rdata = isc_mem_get(mctx, sizeof(*rdata) + region.length);
		region.base = (unsigned char *)(rdata + 1);
		memmove(region.base, &na.type, region.length);
		dns_rdata_init(rdata);
		dns_rdata_fromregion(rdata, dns_zone_getclass(zone),
				     rdatalist->type, &region);
		ISC_LIST_APPEND(rdatalist->rdata, rdata, link);
	}

	/*
	 * If no address is specified (unlikely in this context, but possible),
	 * there's nothing to do anymore.
	 */
	if (ISC_LIST_EMPTY(rdatalist_a->rdata) &&
	    ISC_LIST_EMPTY(rdatalist_aaaa->rdata))
	{
		return (ISC_R_SUCCESS);
	}

	/* Add to the list an apex NS with the ns name being the origin name */
	dns_name_toregion(dns_zone_getorigin(zone), &sregion);
	rdata = isc_mem_get(mctx, sizeof(*rdata) + sregion.length);
	region.length = sregion.length;
	region.base = (unsigned char *)(rdata + 1);
	memmove(region.base, sregion.base, region.length);
	dns_rdata_init(rdata);
	dns_rdata_fromregion(rdata, dns_zone_getclass(zone), dns_rdatatype_ns,
			     &region);
	ISC_LIST_APPEND(rdatalist_ns->rdata, rdata, link);

	return (result);
}

/*%
 * Configure an apex NS with an out-of-zone NS names for a static-stub zone.
 * For example, for the zone named "example.com", something like the following
 * RRs will be added to the zone DB:
 * example.com. NS ns.example.net.
 */
static isc_result_t
configure_staticstub_servernames(const cfg_obj_t *zconfig, dns_zone_t *zone,
				 dns_rdatalist_t *rdatalist,
				 const char *zname) {
	const cfg_listelt_t *element;
	isc_mem_t *mctx = dns_zone_getmctx(zone);
	dns_rdata_t *rdata;
	isc_region_t sregion, region;
	isc_result_t result = ISC_R_SUCCESS;

	for (element = cfg_list_first(zconfig); element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *obj;
		const char *str;
		dns_fixedname_t fixed_name;
		dns_name_t *nsname;
		isc_buffer_t b;

		obj = cfg_listelt_value(element);
		str = cfg_obj_asstring(obj);

		nsname = dns_fixedname_initname(&fixed_name);

		isc_buffer_constinit(&b, str, strlen(str));
		isc_buffer_add(&b, strlen(str));
		result = dns_name_fromtext(nsname, &b, dns_rootname, 0, NULL);
		if (result != ISC_R_SUCCESS) {
			cfg_obj_log(zconfig, named_g_lctx, ISC_LOG_ERROR,
				    "server-name '%s' is not a valid "
				    "name",
				    str);
			return (result);
		}
		if (dns_name_issubdomain(nsname, dns_zone_getorigin(zone))) {
			cfg_obj_log(zconfig, named_g_lctx, ISC_LOG_ERROR,
				    "server-name '%s' must not be a "
				    "subdomain of zone name '%s'",
				    str, zname);
			return (ISC_R_FAILURE);
		}

		dns_name_toregion(nsname, &sregion);
		rdata = isc_mem_get(mctx, sizeof(*rdata) + sregion.length);
		region.length = sregion.length;
		region.base = (unsigned char *)(rdata + 1);
		memmove(region.base, sregion.base, region.length);
		dns_rdata_init(rdata);
		dns_rdata_fromregion(rdata, dns_zone_getclass(zone),
				     dns_rdatatype_ns, &region);
		ISC_LIST_APPEND(rdatalist->rdata, rdata, link);
	}

	return (result);
}

/*%
 * Configure static-stub zone.
 */
static isc_result_t
configure_staticstub(const cfg_obj_t *zconfig, dns_zone_t *zone,
		     const char *zname, const char *dbtype) {
	int i = 0;
	const cfg_obj_t *obj;
	isc_mem_t *mctx = dns_zone_getmctx(zone);
	dns_db_t *db = NULL;
	dns_dbversion_t *dbversion = NULL;
	dns_dbnode_t *apexnode = NULL;
	dns_name_t apexname;
	isc_result_t result;
	dns_rdataset_t rdataset;
	dns_rdatalist_t rdatalist_ns, rdatalist_a, rdatalist_aaaa;
	dns_rdatalist_t *rdatalists[] = { &rdatalist_ns, &rdatalist_a,
					  &rdatalist_aaaa, NULL };
	dns_rdata_t *rdata;
	isc_region_t region;

	/* Create the DB beforehand */
	RETERR(dns_db_create(mctx, dbtype, dns_zone_getorigin(zone),
			     dns_dbtype_stub, dns_zone_getclass(zone), 0, NULL,
			     &db));

	dns_rdataset_init(&rdataset);

	dns_rdatalist_init(&rdatalist_ns);
	rdatalist_ns.rdclass = dns_zone_getclass(zone);
	rdatalist_ns.type = dns_rdatatype_ns;
	rdatalist_ns.ttl = STATICSTUB_SERVER_TTL;

	dns_rdatalist_init(&rdatalist_a);
	rdatalist_a.rdclass = dns_zone_getclass(zone);
	rdatalist_a.type = dns_rdatatype_a;
	rdatalist_a.ttl = STATICSTUB_SERVER_TTL;

	dns_rdatalist_init(&rdatalist_aaaa);
	rdatalist_aaaa.rdclass = dns_zone_getclass(zone);
	rdatalist_aaaa.type = dns_rdatatype_aaaa;
	rdatalist_aaaa.ttl = STATICSTUB_SERVER_TTL;

	/* Prepare zone RRs from the configuration */
	obj = NULL;
	result = cfg_map_get(zconfig, "server-addresses", &obj);
	if (result == ISC_R_SUCCESS) {
		INSIST(obj != NULL);
		CHECK(configure_staticstub_serveraddrs(obj, zone, &rdatalist_ns,
						       &rdatalist_a,
						       &rdatalist_aaaa));
	}

	obj = NULL;
	result = cfg_map_get(zconfig, "server-names", &obj);
	if (result == ISC_R_SUCCESS) {
		INSIST(obj != NULL);
		CHECK(configure_staticstub_servernames(obj, zone, &rdatalist_ns,
						       zname));
	}

	/*
	 * Sanity check: there should be at least one NS RR at the zone apex
	 * to trigger delegation.
	 */
	if (ISC_LIST_EMPTY(rdatalist_ns.rdata)) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "No NS record is configured for a "
			      "static-stub zone '%s'",
			      zname);
		result = ISC_R_FAILURE;
		goto cleanup;
	}

	/*
	 * Now add NS and glue A/AAAA RRsets to the zone DB.
	 * First open a new version for the add operation and get a pointer
	 * to the apex node (all RRs are of the apex name).
	 */
	CHECK(dns_db_newversion(db, &dbversion));

	dns_name_init(&apexname, NULL);
	dns_name_clone(dns_zone_getorigin(zone), &apexname);
	CHECK(dns_db_findnode(db, &apexname, false, &apexnode));

	/* Add NS RRset */
	RUNTIME_CHECK(dns_rdatalist_tordataset(&rdatalist_ns, &rdataset) ==
		      ISC_R_SUCCESS);
	CHECK(dns_db_addrdataset(db, apexnode, dbversion, 0, &rdataset, 0,
				 NULL));
	dns_rdataset_disassociate(&rdataset);

	/* Add glue A RRset, if any */
	if (!ISC_LIST_EMPTY(rdatalist_a.rdata)) {
		RUNTIME_CHECK(
			dns_rdatalist_tordataset(&rdatalist_a, &rdataset) ==
			ISC_R_SUCCESS);
		CHECK(dns_db_addrdataset(db, apexnode, dbversion, 0, &rdataset,
					 0, NULL));
		dns_rdataset_disassociate(&rdataset);
	}

	/* Add glue AAAA RRset, if any */
	if (!ISC_LIST_EMPTY(rdatalist_aaaa.rdata)) {
		RUNTIME_CHECK(
			dns_rdatalist_tordataset(&rdatalist_aaaa, &rdataset) ==
			ISC_R_SUCCESS);
		CHECK(dns_db_addrdataset(db, apexnode, dbversion, 0, &rdataset,
					 0, NULL));
		dns_rdataset_disassociate(&rdataset);
	}

	dns_db_closeversion(db, &dbversion, true);
	dns_zone_setdb(zone, db);

	result = ISC_R_SUCCESS;

cleanup:
	if (dns_rdataset_isassociated(&rdataset)) {
		dns_rdataset_disassociate(&rdataset);
	}
	if (apexnode != NULL) {
		dns_db_detachnode(db, &apexnode);
	}
	if (dbversion != NULL) {
		dns_db_closeversion(db, &dbversion, false);
	}
	if (db != NULL) {
		dns_db_detach(&db);
	}
	for (i = 0; rdatalists[i] != NULL; i++) {
		while ((rdata = ISC_LIST_HEAD(rdatalists[i]->rdata)) != NULL) {
			ISC_LIST_UNLINK(rdatalists[i]->rdata, rdata, link);
			dns_rdata_toregion(rdata, &region);
			isc_mem_put(mctx, rdata,
				    sizeof(*rdata) + region.length);
		}
	}

	INSIST(dbversion == NULL);

	return (result);
}

/*%
 * Convert a config file zone type into a server zone type.
 */
static dns_zonetype_t
zonetype_fromconfig(const cfg_obj_t *map) {
	const cfg_obj_t *obj = NULL;
	isc_result_t result;

	result = cfg_map_get(map, "type", &obj);
	INSIST(result == ISC_R_SUCCESS && obj != NULL);
	return (named_config_getzonetype(obj));
}

/*%
 * Helper function for strtoargv().  Pardon the gratuitous recursion.
 */
static isc_result_t
strtoargvsub(isc_mem_t *mctx, char *s, unsigned int *argcp, char ***argvp,
	     unsigned int n) {
	isc_result_t result;

	/* Discard leading whitespace. */
	while (*s == ' ' || *s == '\t') {
		s++;
	}

	if (*s == '\0') {
		/* We have reached the end of the string. */
		*argcp = n;
		*argvp = isc_mem_get(mctx, n * sizeof(char *));
	} else {
		char *p = s;
		while (*p != ' ' && *p != '\t' && *p != '\0') {
			p++;
		}
		if (*p != '\0') {
			*p++ = '\0';
		}

		result = strtoargvsub(mctx, p, argcp, argvp, n + 1);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		(*argvp)[n] = s;
	}
	return (ISC_R_SUCCESS);
}

/*%
 * Tokenize the string "s" into whitespace-separated words,
 * return the number of words in '*argcp' and an array
 * of pointers to the words in '*argvp'.  The caller
 * must free the array using isc_mem_put().  The string
 * is modified in-place.
 */
static isc_result_t
strtoargv(isc_mem_t *mctx, char *s, unsigned int *argcp, char ***argvp) {
	return (strtoargvsub(mctx, s, argcp, argvp, 0));
}

static const char *const primary_synonyms[] = { "primary", "master", NULL };

static const char *const secondary_synonyms[] = { "secondary", "slave", NULL };

static void
checknames(dns_zonetype_t ztype, const cfg_obj_t **maps,
	   const cfg_obj_t **objp) {
	isc_result_t result;

	switch (ztype) {
	case dns_zone_secondary:
	case dns_zone_mirror:
		result = named_checknames_get(maps, secondary_synonyms, objp);
		break;
	case dns_zone_primary:
		result = named_checknames_get(maps, primary_synonyms, objp);
		break;
	default:
		UNREACHABLE();
	}

	INSIST(result == ISC_R_SUCCESS && objp != NULL && *objp != NULL);
}

/*
 * Callback to see if a non-recursive query coming from 'srcaddr' to
 * 'destaddr', with optional key 'mykey' for class 'rdclass' would be
 * delivered to 'myview'.
 *
 * We run this unlocked as both the view list and the interface list
 * are updated when the appropriate task has exclusivity.
 */
static bool
isself(dns_view_t *myview, dns_tsigkey_t *mykey, const isc_sockaddr_t *srcaddr,
       const isc_sockaddr_t *dstaddr, dns_rdataclass_t rdclass, void *arg) {
	ns_interfacemgr_t *interfacemgr = (ns_interfacemgr_t *)arg;
	dns_aclenv_t *env = ns_interfacemgr_getaclenv(interfacemgr);
	dns_view_t *view;
	dns_tsigkey_t *key = NULL;
	isc_netaddr_t netsrc;
	isc_netaddr_t netdst;

	if (interfacemgr == NULL) {
		return (true);
	}

	if (!ns_interfacemgr_listeningon(interfacemgr, dstaddr)) {
		return (false);
	}

	isc_netaddr_fromsockaddr(&netsrc, srcaddr);
	isc_netaddr_fromsockaddr(&netdst, dstaddr);

	for (view = ISC_LIST_HEAD(named_g_server->viewlist); view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		const dns_name_t *tsig = NULL;

		if (view->matchrecursiveonly) {
			continue;
		}

		if (rdclass != view->rdclass) {
			continue;
		}

		if (mykey != NULL) {
			bool match;
			isc_result_t result;

			result = dns_view_gettsig(view, &mykey->name, &key);
			if (result != ISC_R_SUCCESS) {
				continue;
			}
			match = dst_key_compare(mykey->key, key->key);
			dns_tsigkey_detach(&key);
			if (!match) {
				continue;
			}
			tsig = dns_tsigkey_identity(mykey);
		}

		if (dns_acl_allowed(&netsrc, tsig, view->matchclients, env) &&
		    dns_acl_allowed(&netdst, tsig, view->matchdestinations,
				    env))
		{
			break;
		}
	}
	return (view == myview);
}

/*%
 * For mirror zones, change "notify yes;" to "notify explicit;", informing the
 * user only if "notify" was explicitly configured rather than inherited from
 * default configuration.
 */
static dns_notifytype_t
process_notifytype(dns_notifytype_t ntype, dns_zonetype_t ztype,
		   const char *zname, const cfg_obj_t **maps) {
	const cfg_obj_t *obj = NULL;

	/*
	 * Return the original setting if this is not a mirror zone or if the
	 * zone is configured with something else than "notify yes;".
	 */
	if (ztype != dns_zone_mirror || ntype != dns_notifytype_yes) {
		return (ntype);
	}

	/*
	 * Only log a message if "notify" was set in the configuration
	 * hierarchy supplied in 'maps'.
	 */
	if (named_config_get(maps, "notify", &obj) == ISC_R_SUCCESS) {
		cfg_obj_log(obj, named_g_lctx, ISC_LOG_INFO,
			    "'notify explicit;' will be used for mirror zone "
			    "'%s'",
			    zname);
	}

	return (dns_notifytype_explicit);
}

isc_result_t
named_zone_configure(const cfg_obj_t *config, const cfg_obj_t *vconfig,
		     const cfg_obj_t *zconfig, cfg_aclconfctx_t *ac,
		     dns_kasplist_t *kasplist, dns_zone_t *zone,
		     dns_zone_t *raw) {
	isc_result_t result;
	const char *zname;
	dns_rdataclass_t zclass;
	dns_rdataclass_t vclass;
	const cfg_obj_t *maps[5];
	const cfg_obj_t *nodefault[4];
	const cfg_obj_t *zoptions = NULL;
	const cfg_obj_t *options = NULL;
	const cfg_obj_t *obj;
	const char *filename = NULL;
	const char *kaspname = NULL;
	const char *dupcheck;
	dns_notifytype_t notifytype = dns_notifytype_yes;
	uint32_t count;
	unsigned int dbargc;
	char **dbargv;
	static char default_dbtype[] = "rbt";
	static char dlz_dbtype[] = "dlz";
	char *cpval = default_dbtype;
	isc_mem_t *mctx = dns_zone_getmctx(zone);
	dns_dialuptype_t dialup = dns_dialuptype_no;
	dns_zonetype_t ztype;
	int i;
	int32_t journal_size;
	bool multi;
	bool alt;
	dns_view_t *view = NULL;
	dns_kasp_t *kasp = NULL;
	bool check = false, fail = false;
	bool warn = false, ignore = false;
	bool ixfrdiff;
	bool use_kasp = false;
	dns_masterformat_t masterformat;
	const dns_master_style_t *masterstyle = &dns_master_style_default;
	isc_stats_t *zoneqrystats;
	dns_stats_t *rcvquerystats;
	dns_stats_t *dnssecsignstats;
	dns_zonestat_level_t statlevel = dns_zonestat_none;
	int seconds;
	dns_ttl_t maxttl = 0; /* unlimited */
	dns_zone_t *mayberaw = (raw != NULL) ? raw : zone;
	isc_dscp_t dscp;

	i = 0;
	if (zconfig != NULL) {
		zoptions = cfg_tuple_get(zconfig, "options");
		nodefault[i] = maps[i] = zoptions;
		i++;
	}
	if (vconfig != NULL) {
		nodefault[i] = maps[i] = cfg_tuple_get(vconfig, "options");
		i++;
	}
	if (config != NULL) {
		(void)cfg_map_get(config, "options", &options);
		if (options != NULL) {
			nodefault[i] = maps[i] = options;
			i++;
		}
	}
	nodefault[i] = NULL;
	maps[i++] = named_g_defaults;
	maps[i] = NULL;

	if (vconfig != NULL) {
		RETERR(named_config_getclass(cfg_tuple_get(vconfig, "class"),
					     dns_rdataclass_in, &vclass));
	} else {
		vclass = dns_rdataclass_in;
	}

	/*
	 * Configure values common to all zone types.
	 */

	zname = cfg_obj_asstring(cfg_tuple_get(zconfig, "name"));

	RETERR(named_config_getclass(cfg_tuple_get(zconfig, "class"), vclass,
				     &zclass));
	dns_zone_setclass(zone, zclass);
	if (raw != NULL) {
		dns_zone_setclass(raw, zclass);
	}

	ztype = zonetype_fromconfig(zoptions);
	if (raw != NULL) {
		dns_zone_settype(raw, ztype);
		dns_zone_settype(zone, dns_zone_primary);
	} else {
		dns_zone_settype(zone, ztype);
	}

	obj = NULL;
	result = cfg_map_get(zoptions, "database", &obj);
	if (result == ISC_R_SUCCESS) {
		cpval = isc_mem_strdup(mctx, cfg_obj_asstring(obj));
	}
	if (cpval == NULL) {
		return (ISC_R_NOMEMORY);
	}

	obj = NULL;
	result = cfg_map_get(zoptions, "dlz", &obj);
	if (result == ISC_R_SUCCESS) {
		const char *dlzname = cfg_obj_asstring(obj);
		size_t len;

		if (cpval != default_dbtype) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "zone '%s': both 'database' and 'dlz' "
				      "specified",
				      zname);
			return (ISC_R_FAILURE);
		}

		len = strlen(dlzname) + 5;
		cpval = isc_mem_allocate(mctx, len);
		snprintf(cpval, len, "dlz %s", dlzname);
	}

	result = strtoargv(mctx, cpval, &dbargc, &dbargv);
	if (result != ISC_R_SUCCESS && cpval != default_dbtype) {
		isc_mem_free(mctx, cpval);
		return (result);
	}

	/*
	 * ANSI C is strange here.  There is no logical reason why (char **)
	 * cannot be promoted automatically to (const char * const *) by the
	 * compiler w/o generating a warning.
	 */
	dns_zone_setdbtype(zone, dbargc, (const char *const *)dbargv);
	isc_mem_put(mctx, dbargv, dbargc * sizeof(*dbargv));
	if (cpval != default_dbtype && cpval != dlz_dbtype) {
		isc_mem_free(mctx, cpval);
	}

	obj = NULL;
	result = cfg_map_get(zoptions, "file", &obj);
	if (result == ISC_R_SUCCESS) {
		filename = cfg_obj_asstring(obj);
	}

	/*
	 * Unless we're using some alternative database, a master zone
	 * will be needing a master file.
	 */
	if (ztype == dns_zone_primary && cpval == default_dbtype &&
	    filename == NULL)
	{
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "zone '%s': 'file' not specified", zname);
		return (ISC_R_FAILURE);
	}

	if (ztype == dns_zone_secondary || ztype == dns_zone_mirror) {
		masterformat = dns_masterformat_raw;
	} else {
		masterformat = dns_masterformat_text;
	}
	obj = NULL;
	result = named_config_get(maps, "masterfile-format", &obj);
	if (result == ISC_R_SUCCESS) {
		const char *masterformatstr = cfg_obj_asstring(obj);

		if (strcasecmp(masterformatstr, "text") == 0) {
			masterformat = dns_masterformat_text;
		} else if (strcasecmp(masterformatstr, "raw") == 0) {
			masterformat = dns_masterformat_raw;
		} else if (strcasecmp(masterformatstr, "map") == 0) {
			masterformat = dns_masterformat_map;
			cfg_obj_log(obj, named_g_lctx, ISC_LOG_WARNING,
				    "masterfile-format: format 'map' is "
				    "deprecated");
		} else {
			UNREACHABLE();
		}
	}

	obj = NULL;
	result = named_config_get(maps, "masterfile-style", &obj);
	if (result == ISC_R_SUCCESS) {
		const char *masterstylestr = cfg_obj_asstring(obj);

		if (masterformat != dns_masterformat_text) {
			cfg_obj_log(obj, named_g_lctx, ISC_LOG_ERROR,
				    "zone '%s': 'masterfile-style' "
				    "can only be used with "
				    "'masterfile-format text'",
				    zname);
			return (ISC_R_FAILURE);
		}

		if (strcasecmp(masterstylestr, "full") == 0) {
			masterstyle = &dns_master_style_full;
		} else if (strcasecmp(masterstylestr, "relative") == 0) {
			masterstyle = &dns_master_style_default;
		} else {
			UNREACHABLE();
		}
	}

	obj = NULL;
	result = named_config_get(maps, "max-records", &obj);
	INSIST(result == ISC_R_SUCCESS && obj != NULL);
	dns_zone_setmaxrecords(mayberaw, cfg_obj_asuint32(obj));
	if (zone != mayberaw) {
		dns_zone_setmaxrecords(zone, 0);
	}

	if (raw != NULL && filename != NULL) {
#define SIGNED ".signed"
		size_t signedlen = strlen(filename) + sizeof(SIGNED);
		char *signedname;

		RETERR(dns_zone_setfile(raw, filename, masterformat,
					masterstyle));
		signedname = isc_mem_get(mctx, signedlen);

		(void)snprintf(signedname, signedlen, "%s" SIGNED, filename);
		result = dns_zone_setfile(zone, signedname,
					  dns_masterformat_raw, NULL);
		isc_mem_put(mctx, signedname, signedlen);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	} else {
		RETERR(dns_zone_setfile(zone, filename, masterformat,
					masterstyle));
	}

	obj = NULL;
	result = cfg_map_get(zoptions, "journal", &obj);
	if (result == ISC_R_SUCCESS) {
		RETERR(dns_zone_setjournal(mayberaw, cfg_obj_asstring(obj)));
	}

	/*
	 * Notify messages are processed by the raw zone if it exists.
	 */
	if (ztype == dns_zone_secondary || ztype == dns_zone_mirror) {
		RETERR(configure_zone_acl(
			zconfig, vconfig, config, allow_notify, ac, mayberaw,
			dns_zone_setnotifyacl, dns_zone_clearnotifyacl));
	}

	/*
	 * XXXAG This probably does not make sense for stubs.
	 */
	RETERR(configure_zone_acl(zconfig, vconfig, config, allow_query, ac,
				  zone, dns_zone_setqueryacl,
				  dns_zone_clearqueryacl));

	RETERR(configure_zone_acl(zconfig, vconfig, config, allow_query_on, ac,
				  zone, dns_zone_setqueryonacl,
				  dns_zone_clearqueryonacl));

	obj = NULL;
	result = named_config_get(maps, "dialup", &obj);
	INSIST(result == ISC_R_SUCCESS && obj != NULL);
	if (cfg_obj_isboolean(obj)) {
		if (cfg_obj_asboolean(obj)) {
			dialup = dns_dialuptype_yes;
		} else {
			dialup = dns_dialuptype_no;
		}
	} else {
		const char *dialupstr = cfg_obj_asstring(obj);
		if (strcasecmp(dialupstr, "notify") == 0) {
			dialup = dns_dialuptype_notify;
		} else if (strcasecmp(dialupstr, "notify-passive") == 0) {
			dialup = dns_dialuptype_notifypassive;
		} else if (strcasecmp(dialupstr, "refresh") == 0) {
			dialup = dns_dialuptype_refresh;
		} else if (strcasecmp(dialupstr, "passive") == 0) {
			dialup = dns_dialuptype_passive;
		} else {
			UNREACHABLE();
		}
	}
	if (raw != NULL) {
		dns_zone_setdialup(raw, dialup);
	}
	dns_zone_setdialup(zone, dialup);

	obj = NULL;
	result = named_config_get(maps, "zone-statistics", &obj);
	INSIST(result == ISC_R_SUCCESS && obj != NULL);
	if (cfg_obj_isboolean(obj)) {
		if (cfg_obj_asboolean(obj)) {
			statlevel = dns_zonestat_full;
		} else {
			statlevel = dns_zonestat_none;
		}
	} else {
		const char *levelstr = cfg_obj_asstring(obj);
		if (strcasecmp(levelstr, "full") == 0) {
			statlevel = dns_zonestat_full;
		} else if (strcasecmp(levelstr, "terse") == 0) {
			statlevel = dns_zonestat_terse;
		} else if (strcasecmp(levelstr, "none") == 0) {
			statlevel = dns_zonestat_none;
		} else {
			UNREACHABLE();
		}
	}
	dns_zone_setstatlevel(zone, statlevel);

	zoneqrystats = NULL;
	rcvquerystats = NULL;
	dnssecsignstats = NULL;
	if (statlevel == dns_zonestat_full) {
		RETERR(isc_stats_create(mctx, &zoneqrystats,
					ns_statscounter_max));
		RETERR(dns_rdatatypestats_create(mctx, &rcvquerystats));
		RETERR(dns_dnssecsignstats_create(mctx, &dnssecsignstats));
	}
	dns_zone_setrequeststats(zone, zoneqrystats);
	dns_zone_setrcvquerystats(zone, rcvquerystats);
	dns_zone_setdnssecsignstats(zone, dnssecsignstats);

	if (zoneqrystats != NULL) {
		isc_stats_detach(&zoneqrystats);
	}

	if (rcvquerystats != NULL) {
		dns_stats_detach(&rcvquerystats);
	}

	if (dnssecsignstats != NULL) {
		dns_stats_detach(&dnssecsignstats);
	}

	/*
	 * Configure master functionality.  This applies
	 * to primary servers (type "primary") and secondaries
	 * acting as primaries (type "secondary"), but not to stubs.
	 */
	if (ztype != dns_zone_stub && ztype != dns_zone_staticstub &&
	    ztype != dns_zone_redirect)
	{
		obj = NULL;
		result = named_config_get(maps, "dnssec-policy", &obj);
		if (result == ISC_R_SUCCESS) {
			kaspname = cfg_obj_asstring(obj);
			if (strcmp(kaspname, "none") != 0) {
				result = dns_kasplist_find(kasplist, kaspname,
							   &kasp);
				if (result != ISC_R_SUCCESS) {
					cfg_obj_log(
						obj, named_g_lctx,
						ISC_LOG_ERROR,
						"dnssec-policy '%s' not found ",
						kaspname);
					RETERR(result);
				}
				dns_zone_setkasp(zone, kasp);
				use_kasp = true;
			}
		}
		if (!use_kasp) {
			dns_zone_setkasp(zone, NULL);
		}

		obj = NULL;
		result = named_config_get(maps, "notify", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		if (cfg_obj_isboolean(obj)) {
			if (cfg_obj_asboolean(obj)) {
				notifytype = dns_notifytype_yes;
			} else {
				notifytype = dns_notifytype_no;
			}
		} else {
			const char *str = cfg_obj_asstring(obj);
			if (strcasecmp(str, "explicit") == 0) {
				notifytype = dns_notifytype_explicit;
			} else if (strcasecmp(str, "master-only") == 0 ||
				   strcasecmp(str, "primary-only") == 0)
			{
				notifytype = dns_notifytype_masteronly;
			} else {
				UNREACHABLE();
			}
		}
		notifytype = process_notifytype(notifytype, ztype, zname,
						nodefault);
		if (raw != NULL) {
			dns_zone_setnotifytype(raw, dns_notifytype_no);
		}
		dns_zone_setnotifytype(zone, notifytype);

		obj = NULL;
		result = named_config_get(maps, "also-notify", &obj);
		if (result == ISC_R_SUCCESS &&
		    (notifytype == dns_notifytype_yes ||
		     notifytype == dns_notifytype_explicit ||
		     (notifytype == dns_notifytype_masteronly &&
		      ztype == dns_zone_primary)))
		{
			dns_ipkeylist_t ipkl;
			dns_ipkeylist_init(&ipkl);

			RETERR(named_config_getipandkeylist(config, "primaries",
							    obj, mctx, &ipkl));
			result = dns_zone_setalsonotifydscpkeys(
				zone, ipkl.addrs, ipkl.dscps, ipkl.keys,
				ipkl.count);
			dns_ipkeylist_clear(mctx, &ipkl);
			RETERR(result);
		} else {
			RETERR(dns_zone_setalsonotify(zone, NULL, 0));
		}

		obj = NULL;
		result = named_config_get(maps, "parental-source", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		RETERR(dns_zone_setparentalsrc4(zone, cfg_obj_assockaddr(obj)));
		dscp = cfg_obj_getdscp(obj);
		if (dscp == -1) {
			dscp = named_g_dscp;
		}
		RETERR(dns_zone_setparentalsrc4dscp(zone, dscp));
		named_add_reserved_dispatch(named_g_server,
					    cfg_obj_assockaddr(obj));

		obj = NULL;
		result = named_config_get(maps, "parental-source-v6", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		RETERR(dns_zone_setparentalsrc6(zone, cfg_obj_assockaddr(obj)));
		dscp = cfg_obj_getdscp(obj);
		if (dscp == -1) {
			dscp = named_g_dscp;
		}
		RETERR(dns_zone_setparentalsrc6dscp(zone, dscp));
		named_add_reserved_dispatch(named_g_server,
					    cfg_obj_assockaddr(obj));

		obj = NULL;
		result = named_config_get(maps, "notify-source", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		RETERR(dns_zone_setnotifysrc4(zone, cfg_obj_assockaddr(obj)));
		dscp = cfg_obj_getdscp(obj);
		if (dscp == -1) {
			dscp = named_g_dscp;
		}
		RETERR(dns_zone_setnotifysrc4dscp(zone, dscp));
		named_add_reserved_dispatch(named_g_server,
					    cfg_obj_assockaddr(obj));

		obj = NULL;
		result = named_config_get(maps, "notify-source-v6", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		RETERR(dns_zone_setnotifysrc6(zone, cfg_obj_assockaddr(obj)));
		dscp = cfg_obj_getdscp(obj);
		if (dscp == -1) {
			dscp = named_g_dscp;
		}
		RETERR(dns_zone_setnotifysrc6dscp(zone, dscp));
		named_add_reserved_dispatch(named_g_server,
					    cfg_obj_assockaddr(obj));

		obj = NULL;
		result = named_config_get(maps, "notify-to-soa", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setoption(zone, DNS_ZONEOPT_NOTIFYTOSOA,
				   cfg_obj_asboolean(obj));

		dns_zone_setisself(zone, isself, named_g_server->interfacemgr);

		RETERR(configure_zone_acl(
			zconfig, vconfig, config, allow_transfer, ac, zone,
			dns_zone_setxfracl, dns_zone_clearxfracl));

		obj = NULL;
		result = named_config_get(maps, "max-transfer-time-out", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setmaxxfrout(zone, cfg_obj_asuint32(obj) * 60);

		obj = NULL;
		result = named_config_get(maps, "max-transfer-idle-out", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setidleout(zone, cfg_obj_asuint32(obj) * 60);

		obj = NULL;
		result = named_config_get(maps, "max-journal-size", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		if (raw != NULL) {
			dns_zone_setjournalsize(raw, -1);
		}
		dns_zone_setjournalsize(zone, -1);
		if (cfg_obj_isstring(obj)) {
			const char *str = cfg_obj_asstring(obj);
			if (strcasecmp(str, "unlimited") == 0) {
				journal_size = DNS_JOURNAL_SIZE_MAX;
			} else {
				INSIST(strcasecmp(str, "default") == 0);
				journal_size = -1;
			}
		} else {
			isc_resourcevalue_t value;
			value = cfg_obj_asuint64(obj);
			if (value > DNS_JOURNAL_SIZE_MAX) {
				cfg_obj_log(obj, named_g_lctx, ISC_LOG_ERROR,
					    "'max-journal-size "
					    "%" PRId64 "' "
					    "is too large",
					    value);
				RETERR(ISC_R_RANGE);
			}
			journal_size = (uint32_t)value;
		}
		if (raw != NULL) {
			dns_zone_setjournalsize(raw, journal_size);
		}
		dns_zone_setjournalsize(zone, journal_size);

		obj = NULL;
		result = named_config_get(maps, "ixfr-from-differences", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		if (cfg_obj_isboolean(obj)) {
			ixfrdiff = cfg_obj_asboolean(obj);
		} else if ((strcasecmp(cfg_obj_asstring(obj), "primary") == 0 ||
			    strcasecmp(cfg_obj_asstring(obj), "master") == 0) &&
			   ztype == dns_zone_primary)
		{
			ixfrdiff = true;
		} else if ((strcasecmp(cfg_obj_asstring(obj), "secondary") ==
				    0 ||
			    strcasecmp(cfg_obj_asstring(obj), "slave") == 0) &&
			   ztype == dns_zone_secondary)
		{
			ixfrdiff = true;
		} else {
			ixfrdiff = false;
		}
		if (raw != NULL) {
			dns_zone_setoption(raw, DNS_ZONEOPT_IXFRFROMDIFFS,
					   true);
			dns_zone_setoption(zone, DNS_ZONEOPT_IXFRFROMDIFFS,
					   false);
		} else {
			dns_zone_setoption(zone, DNS_ZONEOPT_IXFRFROMDIFFS,
					   ixfrdiff);
		}

		obj = NULL;
		result = named_config_get(maps, "max-ixfr-ratio", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		if (cfg_obj_isstring(obj)) {
			dns_zone_setixfrratio(zone, 0);
		} else {
			dns_zone_setixfrratio(zone, cfg_obj_aspercentage(obj));
		}

		obj = NULL;
		result = named_config_get(maps, "request-expire", &obj);
		INSIST(result == ISC_R_SUCCESS);
		dns_zone_setrequestexpire(zone, cfg_obj_asboolean(obj));

		obj = NULL;
		result = named_config_get(maps, "request-ixfr", &obj);
		INSIST(result == ISC_R_SUCCESS);
		dns_zone_setrequestixfr(zone, cfg_obj_asboolean(obj));

		obj = NULL;
		checknames(ztype, maps, &obj);
		INSIST(obj != NULL);
		if (strcasecmp(cfg_obj_asstring(obj), "warn") == 0) {
			fail = false;
			check = true;
		} else if (strcasecmp(cfg_obj_asstring(obj), "fail") == 0) {
			fail = check = true;
		} else if (strcasecmp(cfg_obj_asstring(obj), "ignore") == 0) {
			fail = check = false;
		} else {
			UNREACHABLE();
		}
		if (raw != NULL) {
			dns_zone_setoption(raw, DNS_ZONEOPT_CHECKNAMES, check);
			dns_zone_setoption(raw, DNS_ZONEOPT_CHECKNAMESFAIL,
					   fail);
			dns_zone_setoption(zone, DNS_ZONEOPT_CHECKNAMES, false);
			dns_zone_setoption(zone, DNS_ZONEOPT_CHECKNAMESFAIL,
					   false);
		} else {
			dns_zone_setoption(zone, DNS_ZONEOPT_CHECKNAMES, check);
			dns_zone_setoption(zone, DNS_ZONEOPT_CHECKNAMESFAIL,
					   fail);
		}

		obj = NULL;
		result = named_config_get(maps, "notify-delay", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setnotifydelay(zone, cfg_obj_asuint32(obj));

		obj = NULL;
		result = named_config_get(maps, "check-sibling", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setoption(zone, DNS_ZONEOPT_CHECKSIBLING,
				   cfg_obj_asboolean(obj));

		obj = NULL;
		result = named_config_get(maps, "check-spf", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		if (strcasecmp(cfg_obj_asstring(obj), "warn") == 0) {
			check = true;
		} else if (strcasecmp(cfg_obj_asstring(obj), "ignore") == 0) {
			check = false;
		} else {
			UNREACHABLE();
		}
		dns_zone_setoption(zone, DNS_ZONEOPT_CHECKSPF, check);

		obj = NULL;
		result = named_config_get(maps, "zero-no-soa-ttl", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setzeronosoattl(zone, cfg_obj_asboolean(obj));

		obj = NULL;
		result = named_config_get(maps, "nsec3-test-zone", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setoption(zone, DNS_ZONEOPT_NSEC3TESTZONE,
				   cfg_obj_asboolean(obj));
	} else if (ztype == dns_zone_redirect) {
		dns_zone_setnotifytype(zone, dns_notifytype_no);

		obj = NULL;
		result = named_config_get(maps, "max-journal-size", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setjournalsize(zone, -1);
		if (cfg_obj_isstring(obj)) {
			const char *str = cfg_obj_asstring(obj);
			if (strcasecmp(str, "unlimited") == 0) {
				journal_size = DNS_JOURNAL_SIZE_MAX;
			} else {
				INSIST(strcasecmp(str, "default") == 0);
				journal_size = -1;
			}
		} else {
			isc_resourcevalue_t value;
			value = cfg_obj_asuint64(obj);
			if (value > DNS_JOURNAL_SIZE_MAX) {
				cfg_obj_log(obj, named_g_lctx, ISC_LOG_ERROR,
					    "'max-journal-size "
					    "%" PRId64 "' "
					    "is too large",
					    value);
				RETERR(ISC_R_RANGE);
			}
			journal_size = (uint32_t)value;
		}
		dns_zone_setjournalsize(zone, journal_size);
	}

	if (use_kasp) {
		maxttl = dns_kasp_zonemaxttl(dns_zone_getkasp(zone));
	} else {
		obj = NULL;
		result = named_config_get(maps, "max-zone-ttl", &obj);
		if (result == ISC_R_SUCCESS) {
			if (cfg_obj_isduration(obj)) {
				maxttl = cfg_obj_asduration(obj);
			}
		}
	}
	dns_zone_setmaxttl(zone, maxttl);
	if (raw != NULL) {
		dns_zone_setmaxttl(raw, maxttl);
	}

	/*
	 * Configure update-related options.  These apply to
	 * primary servers only.
	 */
	if (ztype == dns_zone_primary) {
		dns_acl_t *updateacl;

		RETERR(configure_zone_acl(
			zconfig, vconfig, config, allow_update, ac, mayberaw,
			dns_zone_setupdateacl, dns_zone_clearupdateacl));

		updateacl = dns_zone_getupdateacl(mayberaw);
		if (updateacl != NULL && dns_acl_isinsecure(updateacl)) {
			isc_log_write(named_g_lctx, DNS_LOGCATEGORY_SECURITY,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
				      "zone '%s' allows unsigned updates "
				      "from remote hosts, which is insecure",
				      zname);
		}

		RETERR(configure_zone_ssutable(zoptions, mayberaw, zname));
	}

	/*
	 * Configure DNSSEC signing. These apply to primary zones or zones that
	 * use inline-signing (raw != NULL).
	 */
	if (ztype == dns_zone_primary || raw != NULL) {
		const cfg_obj_t *validity, *resign;
		bool allow = false, maint = false;
		bool sigvalinsecs;

		if (use_kasp) {
			if (dns_kasp_nsec3(kasp)) {
				result = dns_zone_setnsec3param(
					zone, 1, dns_kasp_nsec3flags(kasp),
					dns_kasp_nsec3iter(kasp),
					dns_kasp_nsec3saltlen(kasp), NULL, true,
					false);
			} else {
				result = dns_zone_setnsec3param(
					zone, 0, 0, 0, 0, NULL, true, false);
			}
			INSIST(result == ISC_R_SUCCESS);
		}

		if (use_kasp) {
			seconds = (uint32_t)dns_kasp_sigvalidity_dnskey(kasp);
		} else {
			obj = NULL;
			result = named_config_get(maps, "dnskey-sig-validity",
						  &obj);
			INSIST(result == ISC_R_SUCCESS && obj != NULL);
			seconds = cfg_obj_asuint32(obj) * 86400;
		}
		dns_zone_setkeyvalidityinterval(zone, seconds);

		if (use_kasp) {
			seconds = (uint32_t)dns_kasp_sigvalidity(kasp);
			dns_zone_setsigvalidityinterval(zone, seconds);
			seconds = (uint32_t)dns_kasp_sigrefresh(kasp);
			dns_zone_setsigresigninginterval(zone, seconds);
		} else {
			obj = NULL;
			result = named_config_get(maps, "sig-validity-interval",
						  &obj);
			INSIST(result == ISC_R_SUCCESS && obj != NULL);

			sigvalinsecs = ns_server_getoption(
				named_g_server->sctx, NS_SERVER_SIGVALINSECS);
			validity = cfg_tuple_get(obj, "validity");
			seconds = cfg_obj_asuint32(validity);
			if (!sigvalinsecs) {
				seconds *= 86400;
			}
			dns_zone_setsigvalidityinterval(zone, seconds);

			resign = cfg_tuple_get(obj, "re-sign");
			if (cfg_obj_isvoid(resign)) {
				seconds /= 4;
			} else if (!sigvalinsecs) {
				uint32_t r = cfg_obj_asuint32(resign);
				if (seconds > 7 * 86400) {
					seconds = r * 86400;
				} else {
					seconds = r * 3600;
				}
			} else {
				seconds = cfg_obj_asuint32(resign);
			}
			dns_zone_setsigresigninginterval(zone, seconds);
		}

		obj = NULL;
		result = named_config_get(maps, "key-directory", &obj);
		if (result == ISC_R_SUCCESS) {
			filename = cfg_obj_asstring(obj);
			RETERR(dns_zone_setkeydirectory(zone, filename));
		}

		obj = NULL;
		result = named_config_get(maps, "sig-signing-signatures", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setsignatures(zone, cfg_obj_asuint32(obj));

		obj = NULL;
		result = named_config_get(maps, "sig-signing-nodes", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setnodes(zone, cfg_obj_asuint32(obj));

		obj = NULL;
		result = named_config_get(maps, "sig-signing-type", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setprivatetype(zone, cfg_obj_asuint32(obj));

		obj = NULL;
		result = named_config_get(maps, "update-check-ksk", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setoption(zone, DNS_ZONEOPT_UPDATECHECKKSK,
				   cfg_obj_asboolean(obj));
		/*
		 * This setting will be ignored if dnssec-policy is used.
		 * named-checkconf will error if both are configured.
		 */

		obj = NULL;
		result = named_config_get(maps, "dnssec-dnskey-kskonly", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setoption(zone, DNS_ZONEOPT_DNSKEYKSKONLY,
				   cfg_obj_asboolean(obj));
		/*
		 * This setting will be ignored if dnssec-policy is used.
		 * named-checkconf will error if both are configured.
		 */

		obj = NULL;
		result = named_config_get(maps, "dnssec-loadkeys-interval",
					  &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		RETERR(dns_zone_setrefreshkeyinterval(zone,
						      cfg_obj_asuint32(obj)));

		obj = NULL;
		result = cfg_map_get(zoptions, "auto-dnssec", &obj);
		if (kasp != NULL) {
			bool s2i = (strcmp(dns_kasp_getname(kasp),
					   "insecure") != 0);
			dns_zone_setkeyopt(zone, DNS_ZONEKEY_ALLOW, true);
			dns_zone_setkeyopt(zone, DNS_ZONEKEY_CREATE, !s2i);
			dns_zone_setkeyopt(zone, DNS_ZONEKEY_MAINTAIN, true);
		} else if (result == ISC_R_SUCCESS) {
			const char *arg = cfg_obj_asstring(obj);
			if (strcasecmp(arg, "allow") == 0) {
				allow = true;
			} else if (strcasecmp(arg, "maintain") == 0) {
				allow = maint = true;
			} else if (strcasecmp(arg, "off") == 0) {
				/* Default */
			} else {
				UNREACHABLE();
			}
			dns_zone_setkeyopt(zone, DNS_ZONEKEY_ALLOW, allow);
			dns_zone_setkeyopt(zone, DNS_ZONEKEY_CREATE, false);
			dns_zone_setkeyopt(zone, DNS_ZONEKEY_MAINTAIN, maint);
		}
	}

	if (ztype == dns_zone_secondary || ztype == dns_zone_mirror) {
		RETERR(configure_zone_acl(zconfig, vconfig, config,
					  allow_update_forwarding, ac, mayberaw,
					  dns_zone_setforwardacl,
					  dns_zone_clearforwardacl));
	}

	/*%
	 * Configure parental agents, applies to primary and secondary zones.
	 */
	if (ztype == dns_zone_primary || ztype == dns_zone_secondary) {
		obj = NULL;
		(void)cfg_map_get(zoptions, "parental-agents", &obj);
		if (obj != NULL) {
			dns_ipkeylist_t ipkl;
			dns_ipkeylist_init(&ipkl);
			RETERR(named_config_getipandkeylist(
				config, "parental-agents", obj, mctx, &ipkl));
			result = dns_zone_setparentals(zone, ipkl.addrs,
						       ipkl.keys, ipkl.count);
			dns_ipkeylist_clear(mctx, &ipkl);
			RETERR(result);
		} else {
			RETERR(dns_zone_setparentals(zone, NULL, NULL, 0));
		}
	}

	/*%
	 * Primary master functionality.
	 */
	if (ztype == dns_zone_primary) {
		obj = NULL;
		result = named_config_get(maps, "check-wildcard", &obj);
		if (result == ISC_R_SUCCESS) {
			check = cfg_obj_asboolean(obj);
		} else {
			check = false;
		}
		dns_zone_setoption(mayberaw, DNS_ZONEOPT_CHECKWILDCARD, check);

		/*
		 * With map files, the default is ignore duplicate
		 * records.  With other master formats, the default is
		 * taken from the global configuration.
		 */
		obj = NULL;
		if (masterformat != dns_masterformat_map) {
			result = named_config_get(maps, "check-dup-records",
						  &obj);
			INSIST(result == ISC_R_SUCCESS && obj != NULL);
			dupcheck = cfg_obj_asstring(obj);
		} else {
			result = named_config_get(nodefault,
						  "check-dup-records", &obj);
			if (result == ISC_R_SUCCESS) {
				dupcheck = cfg_obj_asstring(obj);
			} else {
				dupcheck = "ignore";
			}
		}
		if (strcasecmp(dupcheck, "warn") == 0) {
			fail = false;
			check = true;
		} else if (strcasecmp(dupcheck, "fail") == 0) {
			fail = check = true;
		} else if (strcasecmp(dupcheck, "ignore") == 0) {
			fail = check = false;
		} else {
			UNREACHABLE();
		}
		dns_zone_setoption(mayberaw, DNS_ZONEOPT_CHECKDUPRR, check);
		dns_zone_setoption(mayberaw, DNS_ZONEOPT_CHECKDUPRRFAIL, fail);

		obj = NULL;
		result = named_config_get(maps, "check-mx", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		if (strcasecmp(cfg_obj_asstring(obj), "warn") == 0) {
			fail = false;
			check = true;
		} else if (strcasecmp(cfg_obj_asstring(obj), "fail") == 0) {
			fail = check = true;
		} else if (strcasecmp(cfg_obj_asstring(obj), "ignore") == 0) {
			fail = check = false;
		} else {
			UNREACHABLE();
		}
		dns_zone_setoption(mayberaw, DNS_ZONEOPT_CHECKMX, check);
		dns_zone_setoption(mayberaw, DNS_ZONEOPT_CHECKMXFAIL, fail);

		/*
		 * With map files, the default is *not* to check
		 * integrity.  With other master formats, the default is
		 * taken from the global configuration.
		 */
		obj = NULL;
		if (masterformat != dns_masterformat_map) {
			result = named_config_get(maps, "check-integrity",
						  &obj);
			INSIST(result == ISC_R_SUCCESS && obj != NULL);
			dns_zone_setoption(mayberaw, DNS_ZONEOPT_CHECKINTEGRITY,
					   cfg_obj_asboolean(obj));
		} else {
			check = false;
			result = named_config_get(nodefault, "check-integrity",
						  &obj);
			if (result == ISC_R_SUCCESS) {
				check = cfg_obj_asboolean(obj);
			}
			dns_zone_setoption(mayberaw, DNS_ZONEOPT_CHECKINTEGRITY,
					   check);
		}

		obj = NULL;
		result = named_config_get(maps, "check-mx-cname", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		if (strcasecmp(cfg_obj_asstring(obj), "warn") == 0) {
			warn = true;
			ignore = false;
		} else if (strcasecmp(cfg_obj_asstring(obj), "fail") == 0) {
			warn = ignore = false;
		} else if (strcasecmp(cfg_obj_asstring(obj), "ignore") == 0) {
			warn = ignore = true;
		} else {
			UNREACHABLE();
		}
		dns_zone_setoption(mayberaw, DNS_ZONEOPT_WARNMXCNAME, warn);
		dns_zone_setoption(mayberaw, DNS_ZONEOPT_IGNOREMXCNAME, ignore);

		obj = NULL;
		result = named_config_get(maps, "check-srv-cname", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		if (strcasecmp(cfg_obj_asstring(obj), "warn") == 0) {
			warn = true;
			ignore = false;
		} else if (strcasecmp(cfg_obj_asstring(obj), "fail") == 0) {
			warn = ignore = false;
		} else if (strcasecmp(cfg_obj_asstring(obj), "ignore") == 0) {
			warn = ignore = true;
		} else {
			UNREACHABLE();
		}
		dns_zone_setoption(mayberaw, DNS_ZONEOPT_WARNSRVCNAME, warn);
		dns_zone_setoption(mayberaw, DNS_ZONEOPT_IGNORESRVCNAME,
				   ignore);

		obj = NULL;
		result = named_config_get(maps, "dnssec-secure-to-insecure",
					  &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setoption(mayberaw, DNS_ZONEOPT_SECURETOINSECURE,
				   cfg_obj_asboolean(obj));

		obj = NULL;
		result = cfg_map_get(zoptions, "dnssec-update-mode", &obj);
		if (result == ISC_R_SUCCESS) {
			const char *arg = cfg_obj_asstring(obj);
			if (strcasecmp(arg, "no-resign") == 0) {
				dns_zone_setkeyopt(zone, DNS_ZONEKEY_NORESIGN,
						   true);
			} else if (strcasecmp(arg, "maintain") == 0) {
				/* Default */
			} else {
				UNREACHABLE();
			}
		}

		obj = NULL;
		result = named_config_get(maps, "serial-update-method", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		if (strcasecmp(cfg_obj_asstring(obj), "unixtime") == 0) {
			dns_zone_setserialupdatemethod(
				zone, dns_updatemethod_unixtime);
		} else if (strcasecmp(cfg_obj_asstring(obj), "date") == 0) {
			dns_zone_setserialupdatemethod(zone,
						       dns_updatemethod_date);
		} else {
			dns_zone_setserialupdatemethod(
				zone, dns_updatemethod_increment);
		}
	}

	/*
	 * Configure slave functionality.
	 */
	switch (ztype) {
	case dns_zone_mirror:
		/*
		 * Disable outgoing zone transfers for mirror zones unless they
		 * are explicitly enabled by zone configuration.
		 */
		obj = NULL;
		(void)cfg_map_get(zoptions, "allow-transfer", &obj);
		if (obj == NULL) {
			dns_acl_t *none;
			RETERR(dns_acl_none(mctx, &none));
			dns_zone_setxfracl(zone, none);
			dns_acl_detach(&none);
		}
		FALLTHROUGH;
	case dns_zone_secondary:
	case dns_zone_stub:
	case dns_zone_redirect:
		count = 0;
		obj = NULL;
		(void)cfg_map_get(zoptions, "primaries", &obj);
		if (obj == NULL) {
			(void)cfg_map_get(zoptions, "masters", &obj);
		}

		/*
		 * Use the built-in primary server list if one was not
		 * explicitly specified and this is a root zone mirror.
		 */
		if (obj == NULL && ztype == dns_zone_mirror &&
		    dns_name_equal(dns_zone_getorigin(zone), dns_rootname))
		{
			result = named_config_getremotesdef(
				named_g_config, "primaries",
				DEFAULT_IANA_ROOT_ZONE_PRIMARIES, &obj);
			RETERR(result);
		}
		if (obj != NULL) {
			dns_ipkeylist_t ipkl;
			dns_ipkeylist_init(&ipkl);

			RETERR(named_config_getipandkeylist(config, "primaries",
							    obj, mctx, &ipkl));
			result = dns_zone_setprimarieswithkeys(
				mayberaw, ipkl.addrs, ipkl.keys, ipkl.count);
			count = ipkl.count;
			dns_ipkeylist_clear(mctx, &ipkl);
			RETERR(result);
		} else {
			result = dns_zone_setprimaries(mayberaw, NULL, 0);
		}
		RETERR(result);

		multi = false;
		if (count > 1) {
			obj = NULL;
			result = named_config_get(maps, "multi-master", &obj);
			INSIST(result == ISC_R_SUCCESS && obj != NULL);
			multi = cfg_obj_asboolean(obj);
		}
		dns_zone_setoption(mayberaw, DNS_ZONEOPT_MULTIMASTER, multi);

		obj = NULL;
		result = named_config_get(maps, "max-transfer-time-in", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setmaxxfrin(mayberaw, cfg_obj_asuint32(obj) * 60);

		obj = NULL;
		result = named_config_get(maps, "max-transfer-idle-in", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setidlein(mayberaw, cfg_obj_asuint32(obj) * 60);

		obj = NULL;
		result = named_config_get(maps, "max-refresh-time", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setmaxrefreshtime(mayberaw, cfg_obj_asuint32(obj));

		obj = NULL;
		result = named_config_get(maps, "min-refresh-time", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setminrefreshtime(mayberaw, cfg_obj_asuint32(obj));

		obj = NULL;
		result = named_config_get(maps, "max-retry-time", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setmaxretrytime(mayberaw, cfg_obj_asuint32(obj));

		obj = NULL;
		result = named_config_get(maps, "min-retry-time", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		dns_zone_setminretrytime(mayberaw, cfg_obj_asuint32(obj));

		obj = NULL;
		result = named_config_get(maps, "transfer-source", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		RETERR(dns_zone_setxfrsource4(mayberaw,
					      cfg_obj_assockaddr(obj)));
		dscp = cfg_obj_getdscp(obj);
		if (dscp == -1) {
			dscp = named_g_dscp;
		}
		RETERR(dns_zone_setxfrsource4dscp(mayberaw, dscp));
		named_add_reserved_dispatch(named_g_server,
					    cfg_obj_assockaddr(obj));

		obj = NULL;
		result = named_config_get(maps, "transfer-source-v6", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		RETERR(dns_zone_setxfrsource6(mayberaw,
					      cfg_obj_assockaddr(obj)));
		dscp = cfg_obj_getdscp(obj);
		if (dscp == -1) {
			dscp = named_g_dscp;
		}
		RETERR(dns_zone_setxfrsource6dscp(mayberaw, dscp));
		named_add_reserved_dispatch(named_g_server,
					    cfg_obj_assockaddr(obj));

		obj = NULL;
		result = named_config_get(maps, "alt-transfer-source", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		RETERR(dns_zone_setaltxfrsource4(mayberaw,
						 cfg_obj_assockaddr(obj)));
		dscp = cfg_obj_getdscp(obj);
		if (dscp == -1) {
			dscp = named_g_dscp;
		}
		RETERR(dns_zone_setaltxfrsource4dscp(mayberaw, dscp));

		obj = NULL;
		result = named_config_get(maps, "alt-transfer-source-v6", &obj);
		INSIST(result == ISC_R_SUCCESS && obj != NULL);
		RETERR(dns_zone_setaltxfrsource6(mayberaw,
						 cfg_obj_assockaddr(obj)));
		dscp = cfg_obj_getdscp(obj);
		if (dscp == -1) {
			dscp = named_g_dscp;
		}
		RETERR(dns_zone_setaltxfrsource6dscp(mayberaw, dscp));

		obj = NULL;
		(void)named_config_get(maps, "use-alt-transfer-source", &obj);
		if (obj == NULL) {
			/*
			 * Default off when views are in use otherwise
			 * on for BIND 8 compatibility.
			 */
			view = dns_zone_getview(zone);
			if (view != NULL && strcmp(view->name, "_default") == 0)
			{
				alt = true;
			} else {
				alt = false;
			}
		} else {
			alt = cfg_obj_asboolean(obj);
		}
		dns_zone_setoption(mayberaw, DNS_ZONEOPT_USEALTXFRSRC, alt);

		obj = NULL;
		(void)named_config_get(maps, "try-tcp-refresh", &obj);
		dns_zone_setoption(mayberaw, DNS_ZONEOPT_TRYTCPREFRESH,
				   cfg_obj_asboolean(obj));
		break;

	case dns_zone_staticstub:
		RETERR(configure_staticstub(zoptions, zone, zname,
					    default_dbtype));
		break;

	default:
		break;
	}

	return (ISC_R_SUCCESS);
}

/*
 * Set up a DLZ zone as writeable
 */
isc_result_t
named_zone_configure_writeable_dlz(dns_dlzdb_t *dlzdatabase, dns_zone_t *zone,
				   dns_rdataclass_t rdclass, dns_name_t *name) {
	dns_db_t *db = NULL;
	isc_time_t now;
	isc_result_t result;

	TIME_NOW(&now);

	dns_zone_settype(zone, dns_zone_dlz);
	result = dns_sdlz_setdb(dlzdatabase, rdclass, name, &db);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	result = dns_zone_dlzpostload(zone, db);
	dns_db_detach(&db);
	return (result);
}

bool
named_zone_reusable(dns_zone_t *zone, const cfg_obj_t *zconfig) {
	const cfg_obj_t *zoptions = NULL;
	const cfg_obj_t *obj = NULL;
	const char *cfilename;
	const char *zfilename;
	dns_zone_t *raw = NULL;
	bool has_raw, inline_signing;
	dns_zonetype_t ztype;

	zoptions = cfg_tuple_get(zconfig, "options");

	/*
	 * We always reconfigure a static-stub zone for simplicity, assuming
	 * the amount of data to be loaded is small.
	 */
	if (zonetype_fromconfig(zoptions) == dns_zone_staticstub) {
		dns_zone_log(zone, ISC_LOG_DEBUG(1),
			     "not reusable: staticstub");
		return (false);
	}

	/* If there's a raw zone, use that for filename and type comparison */
	dns_zone_getraw(zone, &raw);
	if (raw != NULL) {
		zfilename = dns_zone_getfile(raw);
		ztype = dns_zone_gettype(raw);
		dns_zone_detach(&raw);
		has_raw = true;
	} else {
		zfilename = dns_zone_getfile(zone);
		ztype = dns_zone_gettype(zone);
		has_raw = false;
	}

	inline_signing = named_zone_inlinesigning(zconfig);
	if (!inline_signing && has_raw) {
		dns_zone_log(zone, ISC_LOG_DEBUG(1),
			     "not reusable: old zone was inline-signing");
		return (false);
	} else if (inline_signing && !has_raw) {
		dns_zone_log(zone, ISC_LOG_DEBUG(1),
			     "not reusable: old zone was not inline-signing");
		return (false);
	}

	if (zonetype_fromconfig(zoptions) != ztype) {
		dns_zone_log(zone, ISC_LOG_DEBUG(1),
			     "not reusable: type mismatch");
		return (false);
	}

	obj = NULL;
	(void)cfg_map_get(zoptions, "file", &obj);
	if (obj != NULL) {
		cfilename = cfg_obj_asstring(obj);
	} else {
		cfilename = NULL;
	}
	if (!((cfilename == NULL && zfilename == NULL) ||
	      (cfilename != NULL && zfilename != NULL &&
	       strcmp(cfilename, zfilename) == 0)))
	{
		dns_zone_log(zone, ISC_LOG_DEBUG(1),
			     "not reusable: filename mismatch");
		return (false);
	}

	return (true);
}

bool
named_zone_inlinesigning(const cfg_obj_t *zconfig) {
	const cfg_obj_t *zoptions = NULL;
	const cfg_obj_t *signing = NULL;
	bool inline_signing = false;

	zoptions = cfg_tuple_get(zconfig, "options");
	inline_signing = (cfg_map_get(zoptions, "inline-signing", &signing) ==
				  ISC_R_SUCCESS &&
			  cfg_obj_asboolean(signing));

	return (inline_signing);
}
