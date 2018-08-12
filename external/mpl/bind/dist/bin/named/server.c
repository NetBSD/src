/*	$NetBSD: server.c,v 1.2 2018/08/12 13:02:27 christos Exp $	*/

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

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <isc/aes.h>
#include <isc/app.h>
#include <isc/base64.h>
#include <isc/commandline.h>
#include <isc/dir.h>
#include <isc/entropy.h>
#include <isc/file.h>
#include <isc/hash.h>
#include <isc/hex.h>
#include <isc/hmacsha.h>
#include <isc/httpd.h>
#include <isc/lex.h>
#include <isc/meminfo.h>
#include <isc/parseint.h>
#include <isc/platform.h>
#include <isc/portset.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/refcount.h>
#include <isc/resource.h>
#include <isc/sha2.h>
#include <isc/socket.h>
#include <isc/stat.h>
#include <isc/stats.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>
#include <isc/xml.h>

#include <isccfg/grammar.h>
#include <isccfg/namedconf.h>

#include <bind9/check.h>

#include <dns/adb.h>
#include <dns/badcache.h>
#include <dns/cache.h>
#include <dns/catz.h>
#include <dns/db.h>
#include <dns/dispatch.h>
#include <dns/dlz.h>
#include <dns/dnsrps.h>
#include <dns/dns64.h>
#include <dns/dyndb.h>
#include <dns/events.h>
#include <dns/forward.h>
#include <dns/fixedname.h>
#include <dns/journal.h>
#include <dns/keytable.h>
#include <dns/keyvalues.h>
#include <dns/lib.h>
#include <dns/master.h>
#include <dns/masterdump.h>
#include <dns/nta.h>
#include <dns/order.h>
#include <dns/peer.h>
#include <dns/portlist.h>
#include <dns/private.h>
#include <dns/rbt.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/resolver.h>
#include <dns/rootns.h>
#include <dns/rriterator.h>
#include <dns/secalg.h>
#include <dns/soa.h>
#include <dns/stats.h>
#include <dns/tkey.h>
#include <dns/tsig.h>
#include <dns/ttl.h>
#include <dns/view.h>
#include <dns/zone.h>
#include <dns/zt.h>

#include <dst/dst.h>
#include <dst/result.h>

#include <ns/client.h>
#include <ns/listenlist.h>
#include <ns/interfacemgr.h>

#include <named/config.h>
#include <named/control.h>
#ifdef HAVE_GEOIP
#include <named/geoip.h>
#endif /* HAVE_GEOIP */
#include <named/log.h>
#include <named/logconf.h>
#include <named/main.h>
#include <named/os.h>
#include <named/server.h>
#include <named/statschannel.h>
#include <named/tkeyconf.h>
#include <named/tsigconf.h>
#include <named/zoneconf.h>
#ifdef HAVE_LIBSCF
#include <named/smf_globals.h>
#include <stdlib.h>
#endif

#ifdef HAVE_LMDB
#include <lmdb.h>
#define count_newzones count_newzones_db
#define configure_newzones configure_newzones_db
#define dumpzone dumpzone_db
#else  /* HAVE_LMDB */
#define count_newzones count_newzones_file
#define configure_newzones configure_newzones_file
#define dumpzone dumpzone_file
#endif /* HAVE_LMDB */

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

#ifndef SIZE_AS_PERCENT
#define SIZE_AS_PERCENT ((size_t)-2)
#endif

#ifdef TUNE_LARGE
#define RESOLVER_NTASKS 523
#define UDPBUFFERS 32768
#define EXCLBUFFERS 32768
#else
#define RESOLVER_NTASKS 31
#define UDPBUFFERS 1000
#define EXCLBUFFERS 4096
#endif /* TUNE_LARGE */

#define MAX_TCP_TIMEOUT 65535

/*%
 * Check an operation for failure.  Assumes that the function
 * using it has a 'result' variable and a 'cleanup' label.
 */
#define CHECK(op) \
	do { result = (op);					 \
	       if (result != ISC_R_SUCCESS) goto cleanup;	 \
	} while (/*CONSTCOND*/0)

#define TCHECK(op) \
	do { tresult = (op);					 \
		if (tresult != ISC_R_SUCCESS) {			 \
			isc_buffer_clear(*text);		 \
			goto cleanup;	 			 \
		}						 \
	} while (/*CONSTCOND*/ 0)

#define CHECKM(op, msg) \
	do { result = (op);					  \
	       if (result != ISC_R_SUCCESS) {			  \
			isc_log_write(named_g_lctx,		  \
				      NAMED_LOGCATEGORY_GENERAL,	  \
				      NAMED_LOGMODULE_SERVER,	  \
				      ISC_LOG_ERROR,		  \
				      "%s: %s", msg,		  \
				      isc_result_totext(result)); \
			goto cleanup;				  \
		}						  \
	} while (/*CONSTCOND*/0)				  \

#define CHECKMF(op, msg, file) \
	do { result = (op);					  \
	       if (result != ISC_R_SUCCESS) {			  \
			isc_log_write(named_g_lctx,		  \
				      NAMED_LOGCATEGORY_GENERAL,	  \
				      NAMED_LOGMODULE_SERVER,	  \
				      ISC_LOG_ERROR,		  \
				      "%s '%s': %s", msg, file,	  \
				      isc_result_totext(result)); \
			goto cleanup;				  \
		}						  \
	} while (/*CONSTCOND*/0)				  \

#define CHECKFATAL(op, msg) \
	do { result = (op);					  \
	       if (result != ISC_R_SUCCESS)			  \
			fatal(msg, result);			  \
	} while (/*CONSTCOND*/0)				  \

/*%
 * Maximum ADB size for views that share a cache.  Use this limit to suppress
 * the total of memory footprint, which should be the main reason for sharing
 * a cache.  Only effective when a finite max-cache-size is specified.
 * This is currently defined to be 8MB.
 */
#define MAX_ADB_SIZE_FOR_CACHESHARE	8388608U

struct named_dispatch {
	isc_sockaddr_t			addr;
	unsigned int			dispatchgen;
	dns_dispatch_t			*dispatch;
	ISC_LINK(struct named_dispatch)	link;
};

struct named_cache {
	dns_cache_t			*cache;
	dns_view_t			*primaryview;
	isc_boolean_t			needflush;
	isc_boolean_t			adbsizeadjusted;
	dns_rdataclass_t		rdclass;
	ISC_LINK(named_cache_t)		link;
};

struct dumpcontext {
	isc_mem_t			*mctx;
	isc_boolean_t			dumpcache;
	isc_boolean_t			dumpzones;
	isc_boolean_t			dumpadb;
	isc_boolean_t			dumpbad;
	isc_boolean_t			dumpfail;
	FILE				*fp;
	ISC_LIST(struct viewlistentry)	viewlist;
	struct viewlistentry		*view;
	struct zonelistentry		*zone;
	dns_dumpctx_t			*mdctx;
	dns_db_t			*db;
	dns_db_t			*cache;
	isc_task_t			*task;
	dns_dbversion_t			*version;
};

struct viewlistentry {
	dns_view_t			*view;
	ISC_LINK(struct viewlistentry)	link;
	ISC_LIST(struct zonelistentry)	zonelist;
};

struct zonelistentry {
	dns_zone_t			*zone;
	ISC_LINK(struct zonelistentry)	link;
};

/*%
 * Configuration context to retain for each view that allows
 * new zones to be added at runtime.
 */
typedef struct ns_cfgctx {
	isc_mem_t *			mctx;
	cfg_parser_t *			conf_parser;
	cfg_parser_t *			add_parser;
	cfg_obj_t *			config;
	cfg_obj_t *			vconfig;
	cfg_obj_t *			nzf_config;
	cfg_aclconfctx_t *		actx;
} ns_cfgctx_t;

/*%
 * A function to write out added-zone configuration to the new_zone_file
 * specified in 'view'. Maybe called by delete_zoneconf().
 */
typedef isc_result_t (*nzfwriter_t)(const cfg_obj_t *config, dns_view_t *view);

/*%
 * Holds state information for the initial zone loading process.
 * Uses the isc_refcount structure to count the number of views
 * with pending zone loads, dereferencing as each view finishes.
 */
typedef struct {
		named_server_t *server;
		isc_boolean_t reconfig;
		isc_refcount_t refs;
} ns_zoneload_t;

typedef struct {
	named_server_t *server;
} catz_cb_data_t;

typedef struct catz_chgzone_event {
	ISC_EVENT_COMMON(struct catz_chgzone_event);
	dns_catz_entry_t *entry;
	dns_catz_zone_t *origin;
	dns_view_t *view;
	catz_cb_data_t *cbd;
	isc_boolean_t mod;
} catz_chgzone_event_t;

/*
 * These zones should not leak onto the Internet.
 */
const char *empty_zones[] = {
	/* RFC 1918 */
	"10.IN-ADDR.ARPA",
	"16.172.IN-ADDR.ARPA",
	"17.172.IN-ADDR.ARPA",
	"18.172.IN-ADDR.ARPA",
	"19.172.IN-ADDR.ARPA",
	"20.172.IN-ADDR.ARPA",
	"21.172.IN-ADDR.ARPA",
	"22.172.IN-ADDR.ARPA",
	"23.172.IN-ADDR.ARPA",
	"24.172.IN-ADDR.ARPA",
	"25.172.IN-ADDR.ARPA",
	"26.172.IN-ADDR.ARPA",
	"27.172.IN-ADDR.ARPA",
	"28.172.IN-ADDR.ARPA",
	"29.172.IN-ADDR.ARPA",
	"30.172.IN-ADDR.ARPA",
	"31.172.IN-ADDR.ARPA",
	"168.192.IN-ADDR.ARPA",

	/* RFC 6598 */
	"64.100.IN-ADDR.ARPA",
	"65.100.IN-ADDR.ARPA",
	"66.100.IN-ADDR.ARPA",
	"67.100.IN-ADDR.ARPA",
	"68.100.IN-ADDR.ARPA",
	"69.100.IN-ADDR.ARPA",
	"70.100.IN-ADDR.ARPA",
	"71.100.IN-ADDR.ARPA",
	"72.100.IN-ADDR.ARPA",
	"73.100.IN-ADDR.ARPA",
	"74.100.IN-ADDR.ARPA",
	"75.100.IN-ADDR.ARPA",
	"76.100.IN-ADDR.ARPA",
	"77.100.IN-ADDR.ARPA",
	"78.100.IN-ADDR.ARPA",
	"79.100.IN-ADDR.ARPA",
	"80.100.IN-ADDR.ARPA",
	"81.100.IN-ADDR.ARPA",
	"82.100.IN-ADDR.ARPA",
	"83.100.IN-ADDR.ARPA",
	"84.100.IN-ADDR.ARPA",
	"85.100.IN-ADDR.ARPA",
	"86.100.IN-ADDR.ARPA",
	"87.100.IN-ADDR.ARPA",
	"88.100.IN-ADDR.ARPA",
	"89.100.IN-ADDR.ARPA",
	"90.100.IN-ADDR.ARPA",
	"91.100.IN-ADDR.ARPA",
	"92.100.IN-ADDR.ARPA",
	"93.100.IN-ADDR.ARPA",
	"94.100.IN-ADDR.ARPA",
	"95.100.IN-ADDR.ARPA",
	"96.100.IN-ADDR.ARPA",
	"97.100.IN-ADDR.ARPA",
	"98.100.IN-ADDR.ARPA",
	"99.100.IN-ADDR.ARPA",
	"100.100.IN-ADDR.ARPA",
	"101.100.IN-ADDR.ARPA",
	"102.100.IN-ADDR.ARPA",
	"103.100.IN-ADDR.ARPA",
	"104.100.IN-ADDR.ARPA",
	"105.100.IN-ADDR.ARPA",
	"106.100.IN-ADDR.ARPA",
	"107.100.IN-ADDR.ARPA",
	"108.100.IN-ADDR.ARPA",
	"109.100.IN-ADDR.ARPA",
	"110.100.IN-ADDR.ARPA",
	"111.100.IN-ADDR.ARPA",
	"112.100.IN-ADDR.ARPA",
	"113.100.IN-ADDR.ARPA",
	"114.100.IN-ADDR.ARPA",
	"115.100.IN-ADDR.ARPA",
	"116.100.IN-ADDR.ARPA",
	"117.100.IN-ADDR.ARPA",
	"118.100.IN-ADDR.ARPA",
	"119.100.IN-ADDR.ARPA",
	"120.100.IN-ADDR.ARPA",
	"121.100.IN-ADDR.ARPA",
	"122.100.IN-ADDR.ARPA",
	"123.100.IN-ADDR.ARPA",
	"124.100.IN-ADDR.ARPA",
	"125.100.IN-ADDR.ARPA",
	"126.100.IN-ADDR.ARPA",
	"127.100.IN-ADDR.ARPA",

	/* RFC 5735 and RFC 5737 */
	"0.IN-ADDR.ARPA",	/* THIS NETWORK */
	"127.IN-ADDR.ARPA",	/* LOOPBACK */
	"254.169.IN-ADDR.ARPA",	/* LINK LOCAL */
	"2.0.192.IN-ADDR.ARPA",	/* TEST NET */
	"100.51.198.IN-ADDR.ARPA",	/* TEST NET 2 */
	"113.0.203.IN-ADDR.ARPA",	/* TEST NET 3 */
	"255.255.255.255.IN-ADDR.ARPA",	/* BROADCAST */

	/* Local IPv6 Unicast Addresses */
	"0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.IP6.ARPA",
	"1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.IP6.ARPA",
	/* LOCALLY ASSIGNED LOCAL ADDRESS SCOPE */
	"D.F.IP6.ARPA",
	"8.E.F.IP6.ARPA",	/* LINK LOCAL */
	"9.E.F.IP6.ARPA",	/* LINK LOCAL */
	"A.E.F.IP6.ARPA",	/* LINK LOCAL */
	"B.E.F.IP6.ARPA",	/* LINK LOCAL */

	/* Example Prefix, RFC 3849. */
	"8.B.D.0.1.0.0.2.IP6.ARPA",

	/* RFC 7534 */
	"EMPTY.AS112.ARPA",

	/* RFC 8375 */
	"HOME.ARPA",

	NULL
};

ISC_PLATFORM_NORETURN_PRE static void
fatal(const char *msg, isc_result_t result) ISC_PLATFORM_NORETURN_POST;

static void
named_server_reload(isc_task_t *task, isc_event_t *event);

static isc_result_t
ns_listenelt_fromconfig(const cfg_obj_t *listener, const cfg_obj_t *config,
			cfg_aclconfctx_t *actx, isc_mem_t *mctx,
			isc_uint16_t family, ns_listenelt_t **target);
static isc_result_t
ns_listenlist_fromconfig(const cfg_obj_t *listenlist, const cfg_obj_t *config,
			 cfg_aclconfctx_t *actx, isc_mem_t *mctx,
			 isc_uint16_t family, ns_listenlist_t **target);

static isc_result_t
configure_forward(const cfg_obj_t *config, dns_view_t *view,
		  const dns_name_t *origin, const cfg_obj_t *forwarders,
		  const cfg_obj_t *forwardtype);

static isc_result_t
configure_alternates(const cfg_obj_t *config, dns_view_t *view,
		     const cfg_obj_t *alternates);

static isc_result_t
configure_zone(const cfg_obj_t *config, const cfg_obj_t *zconfig,
	       const cfg_obj_t *vconfig, isc_mem_t *mctx, dns_view_t *view,
	       dns_viewlist_t *viewlist, cfg_aclconfctx_t *aclconf,
	       isc_boolean_t added, isc_boolean_t old_rpz_ok,
	       isc_boolean_t modify);

static isc_result_t
configure_newzones(dns_view_t *view, cfg_obj_t *config, cfg_obj_t *vconfig,
		   isc_mem_t *mctx, cfg_aclconfctx_t *actx);

static isc_result_t
add_keydata_zone(dns_view_t *view, const char *directory, isc_mem_t *mctx);

static void
end_reserved_dispatches(named_server_t *server, isc_boolean_t all);

static void
newzone_cfgctx_destroy(void **cfgp);

static inline isc_result_t
putstr(isc_buffer_t **b, const char *str);

static isc_result_t
putmem(isc_buffer_t **b, const char *str, size_t len);

static isc_result_t
putuint8(isc_buffer_t **b, isc_uint8_t val);

static inline isc_result_t
putnull(isc_buffer_t **b);

static int
count_zones(const cfg_obj_t *conf);

#ifdef HAVE_LMDB
static isc_result_t
migrate_nzf(dns_view_t *view);

static isc_result_t
nzd_writable(dns_view_t *view);

static isc_result_t
nzd_open(dns_view_t *view, unsigned int flags, MDB_txn **txnp, MDB_dbi *dbi);

static isc_result_t
nzd_env_reopen(dns_view_t *view);

static void
nzd_env_close(dns_view_t *view);

static isc_result_t
nzd_close(MDB_txn **txnp, isc_boolean_t commit);

static isc_result_t
nzd_count(dns_view_t *view, int *countp);
#else
static isc_result_t
nzf_append(dns_view_t *view, const cfg_obj_t *zconfig);
#endif

/*%
 * Configure a single view ACL at '*aclp'.  Get its configuration from
 * 'vconfig' (for per-view configuration) and maybe from 'config'
 */
static isc_result_t
configure_view_acl(const cfg_obj_t *vconfig, const cfg_obj_t *config,
		   const cfg_obj_t *gconfig, const char *aclname,
		   const char *acltuplename, cfg_aclconfctx_t *actx,
		   isc_mem_t *mctx, dns_acl_t **aclp)
{
	isc_result_t result;
	const cfg_obj_t *maps[4];
	const cfg_obj_t *aclobj = NULL;
	int i = 0;

	if (*aclp != NULL) {
		dns_acl_detach(aclp);
	}
	if (vconfig != NULL) {
		maps[i++] = cfg_tuple_get(vconfig, "options");
	}
	if (config != NULL) {
		const cfg_obj_t *options = NULL;
		(void)cfg_map_get(config, "options", &options);
		if (options != NULL) {
			maps[i++] = options;
		}
	}
	if (gconfig != NULL) {
		const cfg_obj_t *options = NULL;
		(void)cfg_map_get(gconfig, "options", &options);
		if (options != NULL) {
			maps[i++] = options;
		}
	}
	maps[i] = NULL;

	(void)named_config_get(maps, aclname, &aclobj);
	if (aclobj == NULL) {
		/*
		 * No value available.	*aclp == NULL.
		 */
		return (ISC_R_SUCCESS);
	}

	if (acltuplename != NULL) {
		/*
		 * If the ACL is given in an optional tuple, retrieve it.
		 * The parser should have ensured that a valid object be
		 * returned.
		 */
		aclobj = cfg_tuple_get(aclobj, acltuplename);
	}

	result = cfg_acl_fromconfig(aclobj, config, named_g_lctx,
				    actx, mctx, 0, aclp);

	return (result);
}

/*%
 * Configure a sortlist at '*aclp'.  Essentially the same as
 * configure_view_acl() except it calls cfg_acl_fromconfig with a
 * nest_level value of 2.
 */
static isc_result_t
configure_view_sortlist(const cfg_obj_t *vconfig, const cfg_obj_t *config,
			cfg_aclconfctx_t *actx, isc_mem_t *mctx,
			dns_acl_t **aclp)
{
	isc_result_t result;
	const cfg_obj_t *maps[3];
	const cfg_obj_t *aclobj = NULL;
	int i = 0;

	if (*aclp != NULL)
		dns_acl_detach(aclp);
	if (vconfig != NULL)
		maps[i++] = cfg_tuple_get(vconfig, "options");
	if (config != NULL) {
		const cfg_obj_t *options = NULL;
		(void)cfg_map_get(config, "options", &options);
		if (options != NULL)
			maps[i++] = options;
	}
	maps[i] = NULL;

	(void)named_config_get(maps, "sortlist", &aclobj);
	if (aclobj == NULL)
		return (ISC_R_SUCCESS);

	/*
	 * Use a nest level of 3 for the "top level" of the sortlist;
	 * this means each entry in the top three levels will be stored
	 * as lists of separate, nested ACLs, rather than merged together
	 * into IP tables as is usually done with ACLs.
	 */
	result = cfg_acl_fromconfig(aclobj, config, named_g_lctx,
				    actx, mctx, 3, aclp);

	return (result);
}

static isc_result_t
configure_view_nametable(const cfg_obj_t *vconfig, const cfg_obj_t *config,
			 const char *confname, const char *conftuplename,
			 isc_mem_t *mctx, dns_rbt_t **rbtp)
{
	isc_result_t result;
	const cfg_obj_t *maps[3];
	const cfg_obj_t *obj = NULL;
	const cfg_listelt_t *element;
	int i = 0;
	dns_fixedname_t fixed;
	dns_name_t *name;
	isc_buffer_t b;
	const char *str;
	const cfg_obj_t *nameobj;

	if (*rbtp != NULL)
		dns_rbt_destroy(rbtp);
	if (vconfig != NULL)
		maps[i++] = cfg_tuple_get(vconfig, "options");
	if (config != NULL) {
		const cfg_obj_t *options = NULL;
		(void)cfg_map_get(config, "options", &options);
		if (options != NULL)
			maps[i++] = options;
	}
	maps[i] = NULL;

	(void)named_config_get(maps, confname, &obj);
	if (obj == NULL)
		/*
		 * No value available.	*rbtp == NULL.
		 */
		return (ISC_R_SUCCESS);

	if (conftuplename != NULL) {
		obj = cfg_tuple_get(obj, conftuplename);
		if (cfg_obj_isvoid(obj))
			return (ISC_R_SUCCESS);
	}

	result = dns_rbt_create(mctx, NULL, NULL, rbtp);
	if (result != ISC_R_SUCCESS)
		return (result);

	name = dns_fixedname_initname(&fixed);
	for (element = cfg_list_first(obj);
	     element != NULL;
	     element = cfg_list_next(element)) {
		nameobj = cfg_listelt_value(element);
		str = cfg_obj_asstring(nameobj);
		isc_buffer_constinit(&b, str, strlen(str));
		isc_buffer_add(&b, strlen(str));
		CHECK(dns_name_fromtext(name, &b, dns_rootname, 0, NULL));
		/*
		 * We don't need the node data, but need to set dummy data to
		 * avoid a partial match with an empty node.  For example, if
		 * we have foo.example.com and bar.example.com, we'd get a match
		 * for baz.example.com, which is not the expected result.
		 * We simply use (void *)1 as the dummy data.
		 */
		result = dns_rbt_addname(*rbtp, name, (void *)1);
		if (result != ISC_R_SUCCESS) {
			cfg_obj_log(nameobj, named_g_lctx, ISC_LOG_ERROR,
				    "failed to add %s for %s: %s",
				    str, confname, isc_result_totext(result));
			goto cleanup;
		}

	}

	return (result);

  cleanup:
	dns_rbt_destroy(rbtp);
	return (result);

}

static isc_result_t
dstkey_fromconfig(const cfg_obj_t *vconfig, const cfg_obj_t *key,
		  isc_boolean_t managed, dst_key_t **target, isc_mem_t *mctx)
{
	dns_rdataclass_t viewclass;
	dns_rdata_dnskey_t keystruct;
	isc_uint32_t flags, proto, alg;
	const char *keystr, *keynamestr;
	unsigned char keydata[4096];
	isc_buffer_t keydatabuf;
	unsigned char rrdata[4096];
	isc_buffer_t rrdatabuf;
	isc_region_t r;
	dns_fixedname_t fkeyname;
	dns_name_t *keyname;
	isc_buffer_t namebuf;
	isc_result_t result;
	dst_key_t *dstkey = NULL;

	INSIST(target != NULL && *target == NULL);

	flags = cfg_obj_asuint32(cfg_tuple_get(key, "flags"));
	proto = cfg_obj_asuint32(cfg_tuple_get(key, "protocol"));
	alg = cfg_obj_asuint32(cfg_tuple_get(key, "algorithm"));
	keyname = dns_fixedname_name(&fkeyname);
	keynamestr = cfg_obj_asstring(cfg_tuple_get(key, "name"));

	if (managed) {
		const char *initmethod;
		initmethod = cfg_obj_asstring(cfg_tuple_get(key, "init"));

		if (strcasecmp(initmethod, "initial-key") != 0) {
			cfg_obj_log(key, named_g_lctx, ISC_LOG_ERROR,
				    "managed key '%s': "
				    "invalid initialization method '%s'",
				    keynamestr, initmethod);
			result = ISC_R_FAILURE;
			goto cleanup;
		}
	}

	if (vconfig == NULL)
		viewclass = dns_rdataclass_in;
	else {
		const cfg_obj_t *classobj = cfg_tuple_get(vconfig, "class");
		CHECK(named_config_getclass(classobj, dns_rdataclass_in,
					 &viewclass));
	}
	keystruct.common.rdclass = viewclass;
	keystruct.common.rdtype = dns_rdatatype_dnskey;
	/*
	 * The key data in keystruct is not dynamically allocated.
	 */
	keystruct.mctx = NULL;

	ISC_LINK_INIT(&keystruct.common, link);

	if (flags > 0xffff)
		CHECKM(ISC_R_RANGE, "key flags");
	if (proto > 0xff)
		CHECKM(ISC_R_RANGE, "key protocol");
	if (alg > 0xff)
		CHECKM(ISC_R_RANGE, "key algorithm");
	keystruct.flags = (isc_uint16_t)flags;
	keystruct.protocol = (isc_uint8_t)proto;
	keystruct.algorithm = (isc_uint8_t)alg;

	isc_buffer_init(&keydatabuf, keydata, sizeof(keydata));
	isc_buffer_init(&rrdatabuf, rrdata, sizeof(rrdata));

	keystr = cfg_obj_asstring(cfg_tuple_get(key, "key"));
	CHECK(isc_base64_decodestring(keystr, &keydatabuf));
	isc_buffer_usedregion(&keydatabuf, &r);
	keystruct.datalen = r.length;
	keystruct.data = r.base;

	if ((keystruct.algorithm == DST_ALG_RSASHA1 ||
	     keystruct.algorithm == DST_ALG_RSAMD5) &&
	    r.length > 1 && r.base[0] == 1 && r.base[1] == 3)
		cfg_obj_log(key, named_g_lctx, ISC_LOG_WARNING,
			    "%s key '%s' has a weak exponent",
			    managed ? "managed" : "trusted",
			    keynamestr);

	CHECK(dns_rdata_fromstruct(NULL,
				   keystruct.common.rdclass,
				   keystruct.common.rdtype,
				   &keystruct, &rrdatabuf));
	dns_fixedname_init(&fkeyname);
	isc_buffer_constinit(&namebuf, keynamestr, strlen(keynamestr));
	isc_buffer_add(&namebuf, strlen(keynamestr));
	CHECK(dns_name_fromtext(keyname, &namebuf, dns_rootname, 0, NULL));
	CHECK(dst_key_fromdns(keyname, viewclass, &rrdatabuf,
			      mctx, &dstkey));

	*target = dstkey;
	return (ISC_R_SUCCESS);

 cleanup:
	if (result == DST_R_NOCRYPTO) {
		cfg_obj_log(key, named_g_lctx, ISC_LOG_ERROR,
			    "ignoring %s key for '%s': no crypto support",
			    managed ? "managed" : "trusted",
			    keynamestr);
	} else if (result == DST_R_UNSUPPORTEDALG) {
		cfg_obj_log(key, named_g_lctx, ISC_LOG_WARNING,
			    "skipping %s key for '%s': %s",
			    managed ? "managed" : "trusted",
			    keynamestr, isc_result_totext(result));
	} else {
		cfg_obj_log(key, named_g_lctx, ISC_LOG_ERROR,
			    "configuring %s key for '%s': %s",
			    managed ? "managed" : "trusted",
			    keynamestr, isc_result_totext(result));
		result = ISC_R_FAILURE;
	}

	if (dstkey != NULL)
		dst_key_free(&dstkey);

	return (result);
}

/*
 * Load keys from configuration into key table. If 'keyname' is specified,
 * only load keys matching that name. If 'managed' is true, load the key as
 * an initializing key.
 */
static isc_result_t
load_view_keys(const cfg_obj_t *keys, const cfg_obj_t *vconfig,
	       dns_view_t *view, isc_boolean_t managed,
	       const dns_name_t *keyname, isc_mem_t *mctx)
{
	const cfg_listelt_t *elt, *elt2;
	const cfg_obj_t *key, *keylist;
	dst_key_t *dstkey = NULL;
	isc_result_t result;
	dns_keytable_t *secroots = NULL;

	CHECK(dns_view_getsecroots(view, &secroots));

	for (elt = cfg_list_first(keys);
	     elt != NULL;
	     elt = cfg_list_next(elt))
	{
		keylist = cfg_listelt_value(elt);

		for (elt2 = cfg_list_first(keylist);
		     elt2 != NULL;
		     elt2 = cfg_list_next(elt2))
		{
			key = cfg_listelt_value(elt2);
			result = dstkey_fromconfig(vconfig, key, managed,
						   &dstkey, mctx);
			if (result ==  DST_R_UNSUPPORTEDALG) {
				result = ISC_R_SUCCESS;
				continue;
			}
			if (result != ISC_R_SUCCESS) {
				goto cleanup;
			}

			/*
			 * If keyname was specified, we only add that key.
			 */
			if (keyname != NULL &&
			    !dns_name_equal(keyname, dst_key_name(dstkey)))
			{
				dst_key_free(&dstkey);
				continue;
			}

			/*
			 * This key is taken from the configuration, so
			 * if it's a managed key then it's an
			 * initializing key; that's why 'managed'
			 * is duplicated below.
			 */
			CHECK(dns_keytable_add2(secroots, managed,
						managed, &dstkey));
		}
	}

 cleanup:
	if (dstkey != NULL) {
		dst_key_free(&dstkey);
	}
	if (secroots != NULL) {
		dns_keytable_detach(&secroots);
	}
	if (result == DST_R_NOCRYPTO) {
		result = ISC_R_SUCCESS;
	}
	return (result);
}

/*%
 * Check whether a key has been successfully loaded.
 */
static isc_boolean_t
keyloaded(dns_view_t *view, const dns_name_t *name) {
	isc_result_t result;
	dns_keytable_t *secroots = NULL;
	dns_keynode_t *keynode = NULL;

	result = dns_view_getsecroots(view, &secroots);
	if (result != ISC_R_SUCCESS)
		return (ISC_FALSE);

	result = dns_keytable_find(secroots, name, &keynode);

	if (keynode != NULL)
		dns_keytable_detachkeynode(secroots, &keynode);
	if (secroots != NULL)
		dns_keytable_detach(&secroots);

	return (ISC_TF(result == ISC_R_SUCCESS));
}

/*%
 * Configure DNSSEC keys for a view.
 *
 * The per-view configuration values and the server-global defaults are read
 * from 'vconfig' and 'config'.
 */
static isc_result_t
configure_view_dnsseckeys(dns_view_t *view, const cfg_obj_t *vconfig,
			  const cfg_obj_t *config, const cfg_obj_t *bindkeys,
			  isc_boolean_t auto_root, isc_mem_t *mctx)
{
	isc_result_t result = ISC_R_SUCCESS;
	const cfg_obj_t *view_keys = NULL;
	const cfg_obj_t *global_keys = NULL;
	const cfg_obj_t *view_managed_keys = NULL;
	const cfg_obj_t *global_managed_keys = NULL;
	const cfg_obj_t *maps[4];
	const cfg_obj_t *voptions = NULL;
	const cfg_obj_t *options = NULL;
	const cfg_obj_t *obj = NULL;
	const char *directory;
	int i = 0;

	/* We don't need trust anchors for the _bind view */
	if (strcmp(view->name, "_bind") == 0 &&
	    view->rdclass == dns_rdataclass_chaos) {
		return (ISC_R_SUCCESS);
	}

	if (vconfig != NULL) {
		voptions = cfg_tuple_get(vconfig, "options");
		if (voptions != NULL) {
			(void) cfg_map_get(voptions, "trusted-keys",
					   &view_keys);
			(void) cfg_map_get(voptions, "managed-keys",
					   &view_managed_keys);
			maps[i++] = voptions;
		}
	}

	if (config != NULL) {
		(void)cfg_map_get(config, "trusted-keys", &global_keys);
		(void)cfg_map_get(config, "managed-keys", &global_managed_keys);
		(void)cfg_map_get(config, "options", &options);
		if (options != NULL) {
			maps[i++] = options;
		}
	}

	maps[i++] = named_g_defaults;
	maps[i] = NULL;

	result = dns_view_initsecroots(view, mctx);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "couldn't create keytable");
		return (ISC_R_UNEXPECTED);
	}

	result = dns_view_initntatable(view, named_g_taskmgr, named_g_timermgr);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "couldn't create NTA table");
		return (ISC_R_UNEXPECTED);
	}

	if (auto_root && view->rdclass == dns_rdataclass_in) {
		const cfg_obj_t *builtin_keys = NULL;
		const cfg_obj_t *builtin_managed_keys = NULL;

		/*
		 * If bind.keys exists and is populated, it overrides
		 * the managed-keys clause hard-coded in named_g_config.
		 */
		if (bindkeys != NULL) {
			isc_log_write(named_g_lctx, DNS_LOGCATEGORY_SECURITY,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
				      "obtaining root key for view %s "
				      "from '%s'",
				      view->name, named_g_server->bindkeysfile);

			(void)cfg_map_get(bindkeys, "trusted-keys",
					  &builtin_keys);
			(void)cfg_map_get(bindkeys, "managed-keys",
					  &builtin_managed_keys);

			if ((builtin_keys == NULL) &&
			    (builtin_managed_keys == NULL))
				isc_log_write(named_g_lctx,
					      DNS_LOGCATEGORY_SECURITY,
					      NAMED_LOGMODULE_SERVER,
					      ISC_LOG_WARNING,
					      "dnssec-validation auto: "
					      "WARNING: root zone key "
					      "not found");
		}

		if ((builtin_keys == NULL) &&
		    (builtin_managed_keys == NULL))
		{
			isc_log_write(named_g_lctx, DNS_LOGCATEGORY_SECURITY,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
				      "using built-in root key for view %s",
				      view->name);

			(void)cfg_map_get(named_g_config, "trusted-keys",
					  &builtin_keys);
			(void)cfg_map_get(named_g_config, "managed-keys",
					  &builtin_managed_keys);
		}

		if (builtin_keys != NULL)
			CHECK(load_view_keys(builtin_keys, vconfig, view,
					     ISC_FALSE, dns_rootname, mctx));
		if (builtin_managed_keys != NULL)
			CHECK(load_view_keys(builtin_managed_keys, vconfig,
					     view, ISC_TRUE, dns_rootname,
					     mctx));

		if (!keyloaded(view, dns_rootname)) {
			isc_log_write(named_g_lctx, DNS_LOGCATEGORY_SECURITY,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "root key not loaded");
			result = ISC_R_FAILURE;
			goto cleanup;
		}
	}

	CHECK(load_view_keys(view_keys, vconfig, view, ISC_FALSE,
			     NULL, mctx));
	CHECK(load_view_keys(view_managed_keys, vconfig, view, ISC_TRUE,
			     NULL, mctx));

	if (view->rdclass == dns_rdataclass_in) {
		CHECK(load_view_keys(global_keys, vconfig, view, ISC_FALSE,
				     NULL, mctx));
		CHECK(load_view_keys(global_managed_keys, vconfig, view,
				     ISC_TRUE, NULL, mctx));
	}

	/*
	 * Add key zone for managed keys.
	 */
	obj = NULL;
	(void)named_config_get(maps, "managed-keys-directory", &obj);
	directory = (obj != NULL ? cfg_obj_asstring(obj) : NULL);
	if (directory != NULL)
		result = isc_file_isdirectory(directory);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(named_g_lctx, DNS_LOGCATEGORY_SECURITY,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "invalid managed-keys-directory %s: %s",
			      directory, isc_result_totext(result));
		goto cleanup;

	} else if (directory != NULL) {
		if (!isc_file_isdirwritable(directory)) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "managed-keys-directory '%s' "
				      "is not writable", directory);
			result = ISC_R_NOPERM;
			goto cleanup;
		}
	}

	CHECK(add_keydata_zone(view, directory, named_g_mctx));

  cleanup:
	return (result);
}

static isc_result_t
mustbesecure(const cfg_obj_t *mbs, dns_resolver_t *resolver) {
	const cfg_listelt_t *element;
	const cfg_obj_t *obj;
	const char *str;
	dns_fixedname_t fixed;
	dns_name_t *name;
	isc_boolean_t value;
	isc_result_t result;
	isc_buffer_t b;

	name = dns_fixedname_initname(&fixed);
	for (element = cfg_list_first(mbs);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		obj = cfg_listelt_value(element);
		str = cfg_obj_asstring(cfg_tuple_get(obj, "name"));
		isc_buffer_constinit(&b, str, strlen(str));
		isc_buffer_add(&b, strlen(str));
		CHECK(dns_name_fromtext(name, &b, dns_rootname, 0, NULL));
		value = cfg_obj_asboolean(cfg_tuple_get(obj, "value"));
		CHECK(dns_resolver_setmustbesecure(resolver, name, value));
	}

	result = ISC_R_SUCCESS;

 cleanup:
	return (result);
}

/*%
 * Get a dispatch appropriate for the resolver of a given view.
 */
static isc_result_t
get_view_querysource_dispatch(const cfg_obj_t **maps, int af,
			      dns_dispatch_t **dispatchp, isc_dscp_t *dscpp,
			      isc_boolean_t is_firstview)
{
	isc_result_t result = ISC_R_FAILURE;
	dns_dispatch_t *disp;
	isc_sockaddr_t sa;
	unsigned int attrs, attrmask;
	const cfg_obj_t *obj = NULL;
	unsigned int maxdispatchbuffers = UDPBUFFERS;
	isc_dscp_t dscp = -1;

	switch (af) {
	case AF_INET:
		result = named_config_get(maps, "query-source", &obj);
		INSIST(result == ISC_R_SUCCESS);
		break;
	case AF_INET6:
		result = named_config_get(maps, "query-source-v6", &obj);
		INSIST(result == ISC_R_SUCCESS);
		break;
	default:
		INSIST(0);
	}

	sa = *(cfg_obj_assockaddr(obj));
	INSIST(isc_sockaddr_pf(&sa) == af);

	dscp = cfg_obj_getdscp(obj);
	if (dscp != -1 && dscpp != NULL)
		*dscpp = dscp;

	/*
	 * If we don't support this address family, we're done!
	 */
	switch (af) {
	case AF_INET:
		result = isc_net_probeipv4();
		break;
	case AF_INET6:
		result = isc_net_probeipv6();
		break;
	default:
		INSIST(0);
	}
	if (result != ISC_R_SUCCESS)
		return (ISC_R_SUCCESS);

	/*
	 * Try to find a dispatcher that we can share.
	 */
	attrs = 0;
	attrs |= DNS_DISPATCHATTR_UDP;
	switch (af) {
	case AF_INET:
		attrs |= DNS_DISPATCHATTR_IPV4;
		break;
	case AF_INET6:
		attrs |= DNS_DISPATCHATTR_IPV6;
		break;
	}
	if (isc_sockaddr_getport(&sa) == 0) {
		attrs |= DNS_DISPATCHATTR_EXCLUSIVE;
		maxdispatchbuffers = EXCLBUFFERS;
	} else {
		INSIST(obj != NULL);
		if (is_firstview) {
			cfg_obj_log(obj, named_g_lctx, ISC_LOG_INFO,
				    "using specific query-source port "
				    "suppresses port randomization and can be "
				    "insecure.");
		}
	}

	attrmask = 0;
	attrmask |= DNS_DISPATCHATTR_UDP;
	attrmask |= DNS_DISPATCHATTR_TCP;
	attrmask |= DNS_DISPATCHATTR_IPV4;
	attrmask |= DNS_DISPATCHATTR_IPV6;

	disp = NULL;
	result = dns_dispatch_getudp(named_g_dispatchmgr, named_g_socketmgr,
				     named_g_taskmgr, &sa, 4096,
				     maxdispatchbuffers, 32768, 16411, 16433,
				     attrs, attrmask, &disp);
	if (result != ISC_R_SUCCESS) {
		isc_sockaddr_t any;
		char buf[ISC_SOCKADDR_FORMATSIZE];

		switch (af) {
		case AF_INET:
			isc_sockaddr_any(&any);
			break;
		case AF_INET6:
			isc_sockaddr_any6(&any);
			break;
		}
		if (isc_sockaddr_equal(&sa, &any))
			return (ISC_R_SUCCESS);
		isc_sockaddr_format(&sa, buf, sizeof(buf));
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "could not get query source dispatcher (%s)",
			      buf);
		return (result);
	}

	*dispatchp = disp;

	return (ISC_R_SUCCESS);
}

static isc_result_t
configure_order(dns_order_t *order, const cfg_obj_t *ent) {
	dns_rdataclass_t rdclass;
	dns_rdatatype_t rdtype;
	const cfg_obj_t *obj;
	dns_fixedname_t fixed;
	unsigned int mode = 0;
	const char *str;
	isc_buffer_t b;
	isc_result_t result;
	isc_boolean_t addroot;

	result = named_config_getclass(cfg_tuple_get(ent, "class"),
				    dns_rdataclass_any, &rdclass);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = named_config_gettype(cfg_tuple_get(ent, "type"),
				   dns_rdatatype_any, &rdtype);
	if (result != ISC_R_SUCCESS)
		return (result);

	obj = cfg_tuple_get(ent, "name");
	if (cfg_obj_isstring(obj))
		str = cfg_obj_asstring(obj);
	else
		str = "*";
	addroot = ISC_TF(strcmp(str, "*") == 0);
	isc_buffer_constinit(&b, str, strlen(str));
	isc_buffer_add(&b, strlen(str));
	dns_fixedname_init(&fixed);
	result = dns_name_fromtext(dns_fixedname_name(&fixed), &b,
				   dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS)
		return (result);

	obj = cfg_tuple_get(ent, "ordering");
	INSIST(cfg_obj_isstring(obj));
	str = cfg_obj_asstring(obj);
	if (!strcasecmp(str, "fixed"))
#if DNS_RDATASET_FIXED
		mode = DNS_RDATASETATTR_FIXEDORDER;
#else
		mode = DNS_RDATASETATTR_CYCLIC;
#endif /* DNS_RDATASET_FIXED */
	else if (!strcasecmp(str, "random"))
		mode = DNS_RDATASETATTR_RANDOMIZE;
	else if (!strcasecmp(str, "cyclic"))
		mode = DNS_RDATASETATTR_CYCLIC;
	else if (!strcasecmp(str, "none"))
		mode = DNS_RDATASETATTR_NONE;
	else
		INSIST(0);

	/*
	 * "*" should match everything including the root (BIND 8 compat).
	 * As dns_name_matcheswildcard(".", "*.") returns FALSE add a
	 * explicit entry for "." when the name is "*".
	 */
	if (addroot) {
		result = dns_order_add(order, dns_rootname,
				       rdtype, rdclass, mode);
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	return (dns_order_add(order, dns_fixedname_name(&fixed),
			      rdtype, rdclass, mode));
}

static isc_result_t
configure_peer(const cfg_obj_t *cpeer, isc_mem_t *mctx, dns_peer_t **peerp) {
	isc_netaddr_t na;
	dns_peer_t *peer;
	const cfg_obj_t *obj;
	const char *str;
	isc_result_t result;
	unsigned int prefixlen;

	cfg_obj_asnetprefix(cfg_map_getname(cpeer), &na, &prefixlen);

	peer = NULL;
	result = dns_peer_newprefix(mctx, &na, prefixlen, &peer);
	if (result != ISC_R_SUCCESS)
		return (result);

	obj = NULL;
	(void)cfg_map_get(cpeer, "bogus", &obj);
	if (obj != NULL)
		CHECK(dns_peer_setbogus(peer, cfg_obj_asboolean(obj)));

	obj = NULL;
	(void)cfg_map_get(cpeer, "provide-ixfr", &obj);
	if (obj != NULL)
		CHECK(dns_peer_setprovideixfr(peer, cfg_obj_asboolean(obj)));

	obj = NULL;
	(void)cfg_map_get(cpeer, "request-expire", &obj);
	if (obj != NULL)
		CHECK(dns_peer_setrequestexpire(peer, cfg_obj_asboolean(obj)));

	obj = NULL;
	(void)cfg_map_get(cpeer, "request-ixfr", &obj);
	if (obj != NULL)
		CHECK(dns_peer_setrequestixfr(peer, cfg_obj_asboolean(obj)));

	obj = NULL;
	(void)cfg_map_get(cpeer, "request-nsid", &obj);
	if (obj != NULL)
		CHECK(dns_peer_setrequestnsid(peer, cfg_obj_asboolean(obj)));

	obj = NULL;
	(void)cfg_map_get(cpeer, "send-cookie", &obj);
	if (obj != NULL)
		CHECK(dns_peer_setsendcookie(peer, cfg_obj_asboolean(obj)));

	obj = NULL;
	(void)cfg_map_get(cpeer, "edns", &obj);
	if (obj != NULL)
		CHECK(dns_peer_setsupportedns(peer, cfg_obj_asboolean(obj)));

	obj = NULL;
	(void)cfg_map_get(cpeer, "edns-udp-size", &obj);
	if (obj != NULL) {
		isc_uint32_t udpsize = cfg_obj_asuint32(obj);
		if (udpsize < 512U)
			udpsize = 512U;
		if (udpsize > 4096U)
			udpsize = 4096U;
		CHECK(dns_peer_setudpsize(peer, (isc_uint16_t)udpsize));
	}

	obj = NULL;
	(void)cfg_map_get(cpeer, "edns-version", &obj);
	if (obj != NULL) {
		isc_uint32_t ednsversion = cfg_obj_asuint32(obj);
		if (ednsversion > 255U)
			ednsversion = 255U;
		CHECK(dns_peer_setednsversion(peer, (isc_uint8_t)ednsversion));
	}

	obj = NULL;
	(void)cfg_map_get(cpeer, "max-udp-size", &obj);
	if (obj != NULL) {
		isc_uint32_t udpsize = cfg_obj_asuint32(obj);
		if (udpsize < 512U)
			udpsize = 512U;
		if (udpsize > 4096U)
			udpsize = 4096U;
		CHECK(dns_peer_setmaxudp(peer, (isc_uint16_t)udpsize));
	}

	obj = NULL;
	(void)cfg_map_get(cpeer, "padding", &obj);
	if (obj != NULL) {
		isc_uint32_t padding = cfg_obj_asuint32(obj);
		if (padding > 512U) {
			cfg_obj_log(obj, named_g_lctx, ISC_LOG_WARNING,
				    "server padding value cannot "
				    "exceed 512: lowering");
			padding = 512U;
		}
		CHECK(dns_peer_setpadding(peer, (isc_uint16_t)padding));
	}

	obj = NULL;
	(void)cfg_map_get(cpeer, "tcp-only", &obj);
	if (obj != NULL)
		CHECK(dns_peer_setforcetcp(peer, cfg_obj_asboolean(obj)));

	obj = NULL;
	(void)cfg_map_get(cpeer, "tcp-keepalive", &obj);
	if (obj != NULL)
		CHECK(dns_peer_settcpkeepalive(peer, cfg_obj_asboolean(obj)));

	obj = NULL;
	(void)cfg_map_get(cpeer, "transfers", &obj);
	if (obj != NULL)
		CHECK(dns_peer_settransfers(peer, cfg_obj_asuint32(obj)));

	obj = NULL;
	(void)cfg_map_get(cpeer, "transfer-format", &obj);
	if (obj != NULL) {
		str = cfg_obj_asstring(obj);
		if (strcasecmp(str, "many-answers") == 0)
			CHECK(dns_peer_settransferformat(peer,
							 dns_many_answers));
		else if (strcasecmp(str, "one-answer") == 0)
			CHECK(dns_peer_settransferformat(peer,
							 dns_one_answer));
		else
			INSIST(0);
	}

	obj = NULL;
	(void)cfg_map_get(cpeer, "keys", &obj);
	if (obj != NULL) {
		result = dns_peer_setkeybycharp(peer, cfg_obj_asstring(obj));
		if (result != ISC_R_SUCCESS)
			goto cleanup;
	}

	obj = NULL;
	if (na.family == AF_INET)
		(void)cfg_map_get(cpeer, "transfer-source", &obj);
	else
		(void)cfg_map_get(cpeer, "transfer-source-v6", &obj);
	if (obj != NULL) {
		result = dns_peer_settransfersource(peer,
						    cfg_obj_assockaddr(obj));
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		result = dns_peer_settransferdscp(peer, cfg_obj_getdscp(obj));
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		named_add_reserved_dispatch(named_g_server,
					    cfg_obj_assockaddr(obj));
	}

	obj = NULL;
	if (na.family == AF_INET)
		(void)cfg_map_get(cpeer, "notify-source", &obj);
	else
		(void)cfg_map_get(cpeer, "notify-source-v6", &obj);
	if (obj != NULL) {
		result = dns_peer_setnotifysource(peer,
						  cfg_obj_assockaddr(obj));
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		result = dns_peer_setnotifydscp(peer, cfg_obj_getdscp(obj));
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		named_add_reserved_dispatch(named_g_server,
					    cfg_obj_assockaddr(obj));
	}

	obj = NULL;
	if (na.family == AF_INET)
		(void)cfg_map_get(cpeer, "query-source", &obj);
	else
		(void)cfg_map_get(cpeer, "query-source-v6", &obj);
	if (obj != NULL) {
		result = dns_peer_setquerysource(peer,
						 cfg_obj_assockaddr(obj));
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		result = dns_peer_setquerydscp(peer, cfg_obj_getdscp(obj));
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		named_add_reserved_dispatch(named_g_server,
					    cfg_obj_assockaddr(obj));
	}

	*peerp = peer;
	return (ISC_R_SUCCESS);

 cleanup:
	dns_peer_detach(&peer);
	return (result);
}

#ifdef HAVE_DLOPEN
static isc_result_t
configure_dyndb(const cfg_obj_t *dyndb, isc_mem_t *mctx,
		const dns_dyndbctx_t *dctx)
{
	isc_result_t result = ISC_R_SUCCESS;
	const cfg_obj_t *obj;
	const char *name, *library;

	/* Get the name of the dyndb instance and the library path . */
	name = cfg_obj_asstring(cfg_tuple_get(dyndb, "name"));
	library = cfg_obj_asstring(cfg_tuple_get(dyndb, "library"));

	obj = cfg_tuple_get(dyndb, "parameters");
	if (obj != NULL)
		result = dns_dyndb_load(library, name, cfg_obj_asstring(obj),
					cfg_obj_file(obj), cfg_obj_line(obj),
					mctx, dctx);

	if (result != ISC_R_SUCCESS)
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "dynamic database '%s' configuration failed: %s",
			      name, isc_result_totext(result));
	return (result);
}
#endif


static isc_result_t
disable_algorithms(const cfg_obj_t *disabled, dns_resolver_t *resolver) {
	isc_result_t result;
	const cfg_obj_t *algorithms;
	const cfg_listelt_t *element;
	const char *str;
	dns_fixedname_t fixed;
	dns_name_t *name;
	isc_buffer_t b;

	name = dns_fixedname_initname(&fixed);
	str = cfg_obj_asstring(cfg_tuple_get(disabled, "name"));
	isc_buffer_constinit(&b, str, strlen(str));
	isc_buffer_add(&b, strlen(str));
	CHECK(dns_name_fromtext(name, &b, dns_rootname, 0, NULL));

	algorithms = cfg_tuple_get(disabled, "algorithms");
	for (element = cfg_list_first(algorithms);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		isc_textregion_t r;
		dns_secalg_t alg;

		DE_CONST(cfg_obj_asstring(cfg_listelt_value(element)), r.base);
		r.length = strlen(r.base);

		result = dns_secalg_fromtext(&alg, &r);
		if (result != ISC_R_SUCCESS) {
			isc_uint8_t ui;
			result = isc_parse_uint8(&ui, r.base, 10);
			alg = ui;
		}
		if (result != ISC_R_SUCCESS) {
			cfg_obj_log(cfg_listelt_value(element),
				    named_g_lctx, ISC_LOG_ERROR,
				    "invalid algorithm");
			CHECK(result);
		}
		CHECK(dns_resolver_disable_algorithm(resolver, name, alg));
	}
 cleanup:
	return (result);
}

static isc_result_t
disable_ds_digests(const cfg_obj_t *disabled, dns_resolver_t *resolver) {
	isc_result_t result;
	const cfg_obj_t *digests;
	const cfg_listelt_t *element;
	const char *str;
	dns_fixedname_t fixed;
	dns_name_t *name;
	isc_buffer_t b;

	name = dns_fixedname_initname(&fixed);
	str = cfg_obj_asstring(cfg_tuple_get(disabled, "name"));
	isc_buffer_constinit(&b, str, strlen(str));
	isc_buffer_add(&b, strlen(str));
	CHECK(dns_name_fromtext(name, &b, dns_rootname, 0, NULL));

	digests = cfg_tuple_get(disabled, "digests");
	for (element = cfg_list_first(digests);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		isc_textregion_t r;
		dns_dsdigest_t digest;

		DE_CONST(cfg_obj_asstring(cfg_listelt_value(element)), r.base);
		r.length = strlen(r.base);

		/* disable_ds_digests handles numeric values. */
		result = dns_dsdigest_fromtext(&digest, &r);
		if (result != ISC_R_SUCCESS) {
			cfg_obj_log(cfg_listelt_value(element),
				    named_g_lctx, ISC_LOG_ERROR,
				    "invalid algorithm");
			CHECK(result);
		}
		CHECK(dns_resolver_disable_ds_digest(resolver, name, digest));
	}
 cleanup:
	return (result);
}

static isc_boolean_t
on_disable_list(const cfg_obj_t *disablelist, dns_name_t *zonename) {
	const cfg_listelt_t *element;
	dns_fixedname_t fixed;
	dns_name_t *name;
	isc_result_t result;
	const cfg_obj_t *value;
	const char *str;
	isc_buffer_t b;

	name = dns_fixedname_initname(&fixed);

	for (element = cfg_list_first(disablelist);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		value = cfg_listelt_value(element);
		str = cfg_obj_asstring(value);
		isc_buffer_constinit(&b, str, strlen(str));
		isc_buffer_add(&b, strlen(str));
		result = dns_name_fromtext(name, &b, dns_rootname,
					   0, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		if (dns_name_equal(name, zonename))
			return (ISC_TRUE);
	}
	return (ISC_FALSE);
}

static isc_result_t
check_dbtype(dns_zone_t *zone, unsigned int dbtypec, const char **dbargv,
	     isc_mem_t *mctx)
{
	char **argv = NULL;
	unsigned int i;
	isc_result_t result = ISC_R_SUCCESS;

	CHECK(dns_zone_getdbtype(zone, &argv, mctx));

	/*
	 * Check that all the arguments match.
	 */
	for (i = 0; i < dbtypec; i++)
		if (argv[i] == NULL || strcmp(argv[i], dbargv[i]) != 0)
			CHECK(ISC_R_FAILURE);

	/*
	 * Check that there are not extra arguments.
	 */
	if (i == dbtypec && argv[i] != NULL)
		result = ISC_R_FAILURE;

 cleanup:
	isc_mem_free(mctx, argv);
	return (result);
}

static isc_result_t
setquerystats(dns_zone_t *zone, isc_mem_t *mctx, dns_zonestat_level_t level) {
	isc_result_t result;
	isc_stats_t *zoneqrystats;

	dns_zone_setstatlevel(zone, level);

	zoneqrystats = NULL;
	if (level == dns_zonestat_full) {
		result = isc_stats_create(mctx, &zoneqrystats,
					  ns_statscounter_max);
		if (result != ISC_R_SUCCESS)
			return (result);
	}
	dns_zone_setrequeststats(zone, zoneqrystats);
	if (zoneqrystats != NULL)
		isc_stats_detach(&zoneqrystats);

	return (ISC_R_SUCCESS);
}

static named_cache_t *
cachelist_find(named_cachelist_t *cachelist, const char *cachename,
	       dns_rdataclass_t rdclass)
{
	named_cache_t *nsc;

	for (nsc = ISC_LIST_HEAD(*cachelist);
	     nsc != NULL;
	     nsc = ISC_LIST_NEXT(nsc, link)) {
		if (nsc->rdclass == rdclass &&
		    strcmp(dns_cache_getname(nsc->cache), cachename) == 0)
			return (nsc);
	}

	return (NULL);
}

static isc_boolean_t
cache_reusable(dns_view_t *originview, dns_view_t *view,
	       isc_boolean_t new_zero_no_soattl)
{
	if (originview->rdclass != view->rdclass ||
	    originview->checknames != view->checknames ||
	    dns_resolver_getzeronosoattl(originview->resolver) !=
	    new_zero_no_soattl ||
	    originview->acceptexpired != view->acceptexpired ||
	    originview->enablevalidation != view->enablevalidation ||
	    originview->maxcachettl != view->maxcachettl ||
	    originview->maxncachettl != view->maxncachettl) {
		return (ISC_FALSE);
	}

	return (ISC_TRUE);
}

static isc_boolean_t
cache_sharable(dns_view_t *originview, dns_view_t *view,
	       isc_boolean_t new_zero_no_soattl,
	       unsigned int new_cleaning_interval,
	       isc_uint64_t new_max_cache_size,
	       isc_uint32_t new_stale_ttl)
{
	/*
	 * If the cache cannot even reused for the same view, it cannot be
	 * shared with other views.
	 */
	if (!cache_reusable(originview, view, new_zero_no_soattl))
		return (ISC_FALSE);

	/*
	 * Check other cache related parameters that must be consistent among
	 * the sharing views.
	 */
	if (dns_cache_getcleaninginterval(originview->cache) !=
	    new_cleaning_interval ||
	    dns_cache_getservestalettl(originview->cache) != new_stale_ttl ||
	    dns_cache_getcachesize(originview->cache) != new_max_cache_size) {
		return (ISC_FALSE);
	}

	return (ISC_TRUE);
}

/*
 * Callback from DLZ configure when the driver sets up a writeable zone
 */
static isc_result_t
dlzconfigure_callback(dns_view_t *view, dns_dlzdb_t *dlzdb, dns_zone_t *zone) {
	dns_name_t *origin = dns_zone_getorigin(zone);
	dns_rdataclass_t zclass = view->rdclass;
	isc_result_t result;

	result = dns_zonemgr_managezone(named_g_server->zonemgr, zone);
	if (result != ISC_R_SUCCESS)
		return (result);
	dns_zone_setstats(zone, named_g_server->zonestats);

	return (named_zone_configure_writeable_dlz(dlzdb, zone,
						   zclass, origin));
}

static isc_result_t
dns64_reverse(dns_view_t *view, isc_mem_t *mctx, isc_netaddr_t *na,
	      unsigned int prefixlen, const char *server,
	      const char *contact)
{
	char reverse[48+sizeof("ip6.arpa.")] = { 0 };
	char buf[sizeof("x.x.")];
	const char *dns64_dbtype[4] = { "_dns64", "dns64", ".", "." };
	const char *sep = ": view ";
	const char *viewname = view->name;
	const unsigned char *s6;
	dns_fixedname_t fixed;
	dns_name_t *name;
	dns_zone_t *zone = NULL;
	int dns64_dbtypec = 4;
	isc_buffer_t b;
	isc_result_t result;

	REQUIRE(prefixlen == 32 || prefixlen == 40 || prefixlen == 48 ||
		prefixlen == 56 || prefixlen == 64 || prefixlen == 96);

	if (!strcmp(viewname, "_default")) {
		sep = "";
		viewname = "";
	}

	/*
	 * Construct the reverse name of the zone.
	 */
	s6 = na->type.in6.s6_addr;
	while (prefixlen > 0) {
		prefixlen -= 8;
		snprintf(buf, sizeof(buf), "%x.%x.", s6[prefixlen/8] & 0xf,
			 (s6[prefixlen/8] >> 4) & 0xf);
		strlcat(reverse, buf, sizeof(reverse));
	}
	strlcat(reverse, "ip6.arpa.", sizeof(reverse));

	/*
	 * Create the actual zone.
	 */
	if (server != NULL)
		dns64_dbtype[2] = server;
	if (contact != NULL)
		dns64_dbtype[3] = contact;
	name = dns_fixedname_initname(&fixed);
	isc_buffer_constinit(&b, reverse, strlen(reverse));
	isc_buffer_add(&b, strlen(reverse));
	CHECK(dns_name_fromtext(name, &b, dns_rootname, 0, NULL));
	CHECK(dns_zone_create(&zone, mctx));
	CHECK(dns_zone_setorigin(zone, name));
	dns_zone_setview(zone, view);
	CHECK(dns_zonemgr_managezone(named_g_server->zonemgr, zone));
	dns_zone_setclass(zone, view->rdclass);
	dns_zone_settype(zone, dns_zone_master);
	dns_zone_setstats(zone, named_g_server->zonestats);
	CHECK(dns_zone_setdbtype(zone, dns64_dbtypec, dns64_dbtype));
	if (view->queryacl != NULL)
		dns_zone_setqueryacl(zone, view->queryacl);
	if (view->queryonacl != NULL)
		dns_zone_setqueryonacl(zone, view->queryonacl);
	dns_zone_setdialup(zone, dns_dialuptype_no);
	dns_zone_setnotifytype(zone, dns_notifytype_no);
	dns_zone_setoption(zone, DNS_ZONEOPT_NOCHECKNS, ISC_TRUE);
	CHECK(setquerystats(zone, mctx, dns_zonestat_none));	/* XXXMPA */
	CHECK(dns_view_addzone(view, zone));
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "dns64 reverse zone%s%s: %s", sep,
		      viewname, reverse);

cleanup:
	if (zone != NULL)
		dns_zone_detach(&zone);
	return (result);
}

#ifdef USE_DNSRPS
typedef struct conf_dnsrps_ctx conf_dnsrps_ctx_t;
struct conf_dnsrps_ctx {
	isc_result_t	result;
	char		*cstr;
	size_t		cstr_size;
	isc_mem_t	*mctx;
};

/*
 * Add to the DNSRPS configuration string.
 */
static isc_boolean_t
conf_dnsrps_sadd(conf_dnsrps_ctx_t *ctx, const char *p, ...) {
	size_t new_len, cur_len, new_cstr_size;
	char *new_cstr;
	va_list args;

	if (ctx->cstr == NULL) {
		ctx->cstr = isc_mem_get(ctx->mctx, 256);
		if (ctx->cstr == NULL) {
			ctx->result = ISC_R_NOMEMORY;
			return (ISC_FALSE);
		}
		ctx->cstr[0] = '\0';
		ctx->cstr_size = 256;
	}

	cur_len = strlen(ctx->cstr);
	va_start(args, p);
	new_len = vsnprintf(ctx->cstr + cur_len, ctx->cstr_size - cur_len,
			    p, args) + 1;
	va_end(args);

	if (cur_len + new_len <= ctx->cstr_size)
		return (ISC_TRUE);

	new_cstr_size = ((cur_len + new_len)/256 + 1) * 256;
	new_cstr = isc_mem_get(ctx->mctx, new_cstr_size);
	if (new_cstr == NULL) {
		ctx->result = ISC_R_NOMEMORY;
		return (ISC_FALSE);
	}

	memmove(new_cstr, ctx->cstr, cur_len);
	isc_mem_put(ctx->mctx, ctx->cstr, ctx->cstr_size);
	ctx->cstr_size = new_cstr_size;
	ctx->cstr = new_cstr;

	/* cannot use args twice after a single va_start()on some systems */
	va_start(args, p);
	vsnprintf(ctx->cstr + cur_len, ctx->cstr_size - cur_len, p, args);
	va_end(args);
	return (ISC_TRUE);
}

/*
 * Get an DNSRPS configuration value using the global and view options
 * for the default.  Return ISC_FALSE upon failure.
 */
static isc_boolean_t
conf_dnsrps_get(const cfg_obj_t **sub_obj,
		const cfg_obj_t **maps ,const cfg_obj_t *obj,
		const char *name, conf_dnsrps_ctx_t *ctx)
{
	if (ctx != NULL && ctx->result != ISC_R_SUCCESS) {
		*sub_obj = NULL;
		return (ISC_FALSE);
	}

	*sub_obj = cfg_tuple_get(obj, name);
	if (cfg_obj_isvoid(*sub_obj)) {
		*sub_obj = NULL;
		if (maps != NULL &&
		    ISC_R_SUCCESS != named_config_get(maps, name, sub_obj))
			*sub_obj = NULL;
	}
	return (ISC_TRUE);
}

/*
 * Handle a DNSRPS boolean configuration value with the global and view
 * options providing the default.
 */
static void
conf_dnsrps_yes_no(const cfg_obj_t *obj, const char* name,
		   conf_dnsrps_ctx_t *ctx)
{
	const cfg_obj_t *sub_obj;

	if (!conf_dnsrps_get(&sub_obj, NULL, obj, name, ctx))
		return;
	if (sub_obj == NULL)
		return;
	if (ctx == NULL) {
		cfg_obj_log(obj, named_g_lctx, ISC_LOG_ERROR,
			    "\"%s\" without \"dnsrps-enable yes\"",
			    name);
		return;
	}

	conf_dnsrps_sadd(ctx, " %s %s", name,
			 cfg_obj_asboolean(sub_obj) ? "yes" : "no");
}

static void
conf_dnsrps_num(const cfg_obj_t *obj, const char *name,
		conf_dnsrps_ctx_t *ctx)
{
	const cfg_obj_t *sub_obj;

	if (!conf_dnsrps_get(&sub_obj, NULL, obj, name, ctx))
		return;
	if (sub_obj == NULL)
		return;
	if (ctx == NULL) {
		cfg_obj_log(obj, named_g_lctx, ISC_LOG_ERROR,
			    "\"%s\" without \"dnsrps-enable yes\"",
			    name);
		return;
	}

	conf_dnsrps_sadd(ctx, " %s %d", name, cfg_obj_asuint32(sub_obj));
}

/*
 * Convert the parsed RPZ configuration statement to a string for
 * dns_rpz_new_zones().
 */
static isc_result_t
conf_dnsrps(dns_view_t *view, const cfg_obj_t **maps,
	    isc_boolean_t nsip_enabled, isc_boolean_t nsdname_enabled,
	    dns_rpz_zbits_t *nsip_on, dns_rpz_zbits_t *nsdname_on,
	    char **rps_cstr, size_t *rps_cstr_size,
	    const cfg_obj_t *rpz_obj, const cfg_listelt_t *zone_element)
{
	conf_dnsrps_ctx_t ctx;
	const cfg_obj_t *zone_obj, *obj;
	dns_rpz_num_t rpz_num;
	isc_boolean_t on;
	const char *s;

	memset(&ctx, 0, sizeof(ctx));
	ctx.result = ISC_R_SUCCESS;
	ctx.mctx = view->mctx;

	for (rpz_num = 0;
	     zone_element != NULL && ctx.result == ISC_R_SUCCESS;
	     ++rpz_num) {
		zone_obj = cfg_listelt_value(zone_element);

		s = cfg_obj_asstring(cfg_tuple_get(zone_obj, "zone name"));
		conf_dnsrps_sadd(&ctx, "zone \"%s\"", s);

		obj = cfg_tuple_get(zone_obj, "policy");
		if (!cfg_obj_isvoid(obj)) {
			s = cfg_obj_asstring(cfg_tuple_get(obj, "policy name"));
			conf_dnsrps_sadd(&ctx, " policy %s", s);
			if (strcasecmp(s, "cname") == 0) {
				s = cfg_obj_asstring(cfg_tuple_get(obj,
								   "cname"));
				conf_dnsrps_sadd(&ctx, " %s", s);
			}
		}

		conf_dnsrps_yes_no(zone_obj, "recursive-only", &ctx);
		conf_dnsrps_yes_no(zone_obj, "log", &ctx);
		conf_dnsrps_num(zone_obj, "max-policy-ttl", &ctx);
		obj = cfg_tuple_get(rpz_obj, "nsip-enable");
		if (!cfg_obj_isvoid(obj)) {
			if (cfg_obj_asboolean(obj))
				*nsip_on |= DNS_RPZ_ZBIT(rpz_num);
			else
				*nsip_on &= ~DNS_RPZ_ZBIT(rpz_num);
		}
		on = ((*nsip_on & DNS_RPZ_ZBIT(rpz_num)) != 0);
		if (nsip_enabled != on)
			conf_dnsrps_sadd(&ctx, on ? " nsip-enable yes " :
					  " nsip-enable no ");
		obj = cfg_tuple_get(rpz_obj, "nsdname-enable");
		if (!cfg_obj_isvoid(obj)) {
			if (cfg_obj_asboolean(obj))
				*nsdname_on |= DNS_RPZ_ZBIT(rpz_num);
			else
				*nsdname_on &= ~DNS_RPZ_ZBIT(rpz_num);
		}
		on = ((*nsdname_on & DNS_RPZ_ZBIT(rpz_num)) != 0);
		if (nsdname_enabled != on)
			conf_dnsrps_sadd(&ctx, on
						 ? " nsdname-enable yes "
						 : " nsdname-enable no ");
		conf_dnsrps_sadd(&ctx, ";\n");
		zone_element = cfg_list_next(zone_element);
	}

	conf_dnsrps_yes_no(rpz_obj, "recursive-only",  &ctx);
	conf_dnsrps_num(rpz_obj, "max-policy-ttl",  &ctx);
	conf_dnsrps_num(rpz_obj, "min-ns-dots",  &ctx);
	conf_dnsrps_yes_no(rpz_obj, "qname-wait-recurse",  &ctx);
	conf_dnsrps_yes_no(rpz_obj, "break-dnssec",  &ctx);
	if (!nsip_enabled)
		conf_dnsrps_sadd(&ctx, " nsip-enable no ");
	if (!nsdname_enabled)
		conf_dnsrps_sadd(&ctx,  " nsdname-enable no ");

	/*
	 * Get the general dnsrpzd parameters from the response-policy
	 * statement in the view and the general options.
	 */
	if (conf_dnsrps_get(&obj, maps, rpz_obj, "dnsrps-options", &ctx) &&
	    obj != NULL)
		conf_dnsrps_sadd(&ctx, " %s\n", cfg_obj_asstring(obj));

	if (ctx.result == ISC_R_SUCCESS) {
		*rps_cstr = ctx.cstr;
		*rps_cstr_size = ctx.cstr_size;
	} else {
		if (ctx.cstr != NULL)
			isc_mem_put(ctx.mctx, ctx.cstr, ctx.cstr_size);
		*rps_cstr = NULL;
		*rps_cstr_size = 0;
	}
	return (ctx.result);
}
#endif

static isc_result_t
configure_rpz_name(dns_view_t *view, const cfg_obj_t *obj, dns_name_t *name,
		   const char *str, const char *msg)
{
	isc_result_t result;

	result = dns_name_fromstring(name, str, DNS_NAME_DOWNCASE, view->mctx);
	if (result != ISC_R_SUCCESS)
		cfg_obj_log(obj, named_g_lctx, DNS_RPZ_ERROR_LEVEL,
			    "invalid %s '%s'", msg, str);
	return (result);
}

static isc_result_t
configure_rpz_name2(dns_view_t *view, const cfg_obj_t *obj, dns_name_t *name,
		    const char *str, const dns_name_t *origin)
{
	isc_result_t result;

	result = dns_name_fromstring2(name, str, origin, DNS_NAME_DOWNCASE,
				      view->mctx);
	if (result != ISC_R_SUCCESS)
		cfg_obj_log(obj, named_g_lctx, DNS_RPZ_ERROR_LEVEL,
			    "invalid zone '%s'", str);
	return (result);
}

static isc_result_t
configure_rpz_zone(dns_view_t *view, const cfg_listelt_t *element,
		   isc_boolean_t recursive_only_default,
		   dns_ttl_t ttl_default,
		   isc_uint32_t minupdateinterval_default,
		   const dns_rpz_zone_t *old,
		   isc_boolean_t *old_rpz_okp)
{
	const cfg_obj_t *rpz_obj, *obj;
	const char *str;
	dns_rpz_zone_t *zone = NULL;
	isc_result_t result;
	dns_rpz_num_t rpz_num;

	REQUIRE(old != NULL || !*old_rpz_okp);

	rpz_obj = cfg_listelt_value(element);

	if (view->rpzs->p.num_zones >= DNS_RPZ_MAX_ZONES) {
		cfg_obj_log(rpz_obj, named_g_lctx, DNS_RPZ_ERROR_LEVEL,
			    "limit of %d response policy zones exceeded",
			    DNS_RPZ_MAX_ZONES);
		return (ISC_R_FAILURE);
	}

	result = dns_rpz_new_zone(view->rpzs, &zone);
	if (result != ISC_R_SUCCESS) {
		cfg_obj_log(rpz_obj, named_g_lctx, DNS_RPZ_ERROR_LEVEL,
			    "Error creating new RPZ zone : %s",
			    isc_result_totext(result));
		return (result);
	}

	obj = cfg_tuple_get(rpz_obj, "recursive-only");
	if (cfg_obj_isvoid(obj) ?
	    recursive_only_default : cfg_obj_asboolean(obj))
	{
		view->rpzs->p.no_rd_ok &= ~DNS_RPZ_ZBIT(zone->num);
	} else {
		view->rpzs->p.no_rd_ok |= DNS_RPZ_ZBIT(zone->num);
	}

	obj = cfg_tuple_get(rpz_obj, "log");
	if (!cfg_obj_isvoid(obj) && !cfg_obj_asboolean(obj)) {
		view->rpzs->p.no_log |= DNS_RPZ_ZBIT(zone->num);
	} else {
		view->rpzs->p.no_log &= ~DNS_RPZ_ZBIT(zone->num);
	}

	obj = cfg_tuple_get(rpz_obj, "max-policy-ttl");
	if (cfg_obj_isuint32(obj)) {
		zone->max_policy_ttl = cfg_obj_asuint32(obj);
	} else {
		zone->max_policy_ttl = ttl_default;
	}

	obj = cfg_tuple_get(rpz_obj, "min-update-interval");
	if (cfg_obj_isuint32(obj)) {
		zone->min_update_interval = cfg_obj_asuint32(obj);
	} else {
		zone->min_update_interval = minupdateinterval_default;
	}

	if (*old_rpz_okp && zone->max_policy_ttl != old->max_policy_ttl)
		*old_rpz_okp = ISC_FALSE;

	str = cfg_obj_asstring(cfg_tuple_get(rpz_obj, "zone name"));
	result = configure_rpz_name(view, rpz_obj, &zone->origin, str, "zone");
	if (result != ISC_R_SUCCESS)
		return (result);
	if (dns_name_equal(&zone->origin, dns_rootname)) {
		cfg_obj_log(rpz_obj, named_g_lctx, DNS_RPZ_ERROR_LEVEL,
			    "invalid zone name '%s'", str);
		return (DNS_R_EMPTYLABEL);
	}
	if (!view->rpzs->p.dnsrps_enabled) {
		for (rpz_num = 0;
		     rpz_num < view->rpzs->p.num_zones - 1;
		     ++rpz_num)
		{
			if (dns_name_equal(&view->rpzs->zones[rpz_num]->origin,
					   &zone->origin)) {
				cfg_obj_log(rpz_obj, named_g_lctx,
					    DNS_RPZ_ERROR_LEVEL,
					    "duplicate '%s'", str);
				result = DNS_R_DUPLICATE;
				return (result);
			}
		}
	}
	if (*old_rpz_okp && !dns_name_equal(&old->origin, &zone->origin))
		*old_rpz_okp = ISC_FALSE;

	result = configure_rpz_name2(view, rpz_obj, &zone->client_ip,
				     DNS_RPZ_CLIENT_IP_ZONE, &zone->origin);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = configure_rpz_name2(view, rpz_obj, &zone->ip,
				     DNS_RPZ_IP_ZONE, &zone->origin);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = configure_rpz_name2(view, rpz_obj, &zone->nsdname,
				     DNS_RPZ_NSDNAME_ZONE, &zone->origin);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = configure_rpz_name2(view, rpz_obj, &zone->nsip,
				     DNS_RPZ_NSIP_ZONE, &zone->origin);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = configure_rpz_name(view, rpz_obj, &zone->passthru,
				    DNS_RPZ_PASSTHRU_NAME, "name");
	if (result != ISC_R_SUCCESS)
		return (result);

	result = configure_rpz_name(view, rpz_obj, &zone->drop,
				    DNS_RPZ_DROP_NAME, "name");
	if (result != ISC_R_SUCCESS)
		return (result);

	result = configure_rpz_name(view, rpz_obj, &zone->tcp_only,
				    DNS_RPZ_TCP_ONLY_NAME, "name");
	if (result != ISC_R_SUCCESS)
		return (result);

	obj = cfg_tuple_get(rpz_obj, "policy");
	if (cfg_obj_isvoid(obj)) {
		zone->policy = DNS_RPZ_POLICY_GIVEN;
	} else {
		str = cfg_obj_asstring(cfg_tuple_get(obj, "policy name"));
		zone->policy = dns_rpz_str2policy(str);
		INSIST(zone->policy != DNS_RPZ_POLICY_ERROR);
		if (zone->policy == DNS_RPZ_POLICY_CNAME) {
			str = cfg_obj_asstring(cfg_tuple_get(obj, "cname"));
			result = configure_rpz_name(view, rpz_obj, &zone->cname,
						    str, "cname");
			if (result != ISC_R_SUCCESS)
				return (result);
		}
	}
	if (*old_rpz_okp && (zone->policy != old->policy ||
			     !dns_name_equal(&old->cname, &zone->cname)))
		*old_rpz_okp = ISC_FALSE;

	return (ISC_R_SUCCESS);
}

static isc_result_t
configure_rpz(dns_view_t *view, const cfg_obj_t **maps,
	      const cfg_obj_t *rpz_obj, isc_boolean_t *old_rpz_okp)
{
	isc_boolean_t dnsrps_enabled;
	const cfg_listelt_t *zone_element;
	char *rps_cstr;
	size_t rps_cstr_size;
	const cfg_obj_t *sub_obj;
	isc_boolean_t recursive_only_default;
	isc_boolean_t nsip_enabled, nsdname_enabled;
	dns_rpz_zbits_t nsip_on, nsdname_on;
	dns_ttl_t ttl_default;
	isc_uint32_t minupdateinterval_default;
	dns_rpz_zones_t *zones;
	const dns_rpz_zones_t *old;
	dns_view_t *pview;
	const dns_rpz_zone_t *old_zone;
	isc_result_t result;
	int i;

	*old_rpz_okp = ISC_FALSE;

	zone_element = cfg_list_first(cfg_tuple_get(rpz_obj, "zone list"));
	if (zone_element == NULL)
		return (ISC_R_SUCCESS);

#ifdef ENABLE_RPZ_NSIP
	nsip_enabled = ISC_TRUE;
	nsdname_enabled = ISC_TRUE;
#else
	nsip_enabled = ISC_FALSE;
	nsdname_enabled = ISC_FALSE;
#endif
	sub_obj = cfg_tuple_get(rpz_obj, "nsip-enable");
	if (!cfg_obj_isvoid(sub_obj))
		nsip_enabled = cfg_obj_asboolean(sub_obj);
	nsip_on = nsip_enabled ? DNS_RPZ_ALL_ZBITS : 0;

	sub_obj = cfg_tuple_get(rpz_obj, "nsdname-enable");
	if (!cfg_obj_isvoid(sub_obj))
		nsdname_enabled = cfg_obj_asboolean(sub_obj);
	nsdname_on = nsdname_enabled ? DNS_RPZ_ALL_ZBITS : 0;

	/*
	 * "dnsrps-enable yes|no" can be either a global or response-policy
	 * clause.
	 */
	dnsrps_enabled = ISC_FALSE;
	rps_cstr = NULL;
	rps_cstr_size = 0;
	sub_obj = NULL;
	(void)named_config_get(maps, "dnsrps-enable", &sub_obj);
	if (sub_obj != NULL) {
		dnsrps_enabled = cfg_obj_asboolean(sub_obj);
	}
	sub_obj = cfg_tuple_get(rpz_obj, "dnsrps-enable");
	if (!cfg_obj_isvoid(sub_obj)) {
		dnsrps_enabled = cfg_obj_asboolean(sub_obj);
	}
#ifndef USE_DNSRPS
	if (dnsrps_enabled) {
		cfg_obj_log(rpz_obj, named_g_lctx, DNS_RPZ_ERROR_LEVEL,
			    "\"dnsrps-enable yes\" but"
			    " without `./configure --enable-dnsrps`");
		return (ISC_R_FAILURE);
	}
#else
	if (dnsrps_enabled) {
		if (librpz == NULL) {
			cfg_obj_log(rpz_obj, named_g_lctx, DNS_RPZ_ERROR_LEVEL,
				    "\"dnsrps-enable yes\" but %s",
				    librpz_lib_open_emsg.c);
			return (ISC_R_FAILURE);
		}

		/*
		 * Generate the DNS Response Policy Service
		 * configuration string.
		 */
		result = conf_dnsrps(view, maps,
				      nsip_enabled, nsdname_enabled,
				      &nsip_on, &nsdname_on,
				      &rps_cstr, &rps_cstr_size,
				      rpz_obj, zone_element);
		if (result != ISC_R_SUCCESS)
			return (result);
	}
#endif

	result = dns_rpz_new_zones(&view->rpzs, rps_cstr,
				   rps_cstr_size, view->mctx,
				   named_g_taskmgr, named_g_timermgr);
	if (result != ISC_R_SUCCESS)
		return (result);

	zones = view->rpzs;

	zones->p.nsip_on = nsip_on;
	zones->p.nsdname_on = nsdname_on;

	sub_obj = cfg_tuple_get(rpz_obj, "recursive-only");
	if (!cfg_obj_isvoid(sub_obj) &&
	    !cfg_obj_asboolean(sub_obj))
		recursive_only_default = ISC_FALSE;
	else
		recursive_only_default = ISC_TRUE;

	sub_obj = cfg_tuple_get(rpz_obj, "break-dnssec");
	if (!cfg_obj_isvoid(sub_obj) &&
	    cfg_obj_asboolean(sub_obj))
		zones->p.break_dnssec = ISC_TRUE;
	else
		zones->p.break_dnssec = ISC_FALSE;

	sub_obj = cfg_tuple_get(rpz_obj, "max-policy-ttl");
	if (cfg_obj_isuint32(sub_obj))
		ttl_default = cfg_obj_asuint32(sub_obj);
	else
		ttl_default = DNS_RPZ_MAX_TTL_DEFAULT;

	sub_obj = cfg_tuple_get(rpz_obj, "min-update-interval");
	if (cfg_obj_isuint32(sub_obj))
		minupdateinterval_default = cfg_obj_asuint32(sub_obj);
	else
		minupdateinterval_default = DNS_RPZ_MINUPDATEINTERVAL_DEFAULT;

	sub_obj = cfg_tuple_get(rpz_obj, "min-ns-dots");
	if (cfg_obj_isuint32(sub_obj))
		zones->p.min_ns_labels = cfg_obj_asuint32(sub_obj) + 1;
	else
		zones->p.min_ns_labels = 2;

	sub_obj = cfg_tuple_get(rpz_obj, "qname-wait-recurse");
	if (cfg_obj_isvoid(sub_obj) || cfg_obj_asboolean(sub_obj))
		zones->p.qname_wait_recurse = ISC_TRUE;
	else
		zones->p.qname_wait_recurse = ISC_FALSE;

	sub_obj = cfg_tuple_get(rpz_obj, "nsip-wait-recurse");
	if (cfg_obj_isvoid(sub_obj) || cfg_obj_asboolean(sub_obj))
		zones->p.nsip_wait_recurse = ISC_TRUE;
	else
		zones->p.nsip_wait_recurse = ISC_FALSE;

	pview = NULL;
	result = dns_viewlist_find(&named_g_server->viewlist,
				   view->name, view->rdclass, &pview);
	if (result == ISC_R_SUCCESS) {
		old = pview->rpzs;
	} else {
		old = NULL;
	}
	if (old == NULL)
		*old_rpz_okp = ISC_FALSE;
	else
		*old_rpz_okp = ISC_TRUE;

	for (i = 0;
	     zone_element != NULL;
	     ++i, zone_element = cfg_list_next(zone_element)) {
		INSIST(old != NULL || !*old_rpz_okp);
		if (*old_rpz_okp && i < old->p.num_zones) {
			old_zone = old->zones[i];
		} else {
			*old_rpz_okp = ISC_FALSE;
			old_zone = NULL;
		}
		result = configure_rpz_zone(view, zone_element,
					    recursive_only_default,
					    ttl_default,
					    minupdateinterval_default,
					    old_zone, old_rpz_okp);
		if (result != ISC_R_SUCCESS) {
			if (pview != NULL)
				dns_view_detach(&pview);
			return (result);
		}
	}

	/*
	 * If this is a reloading and the parameters and list of policy
	 * zones are unchanged, then use the same policy data.
	 * Data for individual zones that must be reloaded will be merged.
	 */
	if (*old_rpz_okp &&
	    old != NULL && memcmp(&old->p, &zones->p, sizeof(zones->p)) != 0)
	{
		*old_rpz_okp = ISC_FALSE;
	}

	if (*old_rpz_okp &&
	    (old == NULL ||
	     old->rps_cstr == NULL) != (zones->rps_cstr == NULL))
	{
		*old_rpz_okp = ISC_FALSE;
	}

	if (*old_rpz_okp &&
	    (zones->rps_cstr != NULL &&
	     strcmp(old->rps_cstr, zones->rps_cstr) != 0))
	{
		*old_rpz_okp = ISC_FALSE;
	}

	if (*old_rpz_okp) {
		dns_rpz_detach_rpzs(&view->rpzs);
		dns_rpz_attach_rpzs(pview->rpzs, &view->rpzs);
	} else if (old != NULL && pview != NULL) {
		++pview->rpzs->rpz_ver;
		view->rpzs->rpz_ver = pview->rpzs->rpz_ver;
		cfg_obj_log(rpz_obj, named_g_lctx, DNS_RPZ_DEBUG_LEVEL1,
			    "updated RPZ policy: version %d",
			    view->rpzs->rpz_ver);
	}

	if (pview != NULL)
		dns_view_detach(&pview);

	return (ISC_R_SUCCESS);
}

static void
catz_addmodzone_taskaction(isc_task_t *task, isc_event_t *event0) {
	catz_chgzone_event_t *ev = (catz_chgzone_event_t *) event0;
	isc_result_t result;
	isc_buffer_t namebuf;
	isc_buffer_t *confbuf;
	char nameb[DNS_NAME_FORMATSIZE];
	const cfg_obj_t *zlist = NULL;
	cfg_obj_t *zoneconf = NULL;
	cfg_obj_t *zoneobj = NULL;
	ns_cfgctx_t *cfg;
	dns_zone_t *zone = NULL;

	cfg = (ns_cfgctx_t *) ev->view->new_zone_config;
	if (cfg == NULL) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "catz: allow-new-zones statement missing from "
			      "config; cannot add zone from the catalog");
		goto cleanup;
	}

	isc_buffer_init(&namebuf, nameb, DNS_NAME_FORMATSIZE);
	dns_name_totext(dns_catz_entry_getname(ev->entry), ISC_TRUE, &namebuf);
	isc_buffer_putuint8(&namebuf, 0);

	/* Zone shouldn't already exist */
	result = dns_zt_find(ev->view->zonetable,
			     dns_catz_entry_getname(ev->entry), 0, NULL, &zone);

	if (ev->mod == ISC_TRUE) {
		if (result != ISC_R_SUCCESS) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
				      "catz: error \"%s\" while trying to "
				      "modify zone \"%s\"",
				      isc_result_totext(result),
				      nameb);
			goto cleanup;
		} else {
			if (!dns_zone_getadded(zone)) {
				isc_log_write(named_g_lctx,
					      NAMED_LOGCATEGORY_GENERAL,
					      NAMED_LOGMODULE_SERVER,
					      ISC_LOG_WARNING,
					      "catz: "
					      "catz_addmodzone_taskaction: "
					      "zone '%s' is not a dynamically "
					      "added zone",
					      nameb);
				goto cleanup;
			}
			if (dns_zone_get_parentcatz(zone) != ev->origin) {
				isc_log_write(named_g_lctx,
					      NAMED_LOGCATEGORY_GENERAL,
					      NAMED_LOGMODULE_SERVER,
					      ISC_LOG_WARNING,
					      "catz: catz_delzone_taskaction: "
					      "zone '%s' exists in multiple "
					      "catalog zones",
					      nameb);
				goto cleanup;
			}
			dns_zone_detach(&zone);
		}

	} else {
		if (result != ISC_R_NOTFOUND && result != DNS_R_PARTIALMATCH) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
				      "catz: error \"%s\" while trying to "
				      "add zone \"%s\"",
				      isc_result_totext(result),
				      nameb);
			goto cleanup;
		} else { /* this can happen in case of DNS_R_PARTIALMATCH */
			if (zone != NULL)
				dns_zone_detach(&zone);
		}
	}
	RUNTIME_CHECK(zone == NULL);
	/* Create a config for new zone */
	confbuf = NULL;
	result = dns_catz_generate_zonecfg(ev->origin, ev->entry, &confbuf);
	if (result == ISC_R_SUCCESS) {
		cfg_parser_reset(cfg->add_parser);
		result = cfg_parse_buffer3(cfg->add_parser, confbuf, "catz", 0,
					   &cfg_type_addzoneconf, &zoneconf);
		isc_buffer_free(&confbuf);
	}
	/*
	 * Fail if either dns_catz_generate_zonecfg() or cfg_parse_buffer3()
	 * failed.
	 */
	if (result != ISC_R_SUCCESS) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "catz: error \"%s\" while trying to generate "
			      "config for zone \"%s\"",
			      isc_result_totext(result), nameb);
		goto cleanup;
	}
	CHECK(cfg_map_get(zoneconf, "zone", &zlist));
	if (!cfg_obj_islist(zlist))
		CHECK(ISC_R_FAILURE);

	/* For now we only support adding one zone at a time */
	zoneobj = cfg_listelt_value(cfg_list_first(zlist));

	/* Mark view unfrozen so that zone can be added */

	result = isc_task_beginexclusive(task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	dns_view_thaw(ev->view);
	result = configure_zone(cfg->config, zoneobj, cfg->vconfig,
				ev->cbd->server->mctx, ev->view,
				&ev->cbd->server->viewlist, cfg->actx,
				ISC_TRUE, ISC_FALSE, ev->mod);
	dns_view_freeze(ev->view);
	isc_task_endexclusive(task);

	if (result != ISC_R_SUCCESS) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
			      "catz: failed to configure zone \"%s\" - %d",
			      nameb, result);
		goto cleanup;
	}

	/* Is it there yet? */
	CHECK(dns_zt_find(ev->view->zonetable,
			dns_catz_entry_getname(ev->entry), 0, NULL, &zone));

	/*
	 * Load the zone from the master file.	If this fails, we'll
	 * need to undo the configuration we've done already.
	 */
	result = dns_zone_loadnew(zone);
	if (result != ISC_R_SUCCESS) {
		dns_db_t *dbp = NULL;
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "catz: dns_zone_loadnew() failed "
			      "with %s; reverting.",
			      isc_result_totext(result));

		/* If the zone loaded partially, unload it */
		if (dns_zone_getdb(zone, &dbp) == ISC_R_SUCCESS) {
			dns_db_detach(&dbp);
			dns_zone_unload(zone);
		}

		/* Remove the zone from the zone table */
		dns_zt_unmount(ev->view->zonetable, zone);
		goto cleanup;
	}

	/* Flag the zone as having been added at runtime */
	dns_zone_setadded(zone, ISC_TRUE);
	dns_zone_set_parentcatz(zone, ev->origin);

 cleanup:
	if (zone != NULL)
		dns_zone_detach(&zone);
	if (zoneconf != NULL)
		cfg_obj_destroy(cfg->add_parser, &zoneconf);
	dns_catz_entry_detach(ev->origin, &ev->entry);
	dns_catz_zone_detach(&ev->origin);
	dns_view_detach(&ev->view);
	isc_event_free(ISC_EVENT_PTR(&ev));
}

static void
catz_delzone_taskaction(isc_task_t *task, isc_event_t *event0) {
	catz_chgzone_event_t *ev = (catz_chgzone_event_t *) event0;
	isc_result_t result;
	dns_zone_t *zone = NULL;
	dns_db_t *dbp = NULL;
	char cname[DNS_NAME_FORMATSIZE];
	const char * file;

	result = isc_task_beginexclusive(task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	dns_name_format(dns_catz_entry_getname(ev->entry), cname,
			DNS_NAME_FORMATSIZE);
	result = dns_zt_find(ev->view->zonetable,
			     dns_catz_entry_getname(ev->entry), 0, NULL, &zone);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
			      "catz: catz_delzone_taskaction: "
			      "zone '%s' not found", cname);
		goto cleanup;
	}

	if (!dns_zone_getadded(zone)) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
			      "catz: catz_delzone_taskaction: "
			      "zone '%s' is not a dynamically added zone",
			      cname);
		goto cleanup;
	}

	if (dns_zone_get_parentcatz(zone) != ev->origin) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
			      "catz: catz_delzone_taskaction: zone "
			      "'%s' exists in multiple catalog zones",
			      cname);
		goto cleanup;
	}

	/* Stop answering for this zone */
	if (dns_zone_getdb(zone, &dbp) == ISC_R_SUCCESS) {
		dns_db_detach(&dbp);
		dns_zone_unload(zone);
	}

	CHECK(dns_zt_unmount(ev->view->zonetable, zone));
	file = dns_zone_getfile(zone);
	if (file != NULL)
		isc_file_remove(file);

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
		      "catz: catz_delzone_taskaction: "
		      "zone '%s' deleted", cname);
  cleanup:
	isc_task_endexclusive(task);
	if (zone != NULL)
		dns_zone_detach(&zone);
	dns_catz_entry_detach(ev->origin, &ev->entry);
	dns_catz_zone_detach(&ev->origin);
	dns_view_detach(&ev->view);
	isc_event_free(ISC_EVENT_PTR(&ev));
}

static isc_result_t
catz_create_chg_task(dns_catz_entry_t *entry, dns_catz_zone_t *origin,
		     dns_view_t *view, isc_taskmgr_t *taskmgr, void *udata,
		     isc_eventtype_t type)
{
	catz_chgzone_event_t *event;
	isc_task_t *task;
	isc_result_t result;
	isc_taskaction_t action;

	switch (type) {
	case DNS_EVENT_CATZADDZONE:
	case DNS_EVENT_CATZMODZONE:
		action = catz_addmodzone_taskaction;
		break;
	case DNS_EVENT_CATZDELZONE:
		action = catz_delzone_taskaction;
		break;
	default:
		REQUIRE(0);
	}

	event = (catz_chgzone_event_t *) isc_event_allocate(view->mctx, origin,
							    type, action, NULL,
							    sizeof(*event));
	if (event == NULL)
		return (ISC_R_NOMEMORY);

	event->cbd = (catz_cb_data_t *) udata;
	event->entry = NULL;
	event->origin = NULL;
	event->view = NULL;
	event->mod = ISC_TF(type == DNS_EVENT_CATZMODZONE);
	dns_catz_entry_attach(entry, &event->entry);
	dns_catz_zone_attach(origin, &event->origin);
	dns_view_attach(view, &event->view);

	task = NULL;
	result = isc_taskmgr_excltask(taskmgr, &task);
	REQUIRE(result == ISC_R_SUCCESS);
	isc_task_send(task, ISC_EVENT_PTR(&event));
	isc_task_detach(&task);

	return (ISC_R_SUCCESS);
}

static isc_result_t
catz_addzone(dns_catz_entry_t *entry, dns_catz_zone_t *origin,
	     dns_view_t *view, isc_taskmgr_t *taskmgr, void *udata)
{
	return (catz_create_chg_task(entry, origin, view, taskmgr, udata,
				     DNS_EVENT_CATZADDZONE));
}

static isc_result_t
catz_delzone(dns_catz_entry_t *entry, dns_catz_zone_t *origin,
	     dns_view_t *view, isc_taskmgr_t *taskmgr, void *udata)
{
	return (catz_create_chg_task(entry, origin, view, taskmgr, udata,
				     DNS_EVENT_CATZDELZONE));
}

static isc_result_t
catz_modzone(dns_catz_entry_t *entry, dns_catz_zone_t *origin,
	     dns_view_t *view, isc_taskmgr_t *taskmgr, void *udata)
{
	return (catz_create_chg_task(entry, origin, view, taskmgr, udata,
				     DNS_EVENT_CATZMODZONE));
}

static isc_result_t
configure_catz_zone(dns_view_t *view, const cfg_obj_t *config,
		    const cfg_listelt_t *element)
{
	const cfg_obj_t *catz_obj, *obj;
	dns_catz_zone_t *zone = NULL;
	const char *str;
	isc_result_t result;
	dns_name_t origin;
	dns_catz_options_t *opts;
	dns_view_t *pview = NULL;

	dns_name_init(&origin, NULL);
	catz_obj = cfg_listelt_value(element);

	str = cfg_obj_asstring(cfg_tuple_get(catz_obj, "zone name"));

	result = dns_name_fromstring(&origin, str, DNS_NAME_DOWNCASE,
				     view->mctx);
	if (result == ISC_R_SUCCESS && dns_name_equal(&origin, dns_rootname))
		result = DNS_R_EMPTYLABEL;

	if (result != ISC_R_SUCCESS) {
		cfg_obj_log(catz_obj, named_g_lctx, DNS_CATZ_ERROR_LEVEL,
			    "catz: invalid zone name '%s'", str);
		goto cleanup;
	}

	result = dns_catz_add_zone(view->catzs, &origin, &zone);
	if (result != ISC_R_SUCCESS && result != ISC_R_EXISTS) {
		cfg_obj_log(catz_obj, named_g_lctx, DNS_CATZ_ERROR_LEVEL,
			    "catz: unable to create catalog zone '%s', "
			    "error %s",
			    str, isc_result_totext(result));
		goto cleanup;
	}

	if (result == ISC_R_EXISTS) {
		isc_ht_iter_t *it = NULL;

		result = dns_viewlist_find(&named_g_server->viewlist,
					   view->name,
					   view->rdclass, &pview);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);

		/*
		 * xxxwpk todo: reconfigure the zone!!!!
		 */
		cfg_obj_log(catz_obj, named_g_lctx, DNS_CATZ_ERROR_LEVEL,
			    "catz: catalog zone '%s' will not be reconfigured",
			    str);
		/*
		 * We have to walk through all the member zones and attach
		 * them to current view
		 */
		result = dns_catz_get_iterator(zone, &it);
		if (result != ISC_R_SUCCESS) {
			cfg_obj_log(catz_obj, named_g_lctx,
				    DNS_CATZ_ERROR_LEVEL,
				    "catz: unable to create iterator");
			goto cleanup;
		}

		for (result = isc_ht_iter_first(it);
		     result == ISC_R_SUCCESS;
		     result = isc_ht_iter_next(it))
		{
			dns_name_t *name = NULL;
			dns_zone_t *dnszone = NULL;
			dns_catz_entry_t *entry = NULL;
			isc_result_t tresult;

			isc_ht_iter_current(it, (void **) &entry);
			name = dns_catz_entry_getname(entry);

			tresult = dns_view_findzone(pview, name, &dnszone);
			RUNTIME_CHECK(tresult == ISC_R_SUCCESS);

			dns_zone_setview(dnszone, view);
			dns_view_addzone(view, dnszone);

			/*
			 * The dns_view_findzone() call above increments the
			 * zone's reference count, which we need to decrement
			 * back.  However, as dns_zone_detach() sets the
			 * supplied pointer to NULL, calling it is deferred
			 * until the dnszone variable is no longer used.
			 */
			dns_zone_detach(&dnszone);
		}

		isc_ht_iter_destroy(&it);

		result = ISC_R_SUCCESS;
	}

	dns_catz_zone_resetdefoptions(zone);
	opts = dns_catz_zone_getdefoptions(zone);

	obj = cfg_tuple_get(catz_obj, "default-masters");
	if (obj != NULL && cfg_obj_istuple(obj))
		result = named_config_getipandkeylist(config, obj,
						   view->mctx, &opts->masters);

	obj = cfg_tuple_get(catz_obj, "in-memory");
	if (obj != NULL && cfg_obj_isboolean(obj))
		opts->in_memory = cfg_obj_asboolean(obj);

	obj = cfg_tuple_get(catz_obj, "zone-directory");
	if (!opts->in_memory && obj != NULL && cfg_obj_isstring(obj)) {
		opts->zonedir = isc_mem_strdup(view->mctx,
					       cfg_obj_asstring(obj));
		if (isc_file_isdirectory(opts->zonedir) != ISC_R_SUCCESS) {
			cfg_obj_log(obj, named_g_lctx, DNS_CATZ_ERROR_LEVEL,
				    "catz: zone-directory '%s' "
				    "not found; zone files will not be "
				    "saved", opts->zonedir);
			opts->in_memory = ISC_TRUE;
		}
	}

	obj = cfg_tuple_get(catz_obj, "min-update-interval");
	if (obj != NULL && cfg_obj_isuint32(obj))
		opts->min_update_interval = cfg_obj_asuint32(obj);

  cleanup:
	if (pview != NULL)
		dns_view_detach(&pview);
	dns_name_free(&origin, view->mctx);

	return (result);
}

static catz_cb_data_t ns_catz_cbdata;
static dns_catz_zonemodmethods_t ns_catz_zonemodmethods = {
	catz_addzone,
	catz_modzone,
	catz_delzone,
	&ns_catz_cbdata
};

static isc_result_t
configure_catz(dns_view_t *view, const cfg_obj_t *config,
	       const cfg_obj_t *catz_obj)
{
	const cfg_listelt_t *zone_element;
	const dns_catz_zones_t *old = NULL;
	dns_view_t *pview = NULL;
	isc_result_t result;

	/* xxxwpk TODO do it cleaner, once, somewhere */
	ns_catz_cbdata.server = named_g_server;

	zone_element = cfg_list_first(cfg_tuple_get(catz_obj, "zone list"));
	if (zone_element == NULL)
		return (ISC_R_SUCCESS);

	CHECK(dns_catz_new_zones(&view->catzs, &ns_catz_zonemodmethods,
				 view->mctx, named_g_taskmgr,
				 named_g_timermgr));

	result = dns_viewlist_find(&named_g_server->viewlist, view->name,
				   view->rdclass, &pview);
	if (result == ISC_R_SUCCESS)
		old = pview->catzs;

	if (old != NULL) {
		dns_catz_catzs_detach(&view->catzs);
		dns_catz_catzs_attach(pview->catzs, &view->catzs);
		dns_catz_prereconfig(view->catzs);
	}

	while (zone_element != NULL) {
		CHECK(configure_catz_zone(view, config, zone_element));
		zone_element = cfg_list_next(zone_element);
	}

	if (old != NULL)
		dns_catz_postreconfig(view->catzs);

	result = ISC_R_SUCCESS;

  cleanup:
	if (pview != NULL)
		dns_view_detach(&pview);

	return (result);
}

#define CHECK_RRL(cond, pat, val1, val2)				\
	do {								\
		if (!(cond)) {						\
			cfg_obj_log(obj, named_g_lctx, ISC_LOG_ERROR,	\
				    pat, val1, val2);			\
			result = ISC_R_RANGE;				\
			goto cleanup;					\
		    }							\
	} while (/*CONSTCOND*/ 0)

#define CHECK_RRL_RATE(rate, def, max_rate, name)			\
	do {								\
		obj = NULL;						\
		rrl->rate.str = name;					\
		result = cfg_map_get(map, name, &obj);			\
		if (result == ISC_R_SUCCESS) {				\
			rrl->rate.r = cfg_obj_asuint32(obj);		\
			CHECK_RRL(rrl->rate.r <= max_rate,		\
				  name" %d > %d",			\
				  rrl->rate.r, max_rate);		\
		} else {						\
			rrl->rate.r = def;				\
		}							\
		rrl->rate.scaled = rrl->rate.r;				\
	} while (/*CONSTCOND*/0)

static isc_result_t
configure_rrl(dns_view_t *view, const cfg_obj_t *config, const cfg_obj_t *map) {
	const cfg_obj_t *obj;
	dns_rrl_t *rrl;
	isc_result_t result;
	int min_entries, i, j;

	/*
	 * Most DNS servers have few clients, but intentinally open
	 * recursive and authoritative servers often have many.
	 * So start with a small number of entries unless told otherwise
	 * to reduce cold-start costs.
	 */
	min_entries = 500;
	obj = NULL;
	result = cfg_map_get(map, "min-table-size", &obj);
	if (result == ISC_R_SUCCESS) {
		min_entries = cfg_obj_asuint32(obj);
		if (min_entries < 1)
			min_entries = 1;
	}
	result = dns_rrl_init(&rrl, view, min_entries);
	if (result != ISC_R_SUCCESS)
		return (result);

	i = ISC_MAX(20000, min_entries);
	obj = NULL;
	result = cfg_map_get(map, "max-table-size", &obj);
	if (result == ISC_R_SUCCESS) {
		i = cfg_obj_asuint32(obj);
		CHECK_RRL(i >= min_entries,
			  "max-table-size %d < min-table-size %d",
			  i, min_entries);
	}
	rrl->max_entries = i;

	CHECK_RRL_RATE(responses_per_second, 0, DNS_RRL_MAX_RATE,
		       "responses-per-second");
	CHECK_RRL_RATE(referrals_per_second,
		       rrl->responses_per_second.r, DNS_RRL_MAX_RATE,
		       "referrals-per-second");
	CHECK_RRL_RATE(nodata_per_second,
		       rrl->responses_per_second.r, DNS_RRL_MAX_RATE,
		       "nodata-per-second");
	CHECK_RRL_RATE(nxdomains_per_second,
		       rrl->responses_per_second.r, DNS_RRL_MAX_RATE,
		       "nxdomains-per-second");
	CHECK_RRL_RATE(errors_per_second,
		       rrl->responses_per_second.r, DNS_RRL_MAX_RATE,
		       "errors-per-second");

	CHECK_RRL_RATE(all_per_second, 0, DNS_RRL_MAX_RATE,
		       "all-per-second");

	CHECK_RRL_RATE(slip, 2, DNS_RRL_MAX_SLIP,
		       "slip");

	i = 15;
	obj = NULL;
	result = cfg_map_get(map, "window", &obj);
	if (result == ISC_R_SUCCESS) {
		i = cfg_obj_asuint32(obj);
		CHECK_RRL(i >= 1 && i <= DNS_RRL_MAX_WINDOW,
			  "window %d < 1 or > %d", i, DNS_RRL_MAX_WINDOW);
	}
	rrl->window = i;

	i = 0;
	obj = NULL;
	result = cfg_map_get(map, "qps-scale", &obj);
	if (result == ISC_R_SUCCESS) {
		i = cfg_obj_asuint32(obj);
		CHECK_RRL(i >= 1, "invalid 'qps-scale %d'%s", i, "");
	}
	rrl->qps_scale = i;
	rrl->qps = 1.0;

	i = 24;
	obj = NULL;
	result = cfg_map_get(map, "ipv4-prefix-length", &obj);
	if (result == ISC_R_SUCCESS) {
		i = cfg_obj_asuint32(obj);
		CHECK_RRL(i >= 8 && i <= 32,
			  "invalid 'ipv4-prefix-length %d'%s", i, "");
	}
	rrl->ipv4_prefixlen = i;
	if (i == 32)
		rrl->ipv4_mask = 0xffffffff;
	else
		rrl->ipv4_mask = htonl(0xffffffff << (32-i));

	i = 56;
	obj = NULL;
	result = cfg_map_get(map, "ipv6-prefix-length", &obj);
	if (result == ISC_R_SUCCESS) {
		i = cfg_obj_asuint32(obj);
		CHECK_RRL(i >= 16 && i <= DNS_RRL_MAX_PREFIX,
			  "ipv6-prefix-length %d < 16 or > %d",
			  i, DNS_RRL_MAX_PREFIX);
	}
	rrl->ipv6_prefixlen = i;
	for (j = 0; j < 4; ++j) {
		if (i <= 0) {
			rrl->ipv6_mask[j] = 0;
		} else if (i < 32) {
			rrl->ipv6_mask[j] = htonl(0xffffffff << (32-i));
		} else {
			rrl->ipv6_mask[j] = 0xffffffff;
		}
		i -= 32;
	}

	obj = NULL;
	result = cfg_map_get(map, "exempt-clients", &obj);
	if (result == ISC_R_SUCCESS) {
		result = cfg_acl_fromconfig(obj, config, named_g_lctx,
					    named_g_aclconfctx, named_g_mctx,
					    0, &rrl->exempt);
		CHECK_RRL(result == ISC_R_SUCCESS,
			  "invalid %s%s", "address match list", "");
	}

	obj = NULL;
	result = cfg_map_get(map, "log-only", &obj);
	if (result == ISC_R_SUCCESS && cfg_obj_asboolean(obj))
		rrl->log_only = ISC_TRUE;
	else
		rrl->log_only = ISC_FALSE;

	return (ISC_R_SUCCESS);

 cleanup:
	dns_rrl_view_destroy(view);
	return (result);
}

static isc_result_t
add_soa(dns_db_t *db, dns_dbversion_t *version, const dns_name_t *name,
	const dns_name_t *origin, const dns_name_t *contact)
{
	dns_dbnode_t *node = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdatalist_t rdatalist;
	dns_rdataset_t rdataset;
	isc_result_t result;
	unsigned char buf[DNS_SOA_BUFFERSIZE];

	CHECK(dns_soa_buildrdata(origin, contact, dns_db_class(db),
				 0, 28800, 7200, 604800, 86400, buf, &rdata));

	dns_rdatalist_init(&rdatalist);
	rdatalist.type = rdata.type;
	rdatalist.rdclass = rdata.rdclass;
	rdatalist.ttl = 86400;
	ISC_LIST_APPEND(rdatalist.rdata, &rdata, link);

	dns_rdataset_init(&rdataset);
	CHECK(dns_rdatalist_tordataset(&rdatalist, &rdataset));
	CHECK(dns_db_findnode(db, name, ISC_TRUE, &node));
	CHECK(dns_db_addrdataset(db, node, version, 0, &rdataset, 0, NULL));

 cleanup:
	if (node != NULL)
		dns_db_detachnode(db, &node);
	return (result);
}

static isc_result_t
add_ns(dns_db_t *db, dns_dbversion_t *version, const dns_name_t *name,
       const dns_name_t *nsname)
{
	dns_dbnode_t *node = NULL;
	dns_rdata_ns_t ns;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdatalist_t rdatalist;
	dns_rdataset_t rdataset;
	isc_result_t result;
	isc_buffer_t b;
	unsigned char buf[DNS_NAME_MAXWIRE];

	isc_buffer_init(&b, buf, sizeof(buf));

	ns.common.rdtype = dns_rdatatype_ns;
	ns.common.rdclass = dns_db_class(db);
	ns.mctx = NULL;
	dns_name_init(&ns.name, NULL);
	dns_name_clone(nsname, &ns.name);
	CHECK(dns_rdata_fromstruct(&rdata, dns_db_class(db), dns_rdatatype_ns,
				   &ns, &b));

	dns_rdatalist_init(&rdatalist);
	rdatalist.type = rdata.type;
	rdatalist.rdclass = rdata.rdclass;
	rdatalist.ttl = 86400;
	ISC_LIST_APPEND(rdatalist.rdata, &rdata, link);

	dns_rdataset_init(&rdataset);
	CHECK(dns_rdatalist_tordataset(&rdatalist, &rdataset));
	CHECK(dns_db_findnode(db, name, ISC_TRUE, &node));
	CHECK(dns_db_addrdataset(db, node, version, 0, &rdataset, 0, NULL));

 cleanup:
	if (node != NULL)
		dns_db_detachnode(db, &node);
	return (result);
}

static isc_result_t
create_empty_zone(dns_zone_t *zone, dns_name_t *name, dns_view_t *view,
		  const cfg_obj_t *zonelist, const char **empty_dbtype,
		  int empty_dbtypec, dns_zonestat_level_t statlevel)
{
	char namebuf[DNS_NAME_FORMATSIZE];
	const cfg_listelt_t *element;
	const cfg_obj_t *obj;
	const cfg_obj_t *zconfig;
	const cfg_obj_t *zoptions;
	const char *rbt_dbtype[4] = { "rbt" };
	const char *sep = ": view ";
	const char *str;
	const char *viewname = view->name;
	dns_db_t *db = NULL;
	dns_dbversion_t *version = NULL;
	dns_fixedname_t cfixed;
	dns_fixedname_t fixed;
	dns_fixedname_t nsfixed;
	dns_name_t *contact;
	dns_name_t *ns;
	dns_name_t *zname;
	dns_zone_t *myzone = NULL;
	int rbt_dbtypec = 1;
	isc_result_t result;
	dns_namereln_t namereln;
	int order;
	unsigned int nlabels;

	zname = dns_fixedname_initname(&fixed);
	ns = dns_fixedname_initname(&nsfixed);
	contact = dns_fixedname_initname(&cfixed);

	/*
	 * Look for forward "zones" beneath this empty zone and if so
	 * create a custom db for the empty zone.
	 */
	for (element = cfg_list_first(zonelist);
	     element != NULL;
	     element = cfg_list_next(element)) {

		zconfig = cfg_listelt_value(element);
		str = cfg_obj_asstring(cfg_tuple_get(zconfig, "name"));
		CHECK(dns_name_fromstring(zname, str, 0, NULL));
		namereln = dns_name_fullcompare(zname, name, &order, &nlabels);
		if (namereln != dns_namereln_subdomain)
			continue;

		zoptions = cfg_tuple_get(zconfig, "options");

		obj = NULL;
		(void)cfg_map_get(zoptions, "type", &obj);
		if (obj != NULL &&
		    strcasecmp(cfg_obj_asstring(obj), "forward") == 0) {
			obj = NULL;
			(void)cfg_map_get(zoptions, "forward", &obj);
			if (obj == NULL)
				continue;
			if (strcasecmp(cfg_obj_asstring(obj), "only") != 0)
				continue;
		}
		if (db == NULL) {
			CHECK(dns_db_create(view->mctx, "rbt", name,
					    dns_dbtype_zone, view->rdclass,
					    0, NULL, &db));
			CHECK(dns_db_newversion(db, &version));
			if (strcmp(empty_dbtype[2], "@") == 0)
				dns_name_clone(name, ns);
			else
				CHECK(dns_name_fromstring(ns, empty_dbtype[2],
							  0, NULL));
			CHECK(dns_name_fromstring(contact, empty_dbtype[3],
						  0, NULL));
			CHECK(add_soa(db, version, name, ns, contact));
			CHECK(add_ns(db, version, name, ns));
		}
		CHECK(add_ns(db, version, zname, dns_rootname));
	}

	/*
	 * Is the existing zone the ok to use?
	 */
	if (zone != NULL) {
		unsigned int typec;
		const char **dbargv;

		if (db != NULL) {
			typec = rbt_dbtypec;
			dbargv = rbt_dbtype;
		} else {
			typec = empty_dbtypec;
			dbargv = empty_dbtype;
		}

		result = check_dbtype(zone, typec, dbargv, view->mctx);
		if (result != ISC_R_SUCCESS)
			zone = NULL;

		if (zone != NULL && dns_zone_gettype(zone) != dns_zone_master)
			zone = NULL;
		if (zone != NULL && dns_zone_getfile(zone) != NULL)
			zone = NULL;
		if (zone != NULL) {
			dns_zone_getraw(zone, &myzone);
			if (myzone != NULL) {
				dns_zone_detach(&myzone);
				zone = NULL;
			}
		}
	}

	if (zone == NULL) {
		CHECK(dns_zonemgr_createzone(named_g_server->zonemgr, &myzone));
		zone = myzone;
		CHECK(dns_zone_setorigin(zone, name));
		CHECK(dns_zonemgr_managezone(named_g_server->zonemgr, zone));
		if (db == NULL)
			CHECK(dns_zone_setdbtype(zone, empty_dbtypec,
						 empty_dbtype));
		dns_zone_setclass(zone, view->rdclass);
		dns_zone_settype(zone, dns_zone_master);
		dns_zone_setstats(zone, named_g_server->zonestats);
	}

	dns_zone_setoption(zone, ~DNS_ZONEOPT_NOCHECKNS, ISC_FALSE);
	dns_zone_setoption(zone, DNS_ZONEOPT_NOCHECKNS, ISC_TRUE);
	dns_zone_setnotifytype(zone, dns_notifytype_no);
	dns_zone_setdialup(zone, dns_dialuptype_no);
	dns_zone_setautomatic(zone, ISC_TRUE);
	if (view->queryacl != NULL)
		dns_zone_setqueryacl(zone, view->queryacl);
	else
		dns_zone_clearqueryacl(zone);
	if (view->queryonacl != NULL)
		dns_zone_setqueryonacl(zone, view->queryonacl);
	else
		dns_zone_clearqueryonacl(zone);
	dns_zone_clearupdateacl(zone);
	if (view->transferacl != NULL)
		dns_zone_setxfracl(zone, view->transferacl);
	else
		dns_zone_clearxfracl(zone);

	CHECK(setquerystats(zone, view->mctx, statlevel));
	if (db != NULL) {
		dns_db_closeversion(db, &version, ISC_TRUE);
		CHECK(dns_zone_replacedb(zone, db, ISC_FALSE));
	}
	dns_zone_setoption2(zone, DNS_ZONEOPT2_AUTOEMPTY, ISC_TRUE);
	dns_zone_setview(zone, view);
	CHECK(dns_view_addzone(view, zone));

	if (!strcmp(viewname, "_default")) {
		sep = "";
		viewname = "";
	}
	dns_name_format(name, namebuf, sizeof(namebuf));
	isc_log_write(named_g_lctx, DNS_LOGCATEGORY_ZONELOAD,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "automatic empty zone%s%s: %s",
		      sep, viewname, namebuf);

 cleanup:
	if (myzone != NULL)
		dns_zone_detach(&myzone);
	if (version != NULL)
		dns_db_closeversion(db, &version, ISC_FALSE);
	if (db != NULL)
		dns_db_detach(&db);

	INSIST(version == NULL);

	return (result);
}

#ifdef HAVE_DNSTAP
static isc_result_t
configure_dnstap(const cfg_obj_t **maps, dns_view_t *view) {
	isc_result_t result;
	const cfg_obj_t *obj, *obj2;
	const cfg_listelt_t *element;
	const char *dpath = named_g_defaultdnstap;
	const cfg_obj_t *dlist = NULL;
	dns_dtmsgtype_t dttypes = 0;
	unsigned int i;
	struct fstrm_iothr_options *fopt = NULL;

	result = named_config_get(maps, "dnstap", &dlist);
	if (result != ISC_R_SUCCESS)
		return (ISC_R_SUCCESS);

	for (element = cfg_list_first(dlist);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const char *str;
		dns_dtmsgtype_t dt = 0;

		obj = cfg_listelt_value(element);
		obj2 = cfg_tuple_get(obj, "type");
		str = cfg_obj_asstring(obj2);
		if (strcasecmp(str, "client") == 0) {
			dt |= DNS_DTTYPE_CQ|DNS_DTTYPE_CR;
		} else if (strcasecmp(str, "auth") == 0) {
			dt |= DNS_DTTYPE_AQ|DNS_DTTYPE_AR;
		} else if (strcasecmp(str, "resolver") == 0) {
			dt |= DNS_DTTYPE_RQ|DNS_DTTYPE_RR;
		} else if (strcasecmp(str, "forwarder") == 0) {
			dt |= DNS_DTTYPE_FQ|DNS_DTTYPE_FR;
		} else if (strcasecmp(str, "all") == 0) {
			dt |= DNS_DTTYPE_CQ|DNS_DTTYPE_CR|
			      DNS_DTTYPE_AQ|DNS_DTTYPE_AR|
			      DNS_DTTYPE_RQ|DNS_DTTYPE_RR|
			      DNS_DTTYPE_FQ|DNS_DTTYPE_FR;
		}

		obj2 = cfg_tuple_get(obj, "mode");
		if (obj2 == NULL || cfg_obj_isvoid(obj2)) {
			dttypes |= dt;
			continue;
		}

		str = cfg_obj_asstring(obj2);
		if (strcasecmp(str, "query") == 0) {
			dt &= ~DNS_DTTYPE_RESPONSE;
		} else if (strcasecmp(str, "response") == 0) {
			dt &= ~DNS_DTTYPE_QUERY;
		}

		dttypes |= dt;
	}

	if (named_g_server->dtenv == NULL && dttypes != 0) {
		dns_dtmode_t dmode;
		isc_uint64_t max_size = 0;
		isc_uint32_t rolls = 0;
		isc_log_rollsuffix_t suffix = isc_log_rollsuffix_increment;

		obj = NULL;
		CHECKM(named_config_get(maps, "dnstap-output", &obj),
		       "'dnstap-output' must be set if 'dnstap' is set");

		obj2 = cfg_tuple_get(obj, "mode");
		if (obj2 == NULL)
			CHECKM(ISC_R_FAILURE, "dnstap-output mode not found");
		if (strcasecmp(cfg_obj_asstring(obj2), "file") == 0)
			dmode = dns_dtmode_file;
		else
			dmode = dns_dtmode_unix;

		obj2 = cfg_tuple_get(obj, "path");
		if (obj2 == NULL)
			CHECKM(ISC_R_FAILURE, "dnstap-output path not found");

		dpath = cfg_obj_asstring(obj2);

		obj2 = cfg_tuple_get(obj, "size");
		if (obj2 != NULL && cfg_obj_isuint64(obj2)) {
			max_size = cfg_obj_asuint64(obj2);
			if (max_size > SIZE_MAX) {
				cfg_obj_log(obj2, named_g_lctx,
				    ISC_LOG_WARNING,
				    "'dnstap-output size "
				    "%" ISC_PRINT_QUADFORMAT "u' "
				    "is too large for this "
				    "system; reducing to %lu",
				    max_size, (unsigned long)SIZE_MAX);
				max_size = SIZE_MAX;
			}
		}

		obj2 = cfg_tuple_get(obj, "versions");
		if (obj2 != NULL && cfg_obj_isuint32(obj2)) {
			rolls = cfg_obj_asuint32(obj2);
		} else {
			rolls = ISC_LOG_ROLLINFINITE;
		}

		obj2 = cfg_tuple_get(obj, "suffix");
		if (obj2 != NULL && cfg_obj_isstring(obj2) &&
		    strcasecmp(cfg_obj_asstring(obj2), "timestamp") == 0)
		{
			suffix = isc_log_rollsuffix_timestamp;
		}

		fopt = fstrm_iothr_options_init();
		fstrm_iothr_options_set_num_input_queues(fopt, named_g_cpus);
		fstrm_iothr_options_set_queue_model(fopt,
						 FSTRM_IOTHR_QUEUE_MODEL_MPSC);

		obj = NULL;
		result = named_config_get(maps, "fstrm-set-buffer-hint", &obj);
		if (result == ISC_R_SUCCESS) {
			i = cfg_obj_asuint32(obj);
			fstrm_iothr_options_set_buffer_hint(fopt, i);
		}

		obj = NULL;
		result = named_config_get(maps, "fstrm-set-flush-timeout",
					  &obj);
		if (result == ISC_R_SUCCESS) {
			i = cfg_obj_asuint32(obj);
			fstrm_iothr_options_set_flush_timeout(fopt, i);
		}

		obj = NULL;
		result = named_config_get(maps, "fstrm-set-input-queue-size",
				       &obj);
		if (result == ISC_R_SUCCESS) {
			i = cfg_obj_asuint32(obj);
			fstrm_iothr_options_set_input_queue_size(fopt, i);
		}

		obj = NULL;
		result = named_config_get(maps,
				       "fstrm-set-output-notify-threshold",
				       &obj);
		if (result == ISC_R_SUCCESS) {
			i = cfg_obj_asuint32(obj);
			fstrm_iothr_options_set_queue_notify_threshold(fopt,
								       i);
		}

		obj = NULL;
		result = named_config_get(maps, "fstrm-set-output-queue-model",
				       &obj);
		if (result == ISC_R_SUCCESS) {
			if (strcasecmp(cfg_obj_asstring(obj), "spsc") == 0)
				i = FSTRM_IOTHR_QUEUE_MODEL_SPSC;
			else
				i = FSTRM_IOTHR_QUEUE_MODEL_MPSC;
			fstrm_iothr_options_set_queue_model(fopt, i);
		}

		obj = NULL;
		result = named_config_get(maps, "fstrm-set-output-queue-size",
				       &obj);
		if (result == ISC_R_SUCCESS) {
			i = cfg_obj_asuint32(obj);
			fstrm_iothr_options_set_output_queue_size(fopt, i);
		}

		obj = NULL;
		result = named_config_get(maps, "fstrm-set-reopen-interval",
				       &obj);
		if (result == ISC_R_SUCCESS) {
			i = cfg_obj_asuint32(obj);
			fstrm_iothr_options_set_reopen_interval(fopt, i);
		}

		CHECKM(dns_dt_create2(named_g_mctx, dmode, dpath,
				      &fopt, named_g_server->task,
				      &named_g_server->dtenv),
		       "unable to create dnstap environment");

		CHECKM(dns_dt_setupfile(named_g_server->dtenv,
					max_size, rolls, suffix),
		       "unable to set up dnstap logfile");
	}

	if (named_g_server->dtenv == NULL)
		return (ISC_R_SUCCESS);

	obj = NULL;
	result = named_config_get(maps, "dnstap-version", &obj);
	if (result != ISC_R_SUCCESS) {
		/* not specified; use the product and version */
		dns_dt_setversion(named_g_server->dtenv, PRODUCT " " VERSION);
	} else if (result == ISC_R_SUCCESS && !cfg_obj_isvoid(obj)) {
		/* Quoted string */
		dns_dt_setversion(named_g_server->dtenv, cfg_obj_asstring(obj));
	}

	obj = NULL;
	result = named_config_get(maps, "dnstap-identity", &obj);
	if (result == ISC_R_SUCCESS && cfg_obj_isboolean(obj)) {
		/* "hostname" is interpreted as boolean ISC_TRUE */
		char buf[256];
		result = named_os_gethostname(buf, sizeof(buf));
		if (result == ISC_R_SUCCESS)
			dns_dt_setidentity(named_g_server->dtenv, buf);
	} else if (result == ISC_R_SUCCESS && !cfg_obj_isvoid(obj)) {
		/* Quoted string */
		dns_dt_setidentity(named_g_server->dtenv,
				   cfg_obj_asstring(obj));
	}

	dns_dt_attach(named_g_server->dtenv, &view->dtenv);
	view->dttypes = dttypes;

	result = ISC_R_SUCCESS;

 cleanup:
	if (fopt != NULL)
		fstrm_iothr_options_destroy(&fopt);

	return (result);
}
#endif /* HAVE_DNSTAP */

static isc_result_t
create_mapped_acl(void) {
	isc_result_t result;
	dns_acl_t *acl = NULL;
	struct in6_addr in6 = IN6ADDR_V4MAPPED_INIT;
	isc_netaddr_t addr;

	isc_netaddr_fromin6(&addr, &in6);

	result = dns_acl_create(named_g_mctx, 1, &acl);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_iptable_addprefix2(acl->iptable, &addr, 96,
					ISC_TRUE, ISC_FALSE);
	if (result == ISC_R_SUCCESS)
		dns_acl_attach(acl, &named_g_mapped);
	dns_acl_detach(&acl);
	return (result);
}

/*
 * Configure 'view' according to 'vconfig', taking defaults from 'config'
 * where values are missing in 'vconfig'.
 *
 * When configuring the default view, 'vconfig' will be NULL and the
 * global defaults in 'config' used exclusively.
 */
static isc_result_t
configure_view(dns_view_t *view, dns_viewlist_t *viewlist,
	       cfg_obj_t *config, cfg_obj_t *vconfig,
	       named_cachelist_t *cachelist, const cfg_obj_t *bindkeys,
	       isc_mem_t *mctx, cfg_aclconfctx_t *actx,
	       isc_boolean_t need_hints)
{
	const cfg_obj_t *maps[4];
	const cfg_obj_t *cfgmaps[3];
	const cfg_obj_t *optionmaps[3];
	const cfg_obj_t *options = NULL;
	const cfg_obj_t *voptions = NULL;
	const cfg_obj_t *forwardtype;
	const cfg_obj_t *forwarders;
	const cfg_obj_t *alternates;
	const cfg_obj_t *zonelist;
	const cfg_obj_t *dlzlist;
	const cfg_obj_t *dlz;
	const cfg_obj_t *dlvobj = NULL;
	unsigned int dlzargc;
	char **dlzargv;
	const cfg_obj_t *dyndb_list;
	const cfg_obj_t *disabled;
	const cfg_obj_t *obj, *obj2;
	const cfg_listelt_t *element;
	in_port_t port;
	dns_cache_t *cache = NULL;
	isc_result_t result;
	unsigned int cleaning_interval;
	size_t max_cache_size;
	isc_uint32_t max_cache_size_percent = 0;
	size_t max_adb_size;
	isc_uint32_t lame_ttl, fail_ttl;
	isc_uint32_t max_stale_ttl;
	dns_tsig_keyring_t *ring = NULL;
	dns_view_t *pview = NULL;	/* Production view */
	isc_mem_t *cmctx = NULL, *hmctx = NULL;
	dns_dispatch_t *dispatch4 = NULL;
	dns_dispatch_t *dispatch6 = NULL;
	isc_boolean_t reused_cache = ISC_FALSE;
	isc_boolean_t shared_cache = ISC_FALSE;
	int i = 0, j = 0, k = 0;
	const char *str;
	const char *cachename = NULL;
	dns_order_t *order = NULL;
	isc_uint32_t udpsize;
	isc_uint32_t maxbits;
	unsigned int resopts = 0;
	dns_zone_t *zone = NULL;
	isc_uint32_t max_clients_per_query;
	isc_boolean_t empty_zones_enable;
	const cfg_obj_t *disablelist = NULL;
	isc_stats_t *resstats = NULL;
	dns_stats_t *resquerystats = NULL;
	isc_boolean_t auto_root = ISC_FALSE;
	named_cache_t *nsc;
	isc_boolean_t zero_no_soattl;
	dns_acl_t *clients = NULL, *mapped = NULL, *excluded = NULL;
	unsigned int query_timeout, ndisp;
	isc_boolean_t old_rpz_ok = ISC_FALSE;
	isc_dscp_t dscp4 = -1, dscp6 = -1;
	dns_dyndbctx_t *dctx = NULL;
	unsigned int resolver_param;

	REQUIRE(DNS_VIEW_VALID(view));

	if (config != NULL)
		(void)cfg_map_get(config, "options", &options);

	/*
	 * maps: view options, options, defaults
	 * cfgmaps: view options, config
	 * optionmaps: view options, options
	 */
	if (vconfig != NULL) {
		voptions = cfg_tuple_get(vconfig, "options");
		maps[i++] = voptions;
		optionmaps[j++] = voptions;
		cfgmaps[k++] = voptions;
	}
	if (options != NULL) {
		maps[i++] = options;
		optionmaps[j++] = options;
	}

	maps[i++] = named_g_defaults;
	maps[i] = NULL;
	optionmaps[j] = NULL;
	if (config != NULL)
		cfgmaps[k++] = config;
	cfgmaps[k] = NULL;

	/*
	 * Set the view's port number for outgoing queries.
	 */
	CHECKM(named_config_getport(config, &port), "port");
	dns_view_setdstport(view, port);

	/*
	 * Make the list of response policy zone names for a view that
	 * is used for real lookups and so cares about hints.
	 */
	obj = NULL;
	if (view->rdclass == dns_rdataclass_in && need_hints &&
	    named_config_get(maps, "response-policy", &obj) == ISC_R_SUCCESS) {
		CHECK(configure_rpz(view, maps, obj, &old_rpz_ok));
	}

	obj = NULL;
	if (view->rdclass == dns_rdataclass_in && need_hints &&
	    named_config_get(maps, "catalog-zones", &obj) == ISC_R_SUCCESS) {
		CHECK(configure_catz(view, config, obj));
	}

	/*
	 * Configure the zones.
	 */
	zonelist = NULL;
	if (voptions != NULL)
		(void)cfg_map_get(voptions, "zone", &zonelist);
	else
		(void)cfg_map_get(config, "zone", &zonelist);

	/*
	 * Load zone configuration
	 */
	for (element = cfg_list_first(zonelist);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *zconfig = cfg_listelt_value(element);
		CHECK(configure_zone(config, zconfig, vconfig, mctx, view,
				     viewlist, actx, ISC_FALSE, old_rpz_ok,
				     ISC_FALSE));
	}

	/*
	 * Check that a master or slave zone was found for each
	 * zone named in the response policy statement
	 * unless we are using RPZ service interface.
	 */
	if (view->rpzs != NULL && !view->rpzs->p.dnsrps_enabled) {
		dns_rpz_num_t n;

		for (n = 0; n < view->rpzs->p.num_zones; ++n) {
			if ((view->rpzs->defined & DNS_RPZ_ZBIT(n)) == 0) {
				char namebuf[DNS_NAME_FORMATSIZE];

				dns_name_format(&view->rpzs->zones[n]->origin,
						namebuf, sizeof(namebuf));
				isc_log_write(named_g_lctx,
					      NAMED_LOGCATEGORY_GENERAL,
					      NAMED_LOGMODULE_SERVER,
					      DNS_RPZ_ERROR_LEVEL, "rpz '%s'"
					      " is not a master or slave zone",
					      namebuf);
				result = ISC_R_NOTFOUND;
				goto cleanup;
			}
		}
	}

	/*
	 * If we're allowing added zones, then load zone configuration
	 * from the newzone file for zones that were added during previous
	 * runs.
	 */
	CHECK(configure_newzones(view, config, vconfig, mctx, actx));

	/*
	 * Create Dynamically Loadable Zone driver.
	 */
	dlzlist = NULL;
	if (voptions != NULL)
		(void)cfg_map_get(voptions, "dlz", &dlzlist);
	else
		(void)cfg_map_get(config, "dlz", &dlzlist);

	for (element = cfg_list_first(dlzlist);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		dlz = cfg_listelt_value(element);

		obj = NULL;
		(void)cfg_map_get(dlz, "database", &obj);
		if (obj != NULL) {
			dns_dlzdb_t *dlzdb = NULL;
			const cfg_obj_t *name, *search = NULL;
			char *s = isc_mem_strdup(mctx, cfg_obj_asstring(obj));

			if (s == NULL) {
				result = ISC_R_NOMEMORY;
				goto cleanup;
			}

			result = isc_commandline_strtoargv(mctx, s, &dlzargc,
							   &dlzargv, 0);
			if (result != ISC_R_SUCCESS) {
				isc_mem_free(mctx, s);
				goto cleanup;
			}

			name = cfg_map_getname(dlz);
			result = dns_dlzcreate(mctx, cfg_obj_asstring(name),
					       dlzargv[0], dlzargc, dlzargv,
					       &dlzdb);
			isc_mem_free(mctx, s);
			isc_mem_put(mctx, dlzargv, dlzargc * sizeof(*dlzargv));
			if (result != ISC_R_SUCCESS)
				goto cleanup;

			/*
			 * If the DLZ backend supports configuration,
			 * and is searchable, then call its configure
			 * method now.  If not searchable, we'll take
			 * care of it when we process the zone statement.
			 */
			(void)cfg_map_get(dlz, "search", &search);
			if (search == NULL || cfg_obj_asboolean(search)) {
				dlzdb->search = ISC_TRUE;
				result = dns_dlzconfigure(view, dlzdb,
							dlzconfigure_callback);
				if (result != ISC_R_SUCCESS)
					goto cleanup;
				ISC_LIST_APPEND(view->dlz_searched,
						dlzdb, link);
			} else {
				dlzdb->search = ISC_FALSE;
				ISC_LIST_APPEND(view->dlz_unsearched,
						dlzdb, link);
			}

		}
	}

	/*
	 * Obtain configuration parameters that affect the decision of whether
	 * we can reuse/share an existing cache.
	 */
	obj = NULL;
	result = named_config_get(maps, "cleaning-interval", &obj);
	INSIST(result == ISC_R_SUCCESS);
	cleaning_interval = cfg_obj_asuint32(obj) * 60;

	obj = NULL;
	result = named_config_get(maps, "max-cache-size", &obj);
	INSIST(result == ISC_R_SUCCESS);
	if (cfg_obj_isstring(obj)) {
		str = cfg_obj_asstring(obj);
		INSIST(strcasecmp(str, "unlimited") == 0);
		max_cache_size = 0;
	} else if (cfg_obj_ispercentage(obj)) {
		max_cache_size = SIZE_AS_PERCENT;
		max_cache_size_percent = cfg_obj_aspercentage(obj);
	} else {
		isc_resourcevalue_t value;
		value = cfg_obj_asuint64(obj);
		if (value > SIZE_MAX) {
			cfg_obj_log(obj, named_g_lctx,
				    ISC_LOG_WARNING,
				    "'max-cache-size "
				    "%" ISC_PRINT_QUADFORMAT "u' "
				    "is too large for this "
				    "system; reducing to %lu",
				    value, (unsigned long)SIZE_MAX);
			value = SIZE_MAX;
		}
		max_cache_size = (size_t) value;
	}

	if (max_cache_size == SIZE_AS_PERCENT) {
		isc_uint64_t totalphys = isc_meminfo_totalphys();

		max_cache_size =
			(size_t) (totalphys * max_cache_size_percent/100);
		if (totalphys == 0) {
			cfg_obj_log(obj, named_g_lctx,
				ISC_LOG_WARNING,
				"Unable to determine amount of physical "
				"memory, setting 'max-cache-size' to "
				"unlimited");
		} else {
			cfg_obj_log(obj, named_g_lctx,
				ISC_LOG_INFO,
				"'max-cache-size %d%%' "
				"- setting to %" ISC_PRINT_QUADFORMAT "uMB "
				"(out of %" ISC_PRINT_QUADFORMAT "uMB)",
				max_cache_size_percent,
				(isc_uint64_t)(max_cache_size / (1024*1024)),
				totalphys / (1024*1024));
		}
	}

	/* Check-names. */
	obj = NULL;
	result = named_checknames_get(maps, "response", &obj);
	INSIST(result == ISC_R_SUCCESS);

	str = cfg_obj_asstring(obj);
	if (strcasecmp(str, "fail") == 0) {
		resopts |= DNS_RESOLVER_CHECKNAMES |
			DNS_RESOLVER_CHECKNAMESFAIL;
		view->checknames = ISC_TRUE;
	} else if (strcasecmp(str, "warn") == 0) {
		resopts |= DNS_RESOLVER_CHECKNAMES;
		view->checknames = ISC_FALSE;
	} else if (strcasecmp(str, "ignore") == 0) {
		view->checknames = ISC_FALSE;
	} else
		INSIST(0);

	obj = NULL;
	result = named_config_get(maps, "zero-no-soa-ttl-cache", &obj);
	INSIST(result == ISC_R_SUCCESS);
	zero_no_soattl = cfg_obj_asboolean(obj);

	obj = NULL;
	result = named_config_get(maps, "dns64", &obj);
	if (result == ISC_R_SUCCESS && strcmp(view->name, "_bind") &&
	    strcmp(view->name, "_meta")) {
		isc_netaddr_t na, suffix, *sp;
		unsigned int prefixlen;
		const char *server, *contact;
		const cfg_obj_t *myobj;

		myobj = NULL;
		result = named_config_get(maps, "dns64-server", &myobj);
		if (result == ISC_R_SUCCESS)
			server = cfg_obj_asstring(myobj);
		else
			server = NULL;

		myobj = NULL;
		result = named_config_get(maps, "dns64-contact", &myobj);
		if (result == ISC_R_SUCCESS)
			contact = cfg_obj_asstring(myobj);
		else
			contact = NULL;

		for (element = cfg_list_first(obj);
		     element != NULL;
		     element = cfg_list_next(element))
		{
			const cfg_obj_t *map = cfg_listelt_value(element);
			dns_dns64_t *dns64 = NULL;
			unsigned int dns64options = 0;

			cfg_obj_asnetprefix(cfg_map_getname(map), &na,
					    &prefixlen);

			obj = NULL;
			(void)cfg_map_get(map, "suffix", &obj);
			if (obj != NULL) {
				sp = &suffix;
				isc_netaddr_fromsockaddr(sp,
						      cfg_obj_assockaddr(obj));
			} else
				sp = NULL;

			clients = mapped = excluded = NULL;
			obj = NULL;
			(void)cfg_map_get(map, "clients", &obj);
			if (obj != NULL) {
				result = cfg_acl_fromconfig(obj, config,
							    named_g_lctx, actx,
							    mctx, 0, &clients);
				if (result != ISC_R_SUCCESS)
					goto cleanup;
			}
			obj = NULL;
			(void)cfg_map_get(map, "mapped", &obj);
			if (obj != NULL) {
				result = cfg_acl_fromconfig(obj, config,
							    named_g_lctx, actx,
							    mctx, 0, &mapped);
				if (result != ISC_R_SUCCESS)
					goto cleanup;
			}
			obj = NULL;
			(void)cfg_map_get(map, "exclude", &obj);
			if (obj != NULL) {
				result = cfg_acl_fromconfig(obj, config,
							    named_g_lctx, actx,
							    mctx, 0, &excluded);
				if (result != ISC_R_SUCCESS)
					goto cleanup;
			} else {
				if (named_g_mapped == NULL) {
					result = create_mapped_acl();
					if (result != ISC_R_SUCCESS)
						goto cleanup;
				}
				dns_acl_attach(named_g_mapped, &excluded);
			}

			obj = NULL;
			(void)cfg_map_get(map, "recursive-only", &obj);
			if (obj != NULL && cfg_obj_asboolean(obj))
				dns64options |= DNS_DNS64_RECURSIVE_ONLY;

			obj = NULL;
			(void)cfg_map_get(map, "break-dnssec", &obj);
			if (obj != NULL && cfg_obj_asboolean(obj))
				dns64options |= DNS_DNS64_BREAK_DNSSEC;

			result = dns_dns64_create(mctx, &na, prefixlen, sp,
						  clients, mapped, excluded,
						  dns64options, &dns64);
			if (result != ISC_R_SUCCESS)
				goto cleanup;
			dns_dns64_append(&view->dns64, dns64);
			view->dns64cnt++;
			result = dns64_reverse(view, mctx, &na, prefixlen,
					       server, contact);
			if (result != ISC_R_SUCCESS)
				goto cleanup;
			if (clients != NULL)
				dns_acl_detach(&clients);
			if (mapped != NULL)
				dns_acl_detach(&mapped);
			if (excluded != NULL)
				dns_acl_detach(&excluded);
		}
	}

	obj = NULL;
	result = named_config_get(maps, "dnssec-accept-expired", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->acceptexpired = cfg_obj_asboolean(obj);

	obj = NULL;
	result = named_config_get(maps, "dnssec-validation", &obj);
	INSIST(result == ISC_R_SUCCESS);
	if (cfg_obj_isboolean(obj)) {
		view->enablevalidation = cfg_obj_asboolean(obj);
	} else {
		/* If dnssec-validation is not boolean, it must be "auto" */
		view->enablevalidation = ISC_TRUE;
		auto_root = ISC_TRUE;
	}

	obj = NULL;
	result = named_config_get(maps, "max-cache-ttl", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->maxcachettl = cfg_obj_asuint32(obj);

	obj = NULL;
	result = named_config_get(maps, "max-ncache-ttl", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->maxncachettl = cfg_obj_asuint32(obj);
	if (view->maxncachettl > 7 * 24 * 3600)
		view->maxncachettl = 7 * 24 * 3600;

	obj = NULL;
	result = named_config_get(maps, "synth-from-dnssec", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->synthfromdnssec = cfg_obj_asboolean(obj);

	obj = NULL;
	result = named_config_get(maps, "max-stale-ttl", &obj);
	INSIST(result == ISC_R_SUCCESS);
	max_stale_ttl = ISC_MAX(cfg_obj_asuint32(obj), 1);

	obj = NULL;
	result = named_config_get(maps, "stale-answer-enable", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->staleanswersenable = cfg_obj_asboolean(obj);

	result = dns_viewlist_find(&named_g_server->viewlist, view->name,
				   view->rdclass, &pview);
	if (result == ISC_R_SUCCESS) {
		view->staleanswersok = pview->staleanswersok;
		dns_view_detach(&pview);
	} else
		view->staleanswersok = dns_stale_answer_conf;

	/*
	 * Configure the view's cache.
	 *
	 * First, check to see if there are any attach-cache options.  If yes,
	 * attempt to lookup an existing cache at attach it to the view.  If
	 * there is not one, then try to reuse an existing cache if possible;
	 * otherwise create a new cache.
	 *
	 * Note that the ADB is not preserved or shared in either case.
	 *
	 * When a matching view is found, the associated statistics are also
	 * retrieved and reused.
	 *
	 * XXX Determining when it is safe to reuse or share a cache is tricky.
	 * When the view's configuration changes, the cached data may become
	 * invalid because it reflects our old view of the world.  We check
	 * some of the configuration parameters that could invalidate the cache
	 * or otherwise make it unsharable, but there are other configuration
	 * options that should be checked.  For example, if a view uses a
	 * forwarder, changes in the forwarder configuration may invalidate
	 * the cache.  At the moment, it's the administrator's responsibility to
	 * ensure these configuration options don't invalidate reusing/sharing.
	 */
	obj = NULL;
	result = named_config_get(maps, "attach-cache", &obj);
	if (result == ISC_R_SUCCESS)
		cachename = cfg_obj_asstring(obj);
	else
		cachename = view->name;
	cache = NULL;
	nsc = cachelist_find(cachelist, cachename, view->rdclass);
	if (nsc != NULL) {
		if (!cache_sharable(nsc->primaryview, view, zero_no_soattl,
				    cleaning_interval, max_cache_size,
				    max_stale_ttl))
		{
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "views %s and %s can't share the cache "
				      "due to configuration parameter mismatch",
				      nsc->primaryview->name, view->name);
			result = ISC_R_FAILURE;
			goto cleanup;
		}
		dns_cache_attach(nsc->cache, &cache);
		shared_cache = ISC_TRUE;
	} else {
		if (strcmp(cachename, view->name) == 0) {
			result = dns_viewlist_find(&named_g_server->viewlist,
						   cachename, view->rdclass,
						   &pview);
			if (result != ISC_R_NOTFOUND && result != ISC_R_SUCCESS)
				goto cleanup;
			if (pview != NULL) {
				if (!cache_reusable(pview, view,
						    zero_no_soattl)) {
					isc_log_write(named_g_lctx,
						      NAMED_LOGCATEGORY_GENERAL,
						      NAMED_LOGMODULE_SERVER,
						      ISC_LOG_DEBUG(1),
						      "cache cannot be reused "
						      "for view %s due to "
						      "configuration parameter "
						      "mismatch", view->name);
				} else {
					INSIST(pview->cache != NULL);
					isc_log_write(named_g_lctx,
						      NAMED_LOGCATEGORY_GENERAL,
						      NAMED_LOGMODULE_SERVER,
						      ISC_LOG_DEBUG(3),
						      "reusing existing cache");
					reused_cache = ISC_TRUE;
					dns_cache_attach(pview->cache, &cache);
				}
				dns_view_getresstats(pview, &resstats);
				dns_view_getresquerystats(pview,
							  &resquerystats);
				dns_view_detach(&pview);
			}
		}
		if (cache == NULL) {
			/*
			 * Create a cache with the desired name.  This normally
			 * equals the view name, but may also be a forward
			 * reference to a view that share the cache with this
			 * view but is not yet configured.  If it is not the
			 * view name but not a forward reference either, then it
			 * is simply a named cache that is not shared.
			 *
			 * We use two separate memory contexts for the
			 * cache, for the main cache memory and the heap
			 * memory.
			 */
			CHECK(isc_mem_create(0, 0, &cmctx));
			isc_mem_setname(cmctx, "cache", NULL);
			CHECK(isc_mem_create(0, 0, &hmctx));
			isc_mem_setname(hmctx, "cache_heap", NULL);
			CHECK(dns_cache_create3(cmctx, hmctx, named_g_taskmgr,
						named_g_timermgr, view->rdclass,
						cachename, "rbt", 0, NULL,
						&cache));
			isc_mem_detach(&cmctx);
			isc_mem_detach(&hmctx);
		}
		nsc = isc_mem_get(mctx, sizeof(*nsc));
		if (nsc == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}
		nsc->cache = NULL;
		dns_cache_attach(cache, &nsc->cache);
		nsc->primaryview = view;
		nsc->needflush = ISC_FALSE;
		nsc->adbsizeadjusted = ISC_FALSE;
		nsc->rdclass = view->rdclass;
		ISC_LINK_INIT(nsc, link);
		ISC_LIST_APPEND(*cachelist, nsc, link);
	}
	dns_view_setcache2(view, cache, shared_cache);

	/*
	 * cache-file cannot be inherited if views are present, but this
	 * should be caught by the configuration checking stage.
	 */
	obj = NULL;
	result = named_config_get(maps, "cache-file", &obj);
	if (result == ISC_R_SUCCESS && strcmp(view->name, "_bind") != 0) {
		CHECK(dns_cache_setfilename(cache, cfg_obj_asstring(obj)));
		if (!reused_cache && !shared_cache)
			CHECK(dns_cache_load(cache));
	}

	dns_cache_setcleaninginterval(cache, cleaning_interval);
	dns_cache_setcachesize(cache, max_cache_size);
	dns_cache_setservestalettl(cache, max_stale_ttl);

	dns_cache_detach(&cache);

	obj = NULL;
	result = named_config_get(maps, "stale-answer-ttl", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->staleanswerttl = ISC_MAX(cfg_obj_asuint32(obj), 1);

	/*
	 * Resolver.
	 *
	 * XXXRTH  Hardwired number of tasks.
	 */
	CHECK(get_view_querysource_dispatch(maps, AF_INET, &dispatch4, &dscp4,
					    ISC_TF(ISC_LIST_PREV(view, link)
						   == NULL)));
	CHECK(get_view_querysource_dispatch(maps, AF_INET6, &dispatch6, &dscp6,
					    ISC_TF(ISC_LIST_PREV(view, link)
						   == NULL)));
	if (dispatch4 == NULL && dispatch6 == NULL) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "unable to obtain neither an IPv4 nor"
				 " an IPv6 dispatch");
		result = ISC_R_UNEXPECTED;
		goto cleanup;
	}

	if (resstats == NULL) {
		CHECK(isc_stats_create(mctx, &resstats,
				       dns_resstatscounter_max));
	}
	dns_view_setresstats(view, resstats);
	if (resquerystats == NULL)
		CHECK(dns_rdatatypestats_create(mctx, &resquerystats));
	dns_view_setresquerystats(view, resquerystats);

	ndisp = 4 * ISC_MIN(named_g_udpdisp, MAX_UDP_DISPATCH);
	CHECK(dns_view_createresolver(view, named_g_taskmgr, RESOLVER_NTASKS,
				      ndisp, named_g_socketmgr,
				      named_g_timermgr, resopts,
				      named_g_dispatchmgr,
				      dispatch4, dispatch6));

	if (dscp4 == -1)
		dscp4 = named_g_dscp;
	if (dscp6 == -1)
		dscp6 = named_g_dscp;
	if (dscp4 != -1)
		dns_resolver_setquerydscp4(view->resolver, dscp4);
	if (dscp6 != -1)
		dns_resolver_setquerydscp6(view->resolver, dscp6);

	/*
	 * Set the ADB cache size to 1/8th of the max-cache-size or
	 * MAX_ADB_SIZE_FOR_CACHESHARE when the cache is shared.
	 */
	max_adb_size = 0;
	if (max_cache_size != 0U) {
		max_adb_size = max_cache_size / 8;
		if (max_adb_size == 0U)
			max_adb_size = 1;	/* Force minimum. */
		if (view != nsc->primaryview &&
		    max_adb_size > MAX_ADB_SIZE_FOR_CACHESHARE) {
			max_adb_size = MAX_ADB_SIZE_FOR_CACHESHARE;
			if (!nsc->adbsizeadjusted) {
				dns_adb_setadbsize(nsc->primaryview->adb,
						   MAX_ADB_SIZE_FOR_CACHESHARE);
				nsc->adbsizeadjusted = ISC_TRUE;
			}
		}
	}
	dns_adb_setadbsize(view->adb, max_adb_size);

	/*
	 * Set up ADB quotas
	 */
	{
		isc_uint32_t fps, freq;
		double low, high, discount;

		obj = NULL;
		result = named_config_get(maps, "fetches-per-server", &obj);
		INSIST(result == ISC_R_SUCCESS);
		obj2 = cfg_tuple_get(obj, "fetches");
		fps = cfg_obj_asuint32(obj2);
		obj2 = cfg_tuple_get(obj, "response");
		if (!cfg_obj_isvoid(obj2)) {
			const char *resp = cfg_obj_asstring(obj2);
			isc_result_t r;

			if (strcasecmp(resp, "drop") == 0)
				r = DNS_R_DROP;
			else if (strcasecmp(resp, "fail") == 0)
				r = DNS_R_SERVFAIL;
			else
				INSIST(0);

			dns_resolver_setquotaresponse(view->resolver,
						      dns_quotatype_server, r);
		}

		obj = NULL;
		result = named_config_get(maps, "fetch-quota-params", &obj);
		INSIST(result == ISC_R_SUCCESS);

		obj2 = cfg_tuple_get(obj, "frequency");
		freq = cfg_obj_asuint32(obj2);

		obj2 = cfg_tuple_get(obj, "low");
		low = (double) cfg_obj_asfixedpoint(obj2) / 100.0;

		obj2 = cfg_tuple_get(obj, "high");
		high = (double) cfg_obj_asfixedpoint(obj2) / 100.0;

		obj2 = cfg_tuple_get(obj, "discount");
		discount = (double) cfg_obj_asfixedpoint(obj2) / 100.0;

		dns_adb_setquota(view->adb, fps, freq, low, high, discount);
	}

	/*
	 * Set resolver's lame-ttl.
	 */
	obj = NULL;
	result = named_config_get(maps, "lame-ttl", &obj);
	INSIST(result == ISC_R_SUCCESS);
	lame_ttl = cfg_obj_asuint32(obj);
	if (lame_ttl > 1800)
		lame_ttl = 1800;
	dns_resolver_setlamettl(view->resolver, lame_ttl);

	/*
	 * Set the resolver's query timeout.
	 */
	obj = NULL;
	result = named_config_get(maps, "resolver-query-timeout", &obj);
	INSIST(result == ISC_R_SUCCESS);
	query_timeout = cfg_obj_asuint32(obj);
	dns_resolver_settimeout(view->resolver, query_timeout);

	/* Specify whether to use 0-TTL for negative response for SOA query */
	dns_resolver_setzeronosoattl(view->resolver, zero_no_soattl);

	/*
	 * Set the resolver's EDNS UDP size.
	 */
	obj = NULL;
	result = named_config_get(maps, "edns-udp-size", &obj);
	INSIST(result == ISC_R_SUCCESS);
	udpsize = cfg_obj_asuint32(obj);
	if (udpsize < 512)
		udpsize = 512;
	if (udpsize > 4096)
		udpsize = 4096;
	dns_resolver_setudpsize(view->resolver, (isc_uint16_t)udpsize);

	/*
	 * Set the maximum UDP response size.
	 */
	obj = NULL;
	result = named_config_get(maps, "max-udp-size", &obj);
	INSIST(result == ISC_R_SUCCESS);
	udpsize = cfg_obj_asuint32(obj);
	if (udpsize < 512)
		udpsize = 512;
	if (udpsize > 4096)
		udpsize = 4096;
	view->maxudp = udpsize;

	/*
	 * Set the maximum UDP when a COOKIE is not provided.
	 */
	obj = NULL;
	result = named_config_get(maps, "nocookie-udp-size", &obj);
	INSIST(result == ISC_R_SUCCESS);
	udpsize = cfg_obj_asuint32(obj);
	if (udpsize < 128)
		udpsize = 128;
	if (udpsize > view->maxudp)
		udpsize = view->maxudp;
	view->nocookieudp = udpsize;

	/*
	 * Set the maximum rsa exponent bits.
	 */
	obj = NULL;
	result = named_config_get(maps, "max-rsa-exponent-size", &obj);
	INSIST(result == ISC_R_SUCCESS);
	maxbits = cfg_obj_asuint32(obj);
	if (maxbits != 0 && maxbits < 35)
		maxbits = 35;
	if (maxbits > 4096)
		maxbits = 4096;
	view->maxbits = maxbits;

	/*
	 * Set resolver retry parameters.
	 */
	obj = NULL;
	CHECK(named_config_get(maps, "resolver-retry-interval", &obj));
	resolver_param = cfg_obj_asuint32(obj);
	if (resolver_param > 0)
		dns_resolver_setretryinterval(view->resolver, resolver_param);

	obj = NULL;
	CHECK(named_config_get(maps, "resolver-nonbackoff-tries", &obj));
	resolver_param = cfg_obj_asuint32(obj);
	if (resolver_param > 0)
		dns_resolver_setnonbackofftries(view->resolver, resolver_param);

	/*
	 * Set supported DNSSEC algorithms.
	 */
	dns_resolver_reset_algorithms(view->resolver);
	disabled = NULL;
	(void)named_config_get(maps, "disable-algorithms", &disabled);
	if (disabled != NULL) {
		for (element = cfg_list_first(disabled);
		     element != NULL;
		     element = cfg_list_next(element))
			CHECK(disable_algorithms(cfg_listelt_value(element),
						 view->resolver));
	}

	/*
	 * Set supported DS/DLV digest types.
	 */
	dns_resolver_reset_ds_digests(view->resolver);
	disabled = NULL;
	(void)named_config_get(maps, "disable-ds-digests", &disabled);
	if (disabled != NULL) {
		for (element = cfg_list_first(disabled);
		     element != NULL;
		     element = cfg_list_next(element))
			CHECK(disable_ds_digests(cfg_listelt_value(element),
						 view->resolver));
	}

	/*
	 * A global or view "forwarders" option, if present,
	 * creates an entry for "." in the forwarding table.
	 */
	forwardtype = NULL;
	forwarders = NULL;
	(void)named_config_get(maps, "forward", &forwardtype);
	(void)named_config_get(maps, "forwarders", &forwarders);
	if (forwarders != NULL)
		CHECK(configure_forward(config, view, dns_rootname,
					forwarders, forwardtype));

	/*
	 * Dual Stack Servers.
	 */
	alternates = NULL;
	(void)named_config_get(maps, "dual-stack-servers", &alternates);
	if (alternates != NULL)
		CHECK(configure_alternates(config, view, alternates));

	/*
	 * We have default hints for class IN if we need them.
	 */
	if (view->rdclass == dns_rdataclass_in && view->hints == NULL)
		dns_view_sethints(view, named_g_server->in_roothints);

	/*
	 * If we still have no hints, this is a non-IN view with no
	 * "hints zone" configured.  Issue a warning, except if this
	 * is a root server.  Root servers never need to consult
	 * their hints, so it's no point requiring users to configure
	 * them.
	 */
	if (view->hints == NULL) {
		dns_zone_t *rootzone = NULL;
		(void)dns_view_findzone(view, dns_rootname, &rootzone);
		if (rootzone != NULL) {
			dns_zone_detach(&rootzone);
			need_hints = ISC_FALSE;
		}
		if (need_hints)
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
				      "no root hints for view '%s'",
				      view->name);
	}

	/*
	 * Configure the view's TSIG keys.
	 */
	CHECK(named_tsigkeyring_fromconfig(config, vconfig,
					   view->mctx, &ring));
	if (named_g_server->sessionkey != NULL) {
		CHECK(dns_tsigkeyring_add(ring,
					  named_g_server->session_keyname,
					  named_g_server->sessionkey));
	}
	dns_view_setkeyring(view, ring);
	dns_tsigkeyring_detach(&ring);

	/*
	 * See if we can re-use a dynamic key ring.
	 */
	result = dns_viewlist_find(&named_g_server->viewlist, view->name,
				   view->rdclass, &pview);
	if (result != ISC_R_NOTFOUND && result != ISC_R_SUCCESS)
		goto cleanup;
	if (pview != NULL) {
		dns_view_getdynamickeyring(pview, &ring);
		if (ring != NULL)
			dns_view_setdynamickeyring(view, ring);
		dns_tsigkeyring_detach(&ring);
		dns_view_detach(&pview);
	} else
		dns_view_restorekeyring(view);

	/*
	 * Configure the view's peer list.
	 */
	{
		const cfg_obj_t *peers = NULL;
		dns_peerlist_t *newpeers = NULL;

		(void)named_config_get(cfgmaps, "server", &peers);
		CHECK(dns_peerlist_new(mctx, &newpeers));
		for (element = cfg_list_first(peers);
		     element != NULL;
		     element = cfg_list_next(element))
		{
			const cfg_obj_t *cpeer = cfg_listelt_value(element);
			dns_peer_t *peer;

			CHECK(configure_peer(cpeer, mctx, &peer));
			dns_peerlist_addpeer(newpeers, peer);
			dns_peer_detach(&peer);
		}
		dns_peerlist_detach(&view->peers);
		view->peers = newpeers; /* Transfer ownership. */
	}

	/*
	 *	Configure the views rrset-order.
	 */
	{
		const cfg_obj_t *rrsetorder = NULL;

		(void)named_config_get(maps, "rrset-order", &rrsetorder);
		CHECK(dns_order_create(mctx, &order));
		for (element = cfg_list_first(rrsetorder);
		     element != NULL;
		     element = cfg_list_next(element))
		{
			const cfg_obj_t *ent = cfg_listelt_value(element);

			CHECK(configure_order(order, ent));
		}
		if (view->order != NULL)
			dns_order_detach(&view->order);
		dns_order_attach(order, &view->order);
		dns_order_detach(&order);
	}
	/*
	 * Copy the aclenv object.
	 */
	dns_aclenv_copy(&view->aclenv,
		ns_interfacemgr_getaclenv(named_g_server->interfacemgr));

	/*
	 * Configure the "match-clients" and "match-destinations" ACL.
	 * (These are only meaningful at the view level, but 'config'
	 * must be passed so that named ACLs defined at the global level
	 * can be retrieved.)
	 */
	CHECK(configure_view_acl(vconfig, config, NULL, "match-clients",
				 NULL, actx, named_g_mctx,
				 &view->matchclients));
	CHECK(configure_view_acl(vconfig, config, NULL, "match-destinations",
				 NULL, actx, named_g_mctx,
				 &view->matchdestinations));

	/*
	 * Configure the "match-recursive-only" option.
	 */
	obj = NULL;
	(void)named_config_get(maps, "match-recursive-only", &obj);
	if (obj != NULL && cfg_obj_asboolean(obj))
		view->matchrecursiveonly = ISC_TRUE;
	else
		view->matchrecursiveonly = ISC_FALSE;

	/*
	 * Configure other configurable data.
	 */
	obj = NULL;
	result = named_config_get(maps, "recursion", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->recursion = cfg_obj_asboolean(obj);

	obj = NULL;
	result = named_config_get(maps, "auth-nxdomain", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->auth_nxdomain = cfg_obj_asboolean(obj);

	obj = NULL;
	result = named_config_get(maps, "glue-cache", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->use_glue_cache = cfg_obj_asboolean(obj);

	obj = NULL;
	result = named_config_get(maps, "minimal-any", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->minimal_any = cfg_obj_asboolean(obj);

	obj = NULL;
	result = named_config_get(maps, "minimal-responses", &obj);
	INSIST(result == ISC_R_SUCCESS);
	if (cfg_obj_isboolean(obj)) {
		if (cfg_obj_asboolean(obj))
			view->minimalresponses = dns_minimal_yes;
		else
			view->minimalresponses = dns_minimal_no;
	} else {
		str = cfg_obj_asstring(obj);
		if (strcasecmp(str, "no-auth") == 0) {
			view->minimalresponses = dns_minimal_noauth;
		} else if (strcasecmp(str, "no-auth-recursive") == 0) {
			view->minimalresponses = dns_minimal_noauthrec;
		} else
			INSIST(0);
	}

	obj = NULL;
	result = named_config_get(maps, "transfer-format", &obj);
	INSIST(result == ISC_R_SUCCESS);
	str = cfg_obj_asstring(obj);
	if (strcasecmp(str, "many-answers") == 0)
		view->transfer_format = dns_many_answers;
	else if (strcasecmp(str, "one-answer") == 0)
		view->transfer_format = dns_one_answer;
	else
		INSIST(0);

	obj = NULL;
	result = named_config_get(maps, "trust-anchor-telemetry", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->trust_anchor_telemetry = cfg_obj_asboolean(obj);

	obj = NULL;
	result = named_config_get(maps, "root-key-sentinel", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->root_key_sentinel = cfg_obj_asboolean(obj);

	CHECK(configure_view_acl(vconfig, config, named_g_config,
				 "allow-query-cache-on", NULL, actx,
				 named_g_mctx, &view->cacheonacl));
	/*
	 * Set the "allow-query", "allow-query-cache", "allow-recursion",
	 * and "allow-recursion-on" ACLs if configured in named.conf, but
	 * NOT from the global defaults. This is done by leaving the third
	 * argument to configure_view_acl() NULL.
	 *
	 * We ignore the global defaults here because these ACLs
	 * can inherit from each other.  If any are still unset after
	 * applying the inheritance rules, we'll look up the defaults at
	 * that time.
	 */

	/* named.conf only */
	CHECK(configure_view_acl(vconfig, config, NULL,
				 "allow-query", NULL, actx,
				 named_g_mctx, &view->queryacl));

	/* named.conf only */
	CHECK(configure_view_acl(vconfig, config, NULL,
				 "allow-query-cache", NULL, actx,
				 named_g_mctx, &view->cacheacl));

	if (strcmp(view->name, "_bind") != 0 &&
	    view->rdclass != dns_rdataclass_chaos)
	{
		/* named.conf only */
		CHECK(configure_view_acl(vconfig, config, NULL,
					 "allow-recursion", NULL, actx,
					 named_g_mctx, &view->recursionacl));
		/* named.conf only */
		CHECK(configure_view_acl(vconfig, config, NULL,
					 "allow-recursion-on", NULL, actx,
					 named_g_mctx, &view->recursiononacl));
	}

	if (view->recursion) {
		/*
		 * "allow-query-cache" inherits from "allow-recursion" if set,
		 * otherwise from "allow-query" if set.
		 * "allow-recursion" inherits from "allow-query-cache" if set,
		 * otherwise from "allow-query" if set.
		 */
		if (view->cacheacl == NULL) {
			if (view->recursionacl != NULL) {
				dns_acl_attach(view->recursionacl,
					       &view->cacheacl);
			} else if (view->queryacl != NULL) {
				dns_acl_attach(view->queryacl,
					       &view->cacheacl);
			}
		}
		if (view->recursionacl == NULL) {
			if (view->cacheacl != NULL) {
				dns_acl_attach(view->cacheacl,
					       &view->recursionacl);
			} else if (view->queryacl != NULL) {
				dns_acl_attach(view->queryacl,
					       &view->recursionacl);
			}
		}

		/*
		 * If any are still unset, we now get default "allow-recursion",
		 * "allow-recursion-on" and "allow-query-cache" ACLs from
		 * the global config.
		 */
		if (view->recursionacl == NULL) {
			/* global default only */
			CHECK(configure_view_acl(NULL, NULL, named_g_config,
						 "allow-recursion", NULL,
						 actx, named_g_mctx,
						 &view->recursionacl));
		}
		if (view->recursiononacl == NULL) {
			/* global default only */
			CHECK(configure_view_acl(NULL, NULL, named_g_config,
						 "allow-recursion-on", NULL,
						 actx, named_g_mctx,
						 &view->recursiononacl));
		}
		if (view->cacheacl == NULL) {
			/* global default only */
			CHECK(configure_view_acl(NULL, NULL, named_g_config,
						 "allow-query-cache", NULL,
						 actx, named_g_mctx,
						 &view->cacheacl));
		}
	} else if (view->cacheacl == NULL) {
		/*
		 * We're not recursive; if "allow-query-cache" hasn't been
		 * set at the options/view level, set it to none.
		 */
		CHECK(dns_acl_none(mctx, &view->cacheacl));
	}

	if (view->queryacl == NULL) {
		/* global default only */
		CHECK(configure_view_acl(NULL, NULL, named_g_config,
					 "allow-query", NULL,
					 actx, named_g_mctx,
					 &view->queryacl));
	}

	/*
	 * Ignore case when compressing responses to the specified
	 * clients. This causes case not always to be preserved,
	 * and is needed by some broken clients.
	 */
	CHECK(configure_view_acl(vconfig, config, named_g_config,
				 "no-case-compress", NULL, actx,
				 named_g_mctx, &view->nocasecompress));

	/*
	 * Disable name compression completely, this is a tradeoff
	 * between CPU and network usage.
	 */
	obj = NULL;
	result = named_config_get(maps, "message-compression", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->msgcompression = cfg_obj_asboolean(obj);

	/*
	 * Filter setting on addresses in the answer section.
	 */
	CHECK(configure_view_acl(vconfig, config, named_g_config,
				 "deny-answer-addresses", "acl",
				 actx, named_g_mctx,
				 &view->denyansweracl));
	CHECK(configure_view_nametable(vconfig, config, "deny-answer-addresses",
				       "except-from", named_g_mctx,
				       &view->answeracl_exclude));

	/*
	 * Filter setting on names (CNAME/DNAME targets) in the answer section.
	 */
	CHECK(configure_view_nametable(vconfig, config, "deny-answer-aliases",
				       "name", named_g_mctx,
				       &view->denyanswernames));
	CHECK(configure_view_nametable(vconfig, config, "deny-answer-aliases",
				       "except-from", named_g_mctx,
				       &view->answernames_exclude));

	/*
	 * Configure sortlist, if set
	 */
	CHECK(configure_view_sortlist(vconfig, config, actx, named_g_mctx,
				      &view->sortlist));

	/*
	 * Configure default allow-notify, allow-update
	 * and allow-update-forwarding ACLs, so they can be
	 * inherited by zones. (Note these cannot be set at
	 * options/view level.)
	 */
	if (view->notifyacl == NULL) {
		CHECK(configure_view_acl(vconfig, config, named_g_config,
					 "allow-notify", NULL, actx,
					 named_g_mctx, &view->notifyacl));
	}
	if (view->updateacl == NULL) {
		CHECK(configure_view_acl(NULL, NULL, named_g_config,
					 "allow-update", NULL, actx,
					 named_g_mctx, &view->updateacl));
	}
	if (view->upfwdacl == NULL) {
		CHECK(configure_view_acl(NULL, NULL, named_g_config,
					 "allow-update-forwarding", NULL, actx,
					 named_g_mctx, &view->upfwdacl));
	}

	/*
	 * Configure default allow-transer ACL so it can be inherited
	 * by zones. (Note this *can* be set at options or view level.)
	 */
	if (view->transferacl == NULL) {
		CHECK(configure_view_acl(vconfig, config, named_g_config,
					 "allow-transfer", NULL, actx,
					 named_g_mctx, &view->transferacl));
	}

	obj = NULL;
	result = named_config_get(maps, "provide-ixfr", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->provideixfr = cfg_obj_asboolean(obj);

	obj = NULL;
	result = named_config_get(maps, "request-nsid", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->requestnsid = cfg_obj_asboolean(obj);

	obj = NULL;
	result = named_config_get(maps, "send-cookie", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->sendcookie = cfg_obj_asboolean(obj);

	obj = NULL;
	if (view->pad_acl != NULL)
		dns_acl_detach(&view->pad_acl);
	result = named_config_get(optionmaps, "response-padding", &obj);
	if (result == ISC_R_SUCCESS) {
		const cfg_obj_t *padobj = cfg_tuple_get(obj, "block-size");
		const cfg_obj_t *aclobj = cfg_tuple_get(obj, "acl");
		isc_uint32_t padding = cfg_obj_asuint32(padobj);

		if (padding > 512U) {
			cfg_obj_log(obj, named_g_lctx, ISC_LOG_WARNING,
				    "response-padding block-size cannot "
				    "exceed 512: lowering");
			padding = 512U;
		}
		view->padding = (isc_uint16_t)padding;
		CHECK(cfg_acl_fromconfig(aclobj, config, named_g_lctx,
					 actx, named_g_mctx, 0,
					 &view->pad_acl));
	}

	obj = NULL;
	result = named_config_get(maps, "require-server-cookie", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->requireservercookie = cfg_obj_asboolean(obj);

	obj = NULL;
	result = named_config_get(maps, "v6-bias", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->v6bias = cfg_obj_asuint32(obj) * 1000;

	obj = NULL;
	result = named_config_get(maps, "max-clients-per-query", &obj);
	INSIST(result == ISC_R_SUCCESS);
	max_clients_per_query = cfg_obj_asuint32(obj);

	obj = NULL;
	result = named_config_get(maps, "clients-per-query", &obj);
	INSIST(result == ISC_R_SUCCESS);
	dns_resolver_setclientsperquery(view->resolver,
					cfg_obj_asuint32(obj),
					max_clients_per_query);

	obj = NULL;
	result = named_config_get(maps, "max-recursion-depth", &obj);
	INSIST(result == ISC_R_SUCCESS);
	dns_resolver_setmaxdepth(view->resolver, cfg_obj_asuint32(obj));

	obj = NULL;
	result = named_config_get(maps, "max-recursion-queries", &obj);
	INSIST(result == ISC_R_SUCCESS);
	dns_resolver_setmaxqueries(view->resolver, cfg_obj_asuint32(obj));

	obj = NULL;
	result = named_config_get(maps, "fetches-per-zone", &obj);
	INSIST(result == ISC_R_SUCCESS);
	obj2 = cfg_tuple_get(obj, "fetches");
	dns_resolver_setfetchesperzone(view->resolver, cfg_obj_asuint32(obj2));
	obj2 = cfg_tuple_get(obj, "response");
	if (!cfg_obj_isvoid(obj2)) {
		const char *resp = cfg_obj_asstring(obj2);
		isc_result_t r;

		if (strcasecmp(resp, "drop") == 0)
			r = DNS_R_DROP;
		else if (strcasecmp(resp, "fail") == 0)
			r = DNS_R_SERVFAIL;
		else
			INSIST(0);

		dns_resolver_setquotaresponse(view->resolver,
					      dns_quotatype_zone, r);
	}

	obj = NULL;
	result = named_config_get(maps, "filter-aaaa-on-v4", &obj);
	INSIST(result == ISC_R_SUCCESS);
	if (cfg_obj_isboolean(obj)) {
		if (cfg_obj_asboolean(obj))
			view->v4_aaaa = dns_aaaa_filter;
		else
			view->v4_aaaa = dns_aaaa_ok;
	} else {
		const char *v4_aaaastr = cfg_obj_asstring(obj);
		if (strcasecmp(v4_aaaastr, "break-dnssec") == 0)
			view->v4_aaaa = dns_aaaa_break_dnssec;
		else
			INSIST(0);
	}

	obj = NULL;
	result = named_config_get(maps, "filter-aaaa-on-v6", &obj);
	INSIST(result == ISC_R_SUCCESS);
	if (cfg_obj_isboolean(obj)) {
		if (cfg_obj_asboolean(obj))
			view->v6_aaaa = dns_aaaa_filter;
		else
			view->v6_aaaa = dns_aaaa_ok;
	} else {
		const char *v6_aaaastr = cfg_obj_asstring(obj);
		if (strcasecmp(v6_aaaastr, "break-dnssec") == 0)
			view->v6_aaaa = dns_aaaa_break_dnssec;
		else
			INSIST(0);
	}

	CHECK(configure_view_acl(vconfig, config, named_g_config,
				 "filter-aaaa", NULL, actx,
				 named_g_mctx, &view->aaaa_acl));

	obj = NULL;
	result = named_config_get(maps, "prefetch", &obj);
	if (result == ISC_R_SUCCESS) {
		const cfg_obj_t *trigger, *eligible;

		trigger = cfg_tuple_get(obj, "trigger");
		view->prefetch_trigger = cfg_obj_asuint32(trigger);
		if (view->prefetch_trigger > 10)
			view->prefetch_trigger = 10;
		eligible = cfg_tuple_get(obj, "eligible");
		if (cfg_obj_isvoid(eligible)) {
			int m;
			for (m = 1; maps[m] != NULL; m++) {
				obj = NULL;
				result = named_config_get(&maps[m],
						       "prefetch", &obj);
				INSIST(result == ISC_R_SUCCESS);
				eligible = cfg_tuple_get(obj, "eligible");
				if (cfg_obj_isuint32(eligible))
					break;
			}
			INSIST(cfg_obj_isuint32(eligible));
		}
		view->prefetch_eligible = cfg_obj_asuint32(eligible);
		if (view->prefetch_eligible < view->prefetch_trigger + 6)
			view->prefetch_eligible = view->prefetch_trigger + 6;
	}

	obj = NULL;
	result = named_config_get(maps, "dnssec-enable", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->enablednssec = cfg_obj_asboolean(obj);

	obj = NULL;
	result = named_config_get(optionmaps, "dnssec-lookaside", &obj);
	if (result == ISC_R_SUCCESS) {
		/* "auto" is deprecated, log a warning if seen */
		const char *dom;
		dlvobj = cfg_listelt_value(cfg_list_first(obj));
		dom = cfg_obj_asstring(cfg_tuple_get(dlvobj, "domain"));
		if (cfg_obj_isvoid(cfg_tuple_get(dlvobj, "trust-anchor"))) {
			/* If "no", skip; if "auto", log warning */
			if (!strcasecmp(dom, "no")) {
				result = ISC_R_NOTFOUND;
			} else if (!strcasecmp(dom, "auto")) {
				/*
				 * Warning logged by libbind9.
				 */
				result = ISC_R_NOTFOUND;
			}
		}
	}

	if (result == ISC_R_SUCCESS) {
		dns_name_t *dlv, *iscdlv;
		dns_fixedname_t f;

		/* Also log a warning if manually configured to dlv.isc.org */
		iscdlv = dns_fixedname_initname(&f);
		CHECK(dns_name_fromstring(iscdlv, "dlv.isc.org", 0, NULL));

		for (element = cfg_list_first(obj);
		     element != NULL;
		     element = cfg_list_next(element))
		{
			obj = cfg_listelt_value(element);
			obj = cfg_tuple_get(obj, "trust-anchor");

			dlv = dns_fixedname_name(&view->dlv_fixed);
			CHECK(dns_name_fromstring(dlv, cfg_obj_asstring(obj),
						  DNS_NAME_DOWNCASE, NULL));
			if (dns_name_equal(dlv, iscdlv)) {
				/*
				 * Warning logged by libbind9.
				 */
				view->dlv = NULL;
			} else {
				view->dlv = dlv;
			}
		}
	} else {
		view->dlv = NULL;
	}

	/*
	 * For now, there is only one kind of trusted keys, the
	 * "security roots".
	 */
	CHECK(configure_view_dnsseckeys(view, vconfig, config, bindkeys,
					auto_root, mctx));
	dns_resolver_resetmustbesecure(view->resolver);
	obj = NULL;
	result = named_config_get(maps, "dnssec-must-be-secure", &obj);
	if (result == ISC_R_SUCCESS)
		CHECK(mustbesecure(obj, view->resolver));

	obj = NULL;
	result = named_config_get(maps, "nta-recheck", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->nta_recheck = cfg_obj_asuint32(obj);

	obj = NULL;
	result = named_config_get(maps, "nta-lifetime", &obj);
	INSIST(result == ISC_R_SUCCESS);
	view->nta_lifetime = cfg_obj_asuint32(obj);

	obj = NULL;
	result = named_config_get(maps, "preferred-glue", &obj);
	if (result == ISC_R_SUCCESS) {
		str = cfg_obj_asstring(obj);
		if (strcasecmp(str, "a") == 0)
			view->preferred_glue = dns_rdatatype_a;
		else if (strcasecmp(str, "aaaa") == 0)
			view->preferred_glue = dns_rdatatype_aaaa;
		else
			view->preferred_glue = 0;
	} else
		view->preferred_glue = 0;

	obj = NULL;
	result = named_config_get(maps, "root-delegation-only", &obj);
	if (result == ISC_R_SUCCESS)
		dns_view_setrootdelonly(view, ISC_TRUE);
	if (result == ISC_R_SUCCESS && ! cfg_obj_isvoid(obj)) {
		const cfg_obj_t *exclude;
		dns_fixedname_t fixed;
		dns_name_t *name;

		name = dns_fixedname_initname(&fixed);
		for (element = cfg_list_first(obj);
		     element != NULL;
		     element = cfg_list_next(element))
		{
			exclude = cfg_listelt_value(element);
			CHECK(dns_name_fromstring(name,
						  cfg_obj_asstring(exclude),
						  0, NULL));
			CHECK(dns_view_excludedelegationonly(view, name));
		}
	} else
		dns_view_setrootdelonly(view, ISC_FALSE);

	/*
	 * Load DynDB modules.
	 */
	dyndb_list = NULL;
	if (voptions != NULL)
		(void)cfg_map_get(voptions, "dyndb", &dyndb_list);
	else
		(void)cfg_map_get(config, "dyndb", &dyndb_list);

#ifdef HAVE_DLOPEN
	for (element = cfg_list_first(dyndb_list);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *dyndb = cfg_listelt_value(element);

		if (dctx == NULL) {
			const void *hashinit = isc_hash_get_initializer();
			CHECK(dns_dyndb_createctx(mctx, hashinit,
						  named_g_lctx, view,
						  named_g_server->zonemgr,
						  named_g_server->task,
						  named_g_timermgr, &dctx));
		}

		CHECK(configure_dyndb(dyndb, mctx, dctx));
	}
#endif

	/*
	 * Setup automatic empty zones.  If recursion is off then
	 * they are disabled by default.
	 */
	obj = NULL;
	(void)named_config_get(maps, "empty-zones-enable", &obj);
	(void)named_config_get(maps, "disable-empty-zone", &disablelist);
	if (obj == NULL && disablelist == NULL &&
	    view->rdclass == dns_rdataclass_in) {
		empty_zones_enable = view->recursion;
	} else if (view->rdclass == dns_rdataclass_in) {
		if (obj != NULL)
			empty_zones_enable = cfg_obj_asboolean(obj);
		else
			empty_zones_enable = view->recursion;
	} else {
		empty_zones_enable = ISC_FALSE;
	}

	if (empty_zones_enable) {
		const char *empty;
		int empty_zone = 0;
		dns_fixedname_t fixed;
		dns_name_t *name;
		isc_buffer_t buffer;
		char server[DNS_NAME_FORMATSIZE + 1];
		char contact[DNS_NAME_FORMATSIZE + 1];
		const char *empty_dbtype[4] =
				    { "_builtin", "empty", NULL, NULL };
		int empty_dbtypec = 4;
		dns_zonestat_level_t statlevel;

		name = dns_fixedname_initname(&fixed);

		obj = NULL;
		result = named_config_get(maps, "empty-server", &obj);
		if (result == ISC_R_SUCCESS) {
			CHECK(dns_name_fromstring(name, cfg_obj_asstring(obj),
						  0, NULL));
			isc_buffer_init(&buffer, server, sizeof(server) - 1);
			CHECK(dns_name_totext(name, ISC_FALSE, &buffer));
			server[isc_buffer_usedlength(&buffer)] = 0;
			empty_dbtype[2] = server;
		} else
			empty_dbtype[2] = "@";

		obj = NULL;
		result = named_config_get(maps, "empty-contact", &obj);
		if (result == ISC_R_SUCCESS) {
			CHECK(dns_name_fromstring(name, cfg_obj_asstring(obj),
						 0, NULL));
			isc_buffer_init(&buffer, contact, sizeof(contact) - 1);
			CHECK(dns_name_totext(name, ISC_FALSE, &buffer));
			contact[isc_buffer_usedlength(&buffer)] = 0;
			empty_dbtype[3] = contact;
		} else
			empty_dbtype[3] = ".";

		obj = NULL;
		result = named_config_get(maps, "zone-statistics", &obj);
		INSIST(result == ISC_R_SUCCESS);
		if (cfg_obj_isboolean(obj)) {
			if (cfg_obj_asboolean(obj))
				statlevel = dns_zonestat_full;
			else
				statlevel = dns_zonestat_none;
		} else {
			const char *levelstr = cfg_obj_asstring(obj);
			if (strcasecmp(levelstr, "full") == 0)
				statlevel = dns_zonestat_full;
			else if (strcasecmp(levelstr, "terse") == 0)
				statlevel = dns_zonestat_terse;
			else if (strcasecmp(levelstr, "none") == 0)
				statlevel = dns_zonestat_none;
			else
				INSIST(0);
		}

		for (empty = empty_zones[empty_zone];
		     empty != NULL;
		     empty = empty_zones[++empty_zone])
		{
			dns_forwarders_t *dnsforwarders = NULL;

			/*
			 * Look for zone on drop list.
			 */
			CHECK(dns_name_fromstring(name, empty, 0, NULL));
			if (disablelist != NULL &&
			    on_disable_list(disablelist, name))
				continue;

			/*
			 * This zone already exists.
			 */
			(void)dns_view_findzone(view, name, &zone);
			if (zone != NULL) {
				dns_zone_detach(&zone);
				continue;
			}

			/*
			 * If we would forward this name don't add a
			 * empty zone for it.
			 */
			result = dns_fwdtable_find(view->fwdtable, name,
						   &dnsforwarders);
			if (result == ISC_R_SUCCESS &&
			    dnsforwarders->fwdpolicy == dns_fwdpolicy_only)
				continue;

			/*
			 * See if we can re-use a existing zone.
			 */
			result = dns_viewlist_find(&named_g_server->viewlist,
						   view->name, view->rdclass,
						   &pview);
			if (result != ISC_R_NOTFOUND &&
			    result != ISC_R_SUCCESS)
				goto cleanup;

			if (pview != NULL) {
				(void)dns_view_findzone(pview, name, &zone);
				dns_view_detach(&pview);
			}

			CHECK(create_empty_zone(zone, name, view, zonelist,
						empty_dbtype, empty_dbtypec,
						statlevel));
			if (zone != NULL)
				dns_zone_detach(&zone);
		}
	}

	obj = NULL;
	result = named_config_get(maps, "rate-limit", &obj);
	if (result == ISC_R_SUCCESS) {
		result = configure_rrl(view, config, obj);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
	}

	/*
	 * Set the servfail-ttl.
	 */
	obj = NULL;
	result = named_config_get(maps, "servfail-ttl", &obj);
	INSIST(result == ISC_R_SUCCESS);
	fail_ttl  = cfg_obj_asuint32(obj);
	if (fail_ttl > 30)
		fail_ttl = 30;
	dns_view_setfailttl(view, fail_ttl);

	/*
	 * Name space to look up redirect information in.
	 */
	obj = NULL;
	result = named_config_get(maps, "nxdomain-redirect", &obj);
	if (result == ISC_R_SUCCESS) {
		dns_name_t *name = dns_fixedname_name(&view->redirectfixed);
		CHECK(dns_name_fromstring(name, cfg_obj_asstring(obj), 0,
					  NULL));
		view->redirectzone = name;
	} else
		view->redirectzone = NULL;

#ifdef HAVE_DNSTAP
	/*
	 * Set up the dnstap environment and configure message
	 * types to log.
	 */
	CHECK(configure_dnstap(maps, view));
#endif /* HAVE_DNSTAP */

	result = ISC_R_SUCCESS;

 cleanup:
	if (clients != NULL)
		dns_acl_detach(&clients);
	if (mapped != NULL)
		dns_acl_detach(&mapped);
	if (excluded != NULL)
		dns_acl_detach(&excluded);
	if (ring != NULL)
		dns_tsigkeyring_detach(&ring);
	if (zone != NULL)
		dns_zone_detach(&zone);
	if (dispatch4 != NULL)
		dns_dispatch_detach(&dispatch4);
	if (dispatch6 != NULL)
		dns_dispatch_detach(&dispatch6);
	if (resstats != NULL)
		isc_stats_detach(&resstats);
	if (resquerystats != NULL)
		dns_stats_detach(&resquerystats);
	if (order != NULL)
		dns_order_detach(&order);
	if (cmctx != NULL)
		isc_mem_detach(&cmctx);
	if (hmctx != NULL)
		isc_mem_detach(&hmctx);

	if (cache != NULL)
		dns_cache_detach(&cache);
	if (dctx != NULL)
		dns_dyndb_destroyctx(&dctx);

	return (result);
}

static isc_result_t
configure_hints(dns_view_t *view, const char *filename) {
	isc_result_t result;
	dns_db_t *db;

	db = NULL;
	result = dns_rootns_create(view->mctx, view->rdclass, filename, &db);
	if (result == ISC_R_SUCCESS) {
		dns_view_sethints(view, db);
		dns_db_detach(&db);
	}

	return (result);
}

static isc_result_t
configure_alternates(const cfg_obj_t *config, dns_view_t *view,
		     const cfg_obj_t *alternates)
{
	const cfg_obj_t *portobj;
	const cfg_obj_t *addresses;
	const cfg_listelt_t *element;
	isc_result_t result = ISC_R_SUCCESS;
	in_port_t port;

	/*
	 * Determine which port to send requests to.
	 */
	CHECKM(named_config_getport(config, &port), "port");

	if (alternates != NULL) {
		portobj = cfg_tuple_get(alternates, "port");
		if (cfg_obj_isuint32(portobj)) {
			isc_uint32_t val = cfg_obj_asuint32(portobj);
			if (val > ISC_UINT16_MAX) {
				cfg_obj_log(portobj, named_g_lctx,
					    ISC_LOG_ERROR,
					    "port '%u' out of range", val);
				return (ISC_R_RANGE);
			}
			port = (in_port_t) val;
		}
	}

	addresses = NULL;
	if (alternates != NULL)
		addresses = cfg_tuple_get(alternates, "addresses");

	for (element = cfg_list_first(addresses);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *alternate = cfg_listelt_value(element);
		isc_sockaddr_t sa;

		if (!cfg_obj_issockaddr(alternate)) {
			dns_fixedname_t fixed;
			dns_name_t *name;
			const char *str = cfg_obj_asstring(cfg_tuple_get(
							   alternate, "name"));
			isc_buffer_t buffer;
			in_port_t myport = port;

			isc_buffer_constinit(&buffer, str, strlen(str));
			isc_buffer_add(&buffer, strlen(str));
			name = dns_fixedname_initname(&fixed);
			CHECK(dns_name_fromtext(name, &buffer, dns_rootname, 0,
						NULL));

			portobj = cfg_tuple_get(alternate, "port");
			if (cfg_obj_isuint32(portobj)) {
				isc_uint32_t val = cfg_obj_asuint32(portobj);
				if (val > ISC_UINT16_MAX) {
					cfg_obj_log(portobj, named_g_lctx,
						    ISC_LOG_ERROR,
						    "port '%u' out of range",
						     val);
					return (ISC_R_RANGE);
				}
				myport = (in_port_t) val;
			}
			CHECK(dns_resolver_addalternate(view->resolver, NULL,
							name, myport));
			continue;
		}

		sa = *cfg_obj_assockaddr(alternate);
		if (isc_sockaddr_getport(&sa) == 0)
			isc_sockaddr_setport(&sa, port);
		CHECK(dns_resolver_addalternate(view->resolver, &sa,
						NULL, 0));
	}

 cleanup:
	return (result);
}

static isc_result_t
configure_forward(const cfg_obj_t *config, dns_view_t *view,
		  const dns_name_t *origin, const cfg_obj_t *forwarders,
		   const cfg_obj_t *forwardtype)
{
	const cfg_obj_t *portobj, *dscpobj;
	const cfg_obj_t *faddresses;
	const cfg_listelt_t *element;
	dns_fwdpolicy_t fwdpolicy = dns_fwdpolicy_none;
	dns_forwarderlist_t fwdlist;
	dns_forwarder_t *fwd;
	isc_result_t result;
	in_port_t port;
	isc_dscp_t dscp = -1;

	ISC_LIST_INIT(fwdlist);

	/*
	 * Determine which port to send forwarded requests to.
	 */
	CHECKM(named_config_getport(config, &port), "port");

	if (forwarders != NULL) {
		portobj = cfg_tuple_get(forwarders, "port");
		if (cfg_obj_isuint32(portobj)) {
			isc_uint32_t val = cfg_obj_asuint32(portobj);
			if (val > ISC_UINT16_MAX) {
				cfg_obj_log(portobj, named_g_lctx,
					    ISC_LOG_ERROR,
					    "port '%u' out of range", val);
				return (ISC_R_RANGE);
			}
			port = (in_port_t) val;
		}
	}

	/*
	 * DSCP value for forwarded requests.
	 */
	dscp = named_g_dscp;
	if (forwarders != NULL) {
		dscpobj = cfg_tuple_get(forwarders, "dscp");
		if (cfg_obj_isuint32(dscpobj)) {
			if (cfg_obj_asuint32(dscpobj) > 63) {
				cfg_obj_log(dscpobj, named_g_lctx,
					    ISC_LOG_ERROR,
					    "dscp value '%u' is out of range",
					    cfg_obj_asuint32(dscpobj));
				return (ISC_R_RANGE);
			}
			dscp = (isc_dscp_t)cfg_obj_asuint32(dscpobj);
		}
	}

	faddresses = NULL;
	if (forwarders != NULL)
		faddresses = cfg_tuple_get(forwarders, "addresses");

	for (element = cfg_list_first(faddresses);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *forwarder = cfg_listelt_value(element);
		fwd = isc_mem_get(view->mctx, sizeof(dns_forwarder_t));
		if (fwd == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}
		fwd->addr = *cfg_obj_assockaddr(forwarder);
		if (isc_sockaddr_getport(&fwd->addr) == 0)
			isc_sockaddr_setport(&fwd->addr, port);
		fwd->dscp = cfg_obj_getdscp(forwarder);
		if (fwd->dscp == -1)
			fwd->dscp = dscp;
		ISC_LINK_INIT(fwd, link);
		ISC_LIST_APPEND(fwdlist, fwd, link);
	}

	if (ISC_LIST_EMPTY(fwdlist)) {
		if (forwardtype != NULL)
			cfg_obj_log(forwardtype, named_g_lctx, ISC_LOG_WARNING,
				    "no forwarders seen; disabling "
				    "forwarding");
		fwdpolicy = dns_fwdpolicy_none;
	} else {
		if (forwardtype == NULL)
			fwdpolicy = dns_fwdpolicy_first;
		else {
			const char *forwardstr = cfg_obj_asstring(forwardtype);
			if (strcasecmp(forwardstr, "first") == 0)
				fwdpolicy = dns_fwdpolicy_first;
			else if (strcasecmp(forwardstr, "only") == 0)
				fwdpolicy = dns_fwdpolicy_only;
			else
				INSIST(0);
		}
	}

	result = dns_fwdtable_addfwd(view->fwdtable, origin, &fwdlist,
				     fwdpolicy);
	if (result != ISC_R_SUCCESS) {
		char namebuf[DNS_NAME_FORMATSIZE];
		dns_name_format(origin, namebuf, sizeof(namebuf));
		cfg_obj_log(forwarders, named_g_lctx, ISC_LOG_WARNING,
			    "could not set up forwarding for domain '%s': %s",
			    namebuf, isc_result_totext(result));
		goto cleanup;
	}

	result = ISC_R_SUCCESS;

 cleanup:

	while (!ISC_LIST_EMPTY(fwdlist)) {
		fwd = ISC_LIST_HEAD(fwdlist);
		ISC_LIST_UNLINK(fwdlist, fwd, link);
		isc_mem_put(view->mctx, fwd, sizeof(dns_forwarder_t));
	}

	return (result);
}

static isc_result_t
get_viewinfo(const cfg_obj_t *vconfig, const char **namep,
	     dns_rdataclass_t *classp)
{
	isc_result_t result = ISC_R_SUCCESS;
	const char *viewname;
	dns_rdataclass_t viewclass;

	REQUIRE(namep != NULL && *namep == NULL);
	REQUIRE(classp != NULL);

	if (vconfig != NULL) {
		const cfg_obj_t *classobj = NULL;

		viewname = cfg_obj_asstring(cfg_tuple_get(vconfig, "name"));
		classobj = cfg_tuple_get(vconfig, "class");
		CHECK(named_config_getclass(classobj, dns_rdataclass_in,
					 &viewclass));
		if (dns_rdataclass_ismeta(viewclass)) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "view '%s': class must not be meta",
				      viewname);
			CHECK(ISC_R_FAILURE);
		}
	} else {
		viewname = "_default";
		viewclass = dns_rdataclass_in;
	}

	*namep = viewname;
	*classp = viewclass;

cleanup:
	return (result);
}

/*
 * Find a view based on its configuration info and attach to it.
 *
 * If 'vconfig' is NULL, attach to the default view.
 */
static isc_result_t
find_view(const cfg_obj_t *vconfig, dns_viewlist_t *viewlist,
	  dns_view_t **viewp)
{
	isc_result_t result;
	const char *viewname = NULL;
	dns_rdataclass_t viewclass;
	dns_view_t *view = NULL;

	result = get_viewinfo(vconfig, &viewname, &viewclass);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_viewlist_find(viewlist, viewname, viewclass, &view);
	if (result != ISC_R_SUCCESS)
		return (result);

	*viewp = view;
	return (ISC_R_SUCCESS);
}

/*
 * Create a new view and add it to the list.
 *
 * If 'vconfig' is NULL, create the default view.
 *
 * The view created is attached to '*viewp'.
 */
static isc_result_t
create_view(const cfg_obj_t *vconfig, dns_viewlist_t *viewlist,
	    dns_view_t **viewp)
{
	isc_result_t result;
	const char *viewname = NULL;
	dns_rdataclass_t viewclass;
	dns_view_t *view = NULL;

	result = get_viewinfo(vconfig, &viewname, &viewclass);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_viewlist_find(viewlist, viewname, viewclass, &view);
	if (result == ISC_R_SUCCESS)
		return (ISC_R_EXISTS);
	if (result != ISC_R_NOTFOUND)
		return (result);
	INSIST(view == NULL);

	result = dns_view_create(named_g_mctx, viewclass, viewname, &view);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = isc_entropy_getdata(named_g_entropy, view->secret,
				     sizeof(view->secret), NULL, 0);
	if (result != ISC_R_SUCCESS) {
		dns_view_detach(&view);
		return (result);
	}

	ISC_LIST_APPEND(*viewlist, view, link);
	dns_view_attach(view, viewp);
	return (ISC_R_SUCCESS);
}

/*
 * Configure or reconfigure a zone.
 */
static isc_result_t
configure_zone(const cfg_obj_t *config, const cfg_obj_t *zconfig,
	       const cfg_obj_t *vconfig, isc_mem_t *mctx, dns_view_t *view,
	       dns_viewlist_t *viewlist, cfg_aclconfctx_t *aclconf,
	       isc_boolean_t added, isc_boolean_t old_rpz_ok,
	       isc_boolean_t modify)
{
	dns_view_t *pview = NULL;	/* Production view */
	dns_zone_t *zone = NULL;	/* New or reused zone */
	dns_zone_t *raw = NULL;		/* New or reused raw zone */
	dns_zone_t *dupzone = NULL;
	const cfg_obj_t *options = NULL;
	const cfg_obj_t *zoptions = NULL;
	const cfg_obj_t *typeobj = NULL;
	const cfg_obj_t *forwarders = NULL;
	const cfg_obj_t *forwardtype = NULL;
	const cfg_obj_t *only = NULL;
	const cfg_obj_t *signing = NULL;
	const cfg_obj_t *viewobj = NULL;
	isc_result_t result;
	isc_result_t tresult;
	isc_buffer_t buffer;
	dns_fixedname_t fixorigin;
	dns_name_t *origin;
	const char *zname;
	dns_rdataclass_t zclass;
	const char *ztypestr;
	dns_rpz_num_t rpz_num;
	isc_boolean_t zone_is_catz = ISC_FALSE;

	options = NULL;
	(void)cfg_map_get(config, "options", &options);

	zoptions = cfg_tuple_get(zconfig, "options");

	/*
	 * Get the zone origin as a dns_name_t.
	 */
	zname = cfg_obj_asstring(cfg_tuple_get(zconfig, "name"));
	isc_buffer_constinit(&buffer, zname, strlen(zname));
	isc_buffer_add(&buffer, strlen(zname));
	dns_fixedname_init(&fixorigin);
	CHECK(dns_name_fromtext(dns_fixedname_name(&fixorigin),
				&buffer, dns_rootname, 0, NULL));
	origin = dns_fixedname_name(&fixorigin);

	CHECK(named_config_getclass(cfg_tuple_get(zconfig, "class"),
				 view->rdclass, &zclass));
	if (zclass != view->rdclass) {
		const char *vname = NULL;
		if (vconfig != NULL)
			vname = cfg_obj_asstring(cfg_tuple_get(vconfig,
							       "name"));
		else
			vname = "<default view>";

		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "zone '%s': wrong class for view '%s'",
			      zname, vname);
		result = ISC_R_FAILURE;
		goto cleanup;
	}

	(void)cfg_map_get(zoptions, "in-view", &viewobj);
	if (viewobj != NULL) {
		const char *inview = cfg_obj_asstring(viewobj);
		dns_view_t *otherview = NULL;

		if (viewlist == NULL) {
			cfg_obj_log(zconfig, named_g_lctx, ISC_LOG_ERROR,
				    "'in-view' option is not permitted in "
				    "dynamically added zones");
			result = ISC_R_FAILURE;
			goto cleanup;
		}

		result = dns_viewlist_find(viewlist, inview, view->rdclass,
					   &otherview);
		if (result != ISC_R_SUCCESS) {
			cfg_obj_log(zconfig, named_g_lctx, ISC_LOG_ERROR,
				    "view '%s' is not yet defined.", inview);
			result = ISC_R_FAILURE;
			goto cleanup;
		}

		result = dns_view_findzone(otherview, origin, &zone);
		dns_view_detach(&otherview);
		if (result != ISC_R_SUCCESS) {
			cfg_obj_log(zconfig, named_g_lctx, ISC_LOG_ERROR,
				    "zone '%s' not defined in view '%s'",
				    zname, inview);
			result = ISC_R_FAILURE;
			goto cleanup;
		}

		CHECK(dns_view_addzone(view, zone));
		dns_zone_detach(&zone);

		/*
		 * If the zone contains a 'forwarders' statement, configure
		 * selective forwarding.  Note: this is not inherited from the
		 * other view.
		 */
		forwarders = NULL;
		result = cfg_map_get(zoptions, "forwarders", &forwarders);
		if (result == ISC_R_SUCCESS) {
			forwardtype = NULL;
			(void)cfg_map_get(zoptions, "forward", &forwardtype);
			CHECK(configure_forward(config, view, origin,
						forwarders, forwardtype));
		}
		result = ISC_R_SUCCESS;
		goto cleanup;
	}

	(void)cfg_map_get(zoptions, "type", &typeobj);
	if (typeobj == NULL) {
		cfg_obj_log(zconfig, named_g_lctx, ISC_LOG_ERROR,
			    "zone '%s' 'type' not specified", zname);
		result = ISC_R_FAILURE;
		goto cleanup;
	}
	ztypestr = cfg_obj_asstring(typeobj);

	/*
	 * "hints zones" aren't zones.	If we've got one,
	 * configure it and return.
	 */
	if (strcasecmp(ztypestr, "hint") == 0) {
		const cfg_obj_t *fileobj = NULL;
		if (cfg_map_get(zoptions, "file", &fileobj) != ISC_R_SUCCESS) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "zone '%s': 'file' not specified",
				      zname);
			result = ISC_R_FAILURE;
			goto cleanup;
		}
		if (dns_name_equal(origin, dns_rootname)) {
			const char *hintsfile = cfg_obj_asstring(fileobj);

			CHECK(configure_hints(view, hintsfile));

			/*
			 * Hint zones may also refer to delegation only points.
			 */
			only = NULL;
			tresult = cfg_map_get(zoptions, "delegation-only",
					      &only);
			if (tresult == ISC_R_SUCCESS && cfg_obj_asboolean(only))
				CHECK(dns_view_adddelegationonly(view, origin));
		} else {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
				      "ignoring non-root hint zone '%s'",
				      zname);
			result = ISC_R_SUCCESS;
		}
		/* Skip ordinary zone processing. */
		goto cleanup;
	}

	/*
	 * "forward zones" aren't zones either.  Translate this syntax into
	 * the appropriate selective forwarding configuration and return.
	 */
	if (strcasecmp(ztypestr, "forward") == 0) {
		forwardtype = NULL;
		forwarders = NULL;

		(void)cfg_map_get(zoptions, "forward", &forwardtype);
		(void)cfg_map_get(zoptions, "forwarders", &forwarders);
		CHECK(configure_forward(config, view, origin, forwarders,
					forwardtype));

		/*
		 * Forward zones may also set delegation only.
		 */
		only = NULL;
		tresult = cfg_map_get(zoptions, "delegation-only", &only);
		if (tresult == ISC_R_SUCCESS && cfg_obj_asboolean(only))
			CHECK(dns_view_adddelegationonly(view, origin));
		goto cleanup;
	}

	/*
	 * "delegation-only zones" aren't zones either.
	 */
	if (strcasecmp(ztypestr, "delegation-only") == 0) {
		result = dns_view_adddelegationonly(view, origin);
		goto cleanup;
	}

	/*
	 * Redirect zones only require minimal configuration.
	 */
	if (strcasecmp(ztypestr, "redirect") == 0) {
		if (view->redirect != NULL) {
			cfg_obj_log(zconfig, named_g_lctx, ISC_LOG_ERROR,
				    "redirect zone already exists");
			result = ISC_R_EXISTS;
			goto cleanup;
		}
		result = dns_viewlist_find(viewlist, view->name,
					   view->rdclass, &pview);
		if (result != ISC_R_NOTFOUND && result != ISC_R_SUCCESS)
			goto cleanup;
		if (pview != NULL && pview->redirect != NULL) {
			dns_zone_attach(pview->redirect, &zone);
			dns_zone_setview(zone, view);
		} else {
			CHECK(dns_zonemgr_createzone(named_g_server->zonemgr,
						     &zone));
			CHECK(dns_zone_setorigin(zone, origin));
			dns_zone_setview(zone, view);
			CHECK(dns_zonemgr_managezone(named_g_server->zonemgr,
						     zone));
			dns_zone_setstats(zone, named_g_server->zonestats);
		}
		CHECK(named_zone_configure(config, vconfig, zconfig,
					   aclconf, zone, NULL));
		dns_zone_attach(zone, &view->redirect);
		goto cleanup;
	}

	if (!modify) {
		/*
		 * Check for duplicates in the new zone table.
		 */
		result = dns_view_findzone(view, origin, &dupzone);
		if (result == ISC_R_SUCCESS) {
			/*
			 * We already have this zone!
			 */
			cfg_obj_log(zconfig, named_g_lctx, ISC_LOG_ERROR,
				    "zone '%s' already exists", zname);
			dns_zone_detach(&dupzone);
			result = ISC_R_EXISTS;
			goto cleanup;
		}
		INSIST(dupzone == NULL);
	}

	/*
	 * Note whether this is a response policy zone and which one if so,
	 * unless we are using RPZ service interface.  In that case, the
	 * BIND zone database has nothing to do with rpz and so we don't care.
	 */
	for (rpz_num = 0; ; ++rpz_num) {
		if (view->rpzs == NULL || rpz_num >= view->rpzs->p.num_zones ||
		    view->rpzs->p.dnsrps_enabled)
		{
			rpz_num = DNS_RPZ_INVALID_NUM;
			break;
		}
		if (dns_name_equal(&view->rpzs->zones[rpz_num]->origin, origin))
			break;
	}

	if (view->catzs != NULL &&
	    dns_catz_get_zone(view->catzs, origin) != NULL)
		zone_is_catz = ISC_TRUE;

	/*
	 * See if we can reuse an existing zone.  This is
	 * only possible if all of these are true:
	 *   - The zone's view exists
	 *   - A zone with the right name exists in the view
	 *   - The zone is compatible with the config
	 *     options (e.g., an existing master zone cannot
	 *     be reused if the options specify a slave zone)
	 *   - The zone was not and is still not a response policy zone
	 *     or the zone is a policy zone with an unchanged number
	 *     and we are using the old policy zone summary data.
	 */
	result = dns_viewlist_find(&named_g_server->viewlist, view->name,
				   view->rdclass, &pview);
	if (result != ISC_R_NOTFOUND && result != ISC_R_SUCCESS)
		goto cleanup;
	if (pview != NULL)
		result = dns_view_findzone(pview, origin, &zone);
	if (result != ISC_R_NOTFOUND && result != ISC_R_SUCCESS)
		goto cleanup;

	if (zone != NULL && !named_zone_reusable(zone, zconfig))
		dns_zone_detach(&zone);

	if (zone != NULL && (rpz_num != dns_zone_get_rpz_num(zone) ||
			     (rpz_num != DNS_RPZ_INVALID_NUM && !old_rpz_ok)))
		dns_zone_detach(&zone);

	if (zone != NULL) {
		/*
		 * We found a reusable zone.  Make it use the
		 * new view.
		 */
		dns_zone_setview(zone, view);
	} else {
		/*
		 * We cannot reuse an existing zone, we have
		 * to create a new one.
		 */
		CHECK(dns_zonemgr_createzone(named_g_server->zonemgr, &zone));
		CHECK(dns_zone_setorigin(zone, origin));
		dns_zone_setview(zone, view);
		CHECK(dns_zonemgr_managezone(named_g_server->zonemgr, zone));
		dns_zone_setstats(zone, named_g_server->zonestats);
	}
	if (rpz_num != DNS_RPZ_INVALID_NUM) {
		result = dns_zone_rpz_enable(zone, view->rpzs, rpz_num);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "zone '%s': incompatible"
				      " masterfile-format or database"
				      " for a response policy zone",
				      zname);
			goto cleanup;
		}
	}

	if (zone_is_catz)
		dns_zone_catz_enable(zone, view->catzs);

	/*
	 * If the zone contains a 'forwarders' statement, configure
	 * selective forwarding.
	 */
	forwarders = NULL;
	if (cfg_map_get(zoptions, "forwarders", &forwarders) == ISC_R_SUCCESS)
	{
		forwardtype = NULL;
		(void)cfg_map_get(zoptions, "forward", &forwardtype);
		CHECK(configure_forward(config, view, origin, forwarders,
					forwardtype));
	}

	/*
	 * Stub and forward zones may also refer to delegation only points.
	 */
	only = NULL;
	if (cfg_map_get(zoptions, "delegation-only", &only) == ISC_R_SUCCESS)
	{
		if (cfg_obj_asboolean(only))
			CHECK(dns_view_adddelegationonly(view, origin));
	}

	/*
	 * Mark whether the zone was originally added at runtime or not
	 */
	dns_zone_setadded(zone, added);

	signing = NULL;
	if ((strcasecmp(ztypestr, "master") == 0 ||
	     strcasecmp(ztypestr, "slave") == 0) &&
	    cfg_map_get(zoptions, "inline-signing", &signing) == ISC_R_SUCCESS &&
	    cfg_obj_asboolean(signing))
	{
		dns_zone_getraw(zone, &raw);
		if (raw == NULL) {
			CHECK(dns_zone_create(&raw, mctx));
			CHECK(dns_zone_setorigin(raw, origin));
			dns_zone_setview(raw, view);
			dns_zone_setstats(raw, named_g_server->zonestats);
			CHECK(dns_zone_link(zone, raw));
		}
	}

	/*
	 * Configure the zone.
	 */
	CHECK(named_zone_configure(config, vconfig, zconfig,
				   aclconf, zone, raw));

	/*
	 * Add the zone to its view in the new view list.
	 */
	if (!modify)
		CHECK(dns_view_addzone(view, zone));

	if (zone_is_catz) {
		/*
		 * force catz reload if the zone is loaded;
		 * if it's not it'll get reloaded on zone load
		 */
		dns_db_t *db = NULL;

		tresult = dns_zone_getdb(zone, &db);
		if (tresult == ISC_R_SUCCESS) {
			dns_catz_dbupdate_callback(db, view->catzs);
			dns_db_detach(&db);
		}

	}

	/*
	 * Ensure that zone keys are reloaded on reconfig
	 */
	if ((dns_zone_getkeyopts(zone) & DNS_ZONEKEY_MAINTAIN) != 0)
		dns_zone_rekey(zone, ISC_FALSE);

 cleanup:
	if (zone != NULL)
		dns_zone_detach(&zone);
	if (raw != NULL)
		dns_zone_detach(&raw);
	if (pview != NULL)
		dns_view_detach(&pview);

	return (result);
}

/*
 * Configure built-in zone for storing managed-key data.
 */
static isc_result_t
add_keydata_zone(dns_view_t *view, const char *directory, isc_mem_t *mctx) {
	isc_result_t result;
	dns_view_t *pview = NULL;
	dns_zone_t *zone = NULL;
	dns_acl_t *none = NULL;
	char filename[PATH_MAX];
	isc_boolean_t defaultview;

	REQUIRE(view != NULL);

	/* See if we can re-use an existing keydata zone. */
	result = dns_viewlist_find(&named_g_server->viewlist,
				   view->name, view->rdclass, &pview);
	if (result != ISC_R_NOTFOUND && result != ISC_R_SUCCESS) {
		return (result);
	}

	if (pview != NULL) {
		if (pview->managed_keys != NULL) {
			dns_zone_attach(pview->managed_keys,
					&view->managed_keys);
			dns_zone_setview(pview->managed_keys, view);
			dns_view_detach(&pview);
			dns_zone_synckeyzone(view->managed_keys);
			return (ISC_R_SUCCESS);
		}

		dns_view_detach(&pview);
	}

	/* No existing keydata zone was found; create one */
	CHECK(dns_zonemgr_createzone(named_g_server->zonemgr, &zone));
	CHECK(dns_zone_setorigin(zone, dns_rootname));

	defaultview = ISC_TF(strcmp(view->name, "_default") == 0);
	CHECK(isc_file_sanitize(directory,
				defaultview ? "managed-keys" : view->name,
				defaultview ? "bind" : "mkeys",
				filename, sizeof(filename)));
	CHECK(dns_zone_setfile(zone, filename));

	dns_zone_setview(zone, view);
	dns_zone_settype(zone, dns_zone_key);
	dns_zone_setclass(zone, view->rdclass);

	CHECK(dns_zonemgr_managezone(named_g_server->zonemgr, zone));

	CHECK(dns_acl_none(mctx, &none));
	dns_zone_setqueryacl(zone, none);
	dns_zone_setqueryonacl(zone, none);
	dns_acl_detach(&none);

	dns_zone_setdialup(zone, dns_dialuptype_no);
	dns_zone_setnotifytype(zone, dns_notifytype_no);
	dns_zone_setoption(zone, DNS_ZONEOPT_NOCHECKNS, ISC_TRUE);
	dns_zone_setjournalsize(zone, 0);

	dns_zone_setstats(zone, named_g_server->zonestats);
	CHECK(setquerystats(zone, mctx, dns_zonestat_none));

	if (view->managed_keys != NULL) {
		dns_zone_detach(&view->managed_keys);
	}
	dns_zone_attach(zone, &view->managed_keys);

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "set up managed keys zone for view %s, file '%s'",
		      view->name, filename);

cleanup:
	if (zone != NULL) {
		dns_zone_detach(&zone);
	}
	if (none != NULL) {
		dns_acl_detach(&none);
	}

	return (result);
}

/*
 * Configure a single server quota.
 */
static void
configure_server_quota(const cfg_obj_t **maps, const char *name,
		       isc_quota_t *quota)
{
	const cfg_obj_t *obj = NULL;
	isc_result_t result;

	result = named_config_get(maps, name, &obj);
	INSIST(result == ISC_R_SUCCESS);
	isc_quota_max(quota, cfg_obj_asuint32(obj));
}

/*
 * This function is called as soon as the 'directory' statement has been
 * parsed.  This can be extended to support other options if necessary.
 */
static isc_result_t
directory_callback(const char *clausename, const cfg_obj_t *obj, void *arg) {
	isc_result_t result;
	const char *directory;

	REQUIRE(strcasecmp("directory", clausename) == 0);

	UNUSED(arg);
	UNUSED(clausename);

	/*
	 * Change directory.
	 */
	directory = cfg_obj_asstring(obj);

	if (! isc_file_ischdiridempotent(directory))
		cfg_obj_log(obj, named_g_lctx, ISC_LOG_WARNING,
			    "option 'directory' contains relative path '%s'",
			    directory);

	if (!isc_file_isdirwritable(directory)) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "directory '%s' is not writable",
			      directory);
		return (ISC_R_NOPERM);
	}

	result = isc_dir_chdir(directory);
	if (result != ISC_R_SUCCESS) {
		cfg_obj_log(obj, named_g_lctx, ISC_LOG_ERROR,
			    "change directory to '%s' failed: %s",
			    directory, isc_result_totext(result));
		return (result);
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
add_listenelt(isc_mem_t *mctx, ns_listenlist_t *list, isc_sockaddr_t *addr,
	      isc_dscp_t dscp, isc_boolean_t wcardport_ok)
{
	ns_listenelt_t *lelt = NULL;
	dns_acl_t *src_acl = NULL;
	isc_result_t result;
	isc_sockaddr_t any_sa6;
	isc_netaddr_t netaddr;

	REQUIRE(isc_sockaddr_pf(addr) == AF_INET6);

	isc_sockaddr_any6(&any_sa6);
	if (!isc_sockaddr_equal(&any_sa6, addr) &&
	    (wcardport_ok || isc_sockaddr_getport(addr) != 0)) {
		isc_netaddr_fromin6(&netaddr, &addr->type.sin6.sin6_addr);

		result = dns_acl_create(mctx, 0, &src_acl);
		if (result != ISC_R_SUCCESS)
			return (result);

		result = dns_iptable_addprefix(src_acl->iptable,
					       &netaddr, 128, ISC_TRUE);
		if (result != ISC_R_SUCCESS)
			goto clean;

		result = ns_listenelt_create(mctx, isc_sockaddr_getport(addr),
					     dscp, src_acl, &lelt);
		if (result != ISC_R_SUCCESS)
			goto clean;
		ISC_LIST_APPEND(list->elts, lelt, link);
	}

	return (ISC_R_SUCCESS);

 clean:
	INSIST(lelt == NULL);
	dns_acl_detach(&src_acl);

	return (result);
}

/*
 * Make a list of xxx-source addresses and call ns_interfacemgr_adjust()
 * to update the listening interfaces accordingly.
 * We currently only consider IPv6, because this only affects IPv6 wildcard
 * sockets.
 */
static void
adjust_interfaces(named_server_t *server, isc_mem_t *mctx) {
	isc_result_t result;
	ns_listenlist_t *list = NULL;
	dns_view_t *view;
	dns_zone_t *zone, *next;
	isc_sockaddr_t addr, *addrp;
	isc_dscp_t dscp = -1;

	result = ns_listenlist_create(mctx, &list);
	if (result != ISC_R_SUCCESS)
		return;

	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link)) {
		dns_dispatch_t *dispatch6;

		dispatch6 = dns_resolver_dispatchv6(view->resolver);
		if (dispatch6 == NULL)
			continue;
		result = dns_dispatch_getlocaladdress(dispatch6, &addr);
		if (result != ISC_R_SUCCESS)
			goto fail;

		/*
		 * We always add non-wildcard address regardless of whether
		 * the port is 'any' (the fourth arg is TRUE): if the port is
		 * specific, we need to add it since it may conflict with a
		 * listening interface; if it's zero, we'll dynamically open
		 * query ports, and some of them may override an existing
		 * wildcard IPv6 port.
		 */
		/* XXXMPA fix dscp */
		result = add_listenelt(mctx, list, &addr, dscp, ISC_TRUE);
		if (result != ISC_R_SUCCESS)
			goto fail;
	}

	zone = NULL;
	for (result = dns_zone_first(server->zonemgr, &zone);
	     result == ISC_R_SUCCESS;
	     next = NULL, result = dns_zone_next(zone, &next), zone = next) {
		dns_view_t *zoneview;

		/*
		 * At this point the zone list may contain a stale zone
		 * just removed from the configuration.  To see the validity,
		 * check if the corresponding view is in our current view list.
		 * There may also be old zones that are still in the process
		 * of shutting down and have detached from their old view
		 * (zoneview == NULL).
		 */
		zoneview = dns_zone_getview(zone);
		if (zoneview == NULL)
			continue;
		for (view = ISC_LIST_HEAD(server->viewlist);
		     view != NULL && view != zoneview;
		     view = ISC_LIST_NEXT(view, link))
			;
		if (view == NULL)
			continue;

		addrp = dns_zone_getnotifysrc6(zone);
		dscp = dns_zone_getnotifysrc6dscp(zone);
		result = add_listenelt(mctx, list, addrp, dscp, ISC_FALSE);
		if (result != ISC_R_SUCCESS)
			goto fail;

		addrp = dns_zone_getxfrsource6(zone);
		dscp = dns_zone_getxfrsource6dscp(zone);
		result = add_listenelt(mctx, list, addrp, dscp, ISC_FALSE);
		if (result != ISC_R_SUCCESS)
			goto fail;
	}

	ns_interfacemgr_adjust(server->interfacemgr, list, ISC_TRUE);

 clean:
	ns_listenlist_detach(&list);
	return;

 fail:
	/*
	 * Even when we failed the procedure, most of other interfaces
	 * should work correctly.  We therefore just warn it.
	 */
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
		      "could not adjust the listen-on list; "
		      "some interfaces may not work");
	goto clean;
}

/*
 * This event callback is invoked to do periodic network interface
 * scanning.
 */

static void
interface_timer_tick(isc_task_t *task, isc_event_t *event) {
	named_server_t *server = (named_server_t *) event->ev_arg;
	INSIST(task == server->task);
	UNUSED(task);

	isc_event_free(&event);
	ns_interfacemgr_scan(server->interfacemgr, ISC_FALSE);
}

static void
heartbeat_timer_tick(isc_task_t *task, isc_event_t *event) {
	named_server_t *server = (named_server_t *) event->ev_arg;
	dns_view_t *view;

	UNUSED(task);
	isc_event_free(&event);
	view = ISC_LIST_HEAD(server->viewlist);
	while (view != NULL) {
		dns_view_dialup(view);
		view = ISC_LIST_NEXT(view, link);
	}
}

typedef struct {
       isc_mem_t       *mctx;
       isc_task_t      *task;
       dns_rdataset_t  rdataset;
       dns_rdataset_t  sigrdataset;
       dns_fetch_t     *fetch;
} ns_tat_t;

static int
cid(const void *a, const void *b) {
	const isc_uint16_t ida = *(const isc_uint16_t *)a;
	const isc_uint16_t idb = *(const isc_uint16_t *)b;
	if (ida < idb)
		return (-1);
	else if (ida > idb)
		return (1);
	else
		return (0);
}

static void
tat_done(isc_task_t *task, isc_event_t *event) {
	dns_fetchevent_t *devent;
	ns_tat_t *tat;

	UNUSED(task);
	INSIST(event != NULL && event->ev_type == DNS_EVENT_FETCHDONE);
	INSIST(event->ev_arg != NULL);

	tat = event->ev_arg;
	devent = (dns_fetchevent_t *) event;

	/* Free resources which are not of interest */
	if (devent->node != NULL)
		dns_db_detachnode(devent->db, &devent->node);
	if (devent->db != NULL)
		dns_db_detach(&devent->db);
	isc_event_free(&event);
	dns_resolver_destroyfetch(&tat->fetch);
	if (dns_rdataset_isassociated(&tat->rdataset))
		dns_rdataset_disassociate(&tat->rdataset);
	if (dns_rdataset_isassociated(&tat->sigrdataset))
		dns_rdataset_disassociate(&tat->sigrdataset);
	isc_task_detach(&tat->task);
	isc_mem_putanddetach(&tat->mctx, tat, sizeof(*tat));
}

struct dotat_arg {
	dns_view_t *view;
	isc_task_t *task;
};

static void
dotat(dns_keytable_t *keytable, dns_keynode_t *keynode, void *arg) {
	isc_result_t result;
	dns_keynode_t *firstnode = keynode;
	dns_keynode_t *nextnode;
	unsigned int i, n = 0;
	char label[64], namebuf[DNS_NAME_FORMATSIZE];
	dns_fixedname_t fixed;
	dns_name_t *tatname;
	isc_uint16_t ids[12]; /* Only 12 id's will fit in a label. */
	int m;
	ns_tat_t *tat;
	dns_name_t *name = NULL;
	struct dotat_arg *dotat_arg = arg;
	dns_view_t *view;
	isc_task_t *task;
	isc_textregion_t r;

	REQUIRE(keytable != NULL);
	REQUIRE(keynode != NULL);
	REQUIRE(arg != NULL);

	view = dotat_arg->view;
	task = dotat_arg->task;

	do {
		dst_key_t *key = dns_keynode_key(keynode);
		if (key != NULL) {
			name = dst_key_name(key);
			if (n < (sizeof(ids)/sizeof(ids[0]))) {
				ids[n] = dst_key_id(key);
				n++;
			}
		}
		nextnode = NULL;
		(void)dns_keytable_nextkeynode(keytable, keynode, &nextnode);
		if (keynode != firstnode) {
			dns_keytable_detachkeynode(keytable, &keynode);
		}
		keynode = nextnode;
	} while (keynode != NULL);

	if (n == 0) {
		return;
	}

	if (n > 1) {
		qsort(ids, n, sizeof(ids[0]), cid);
	}

	/*
	 * Encoded as "_ta-xxxx\(-xxxx\)*" where xxxx is the hex version of
	 * of the keyid.
	 */
	label[0] = 0;
	r.base = label;
	r.length = sizeof(label);
	m = snprintf(r.base, r.length, "_ta");
	if (m < 0 || (unsigned)m > r.length) {
		return;
	}
	isc_textregion_consume(&r, m);
	for (i = 0; i < n; i++) {
		m = snprintf(r.base, r.length, "-%04x", ids[i]);
		if (m < 0 || (unsigned)m > r.length) {
			return;
		}
		isc_textregion_consume(&r, m);
	}
	tatname = dns_fixedname_initname(&fixed);
	result = dns_name_fromstring2(tatname, label, name, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		return;
	}

	dns_name_format(tatname, namebuf, sizeof(namebuf));
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		     "%s: sending trust-anchor-telemetry query '%s/NULL'",
		     view->name, namebuf);

	tat = isc_mem_get(dotat_arg->view->mctx, sizeof(*tat));
	if (tat == NULL) {
		return;
	}

	tat->mctx = NULL;
	tat->task = NULL;
	tat->fetch = NULL;
	dns_rdataset_init(&tat->rdataset);
	dns_rdataset_init(&tat->sigrdataset);
	isc_mem_attach(dotat_arg->view->mctx, &tat->mctx);
	isc_task_attach(task, &tat->task);

	result = dns_resolver_createfetch(view->resolver, tatname,
					  dns_rdatatype_null, NULL, NULL,
					  NULL, 0, tat->task, tat_done, tat,
					  &tat->rdataset, &tat->sigrdataset,
					  &tat->fetch);

	if (result != ISC_R_SUCCESS) {
		isc_task_detach(&tat->task);
		isc_mem_putanddetach(&tat->mctx, tat, sizeof(*tat));
	}
}

static void
tat_timer_tick(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	named_server_t *server = (named_server_t *) event->ev_arg;
	struct dotat_arg arg;
	dns_view_t *view;
	dns_keytable_t *secroots = NULL;

	isc_event_free(&event);

	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		if (!view->trust_anchor_telemetry ||
		    !view->enablevalidation)
		{
			continue;
		}

		result = dns_view_getsecroots(view, &secroots);
		if (result != ISC_R_SUCCESS) {
			continue;
		}

		arg.view = view;
		arg.task = task;
		(void)dns_keytable_forall(secroots, dotat, &arg);
		dns_keytable_detach(&secroots);
	}
}

static void
pps_timer_tick(isc_task_t *task, isc_event_t *event) {
	static unsigned int oldrequests = 0;
	unsigned int requests = ns_client_requests;

	UNUSED(task);
	isc_event_free(&event);

	/*
	 * Don't worry about wrapping as the overflow result will be right.
	 */
	dns_pps = (requests - oldrequests) / 1200;
	oldrequests = requests;
}

/*
 * Replace the current value of '*field', a dynamically allocated
 * string or NULL, with a dynamically allocated copy of the
 * null-terminated string pointed to by 'value', or NULL.
 */
static isc_result_t
setstring(named_server_t *server, char **field, const char *value) {
	char *copy;

	if (value != NULL) {
		copy = isc_mem_strdup(server->mctx, value);
		if (copy == NULL)
			return (ISC_R_NOMEMORY);
	} else {
		copy = NULL;
	}

	if (*field != NULL)
		isc_mem_free(server->mctx, *field);

	*field = copy;
	return (ISC_R_SUCCESS);
}

/*
 * Replace the current value of '*field', a dynamically allocated
 * string or NULL, with another dynamically allocated string
 * or NULL if whether 'obj' is a string or void value, respectively.
 */
static isc_result_t
setoptstring(named_server_t *server, char **field, const cfg_obj_t *obj) {
	if (cfg_obj_isvoid(obj))
		return (setstring(server, field, NULL));
	else
		return (setstring(server, field, cfg_obj_asstring(obj)));
}

static void
set_limit(const cfg_obj_t **maps, const char *configname,
	  const char *description, isc_resource_t resourceid,
	  isc_resourcevalue_t defaultvalue)
{
	const cfg_obj_t *obj = NULL;
	const char *resource;
	isc_resourcevalue_t value;
	isc_result_t result;

	if (named_config_get(maps, configname, &obj) != ISC_R_SUCCESS)
		return;

	if (cfg_obj_isstring(obj)) {
		resource = cfg_obj_asstring(obj);
		if (strcasecmp(resource, "unlimited") == 0)
			value = ISC_RESOURCE_UNLIMITED;
		else {
			INSIST(strcasecmp(resource, "default") == 0);
			value = defaultvalue;
		}
	} else
		value = cfg_obj_asuint64(obj);

	result = isc_resource_setlimit(resourceid, value);
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER,
		      result == ISC_R_SUCCESS ?
			ISC_LOG_DEBUG(3) : ISC_LOG_WARNING,
		      "set maximum %s to %" ISC_PRINT_QUADFORMAT "u: %s",
		      description, value, isc_result_totext(result));
}

#define SETLIMIT(cfgvar, resource, description) \
	set_limit(maps, cfgvar, description, isc_resource_ ## resource, \
		  named_g_init ## resource)

static void
set_limits(const cfg_obj_t **maps) {
	SETLIMIT("stacksize", stacksize, "stack size");
	SETLIMIT("datasize", datasize, "data size");
	SETLIMIT("coresize", coresize, "core size");
	SETLIMIT("files", openfiles, "open files");
}

static void
portset_fromconf(isc_portset_t *portset, const cfg_obj_t *ports,
		 isc_boolean_t positive)
{
	const cfg_listelt_t *element;

	for (element = cfg_list_first(ports);
	     element != NULL;
	     element = cfg_list_next(element)) {
		const cfg_obj_t *obj = cfg_listelt_value(element);

		if (cfg_obj_isuint32(obj)) {
			in_port_t port = (in_port_t)cfg_obj_asuint32(obj);

			if (positive)
				isc_portset_add(portset, port);
			else
				isc_portset_remove(portset, port);
		} else {
			const cfg_obj_t *obj_loport, *obj_hiport;
			in_port_t loport, hiport;

			obj_loport = cfg_tuple_get(obj, "loport");
			loport = (in_port_t)cfg_obj_asuint32(obj_loport);
			obj_hiport = cfg_tuple_get(obj, "hiport");
			hiport = (in_port_t)cfg_obj_asuint32(obj_hiport);

			if (positive)
				isc_portset_addrange(portset, loport, hiport);
			else {
				isc_portset_removerange(portset, loport,
							hiport);
			}
		}
	}
}

static isc_result_t
removed(dns_zone_t *zone, void *uap) {
	const char *type;

	if (dns_zone_getview(zone) != uap)
		return (ISC_R_SUCCESS);

	switch (dns_zone_gettype(zone)) {
	case dns_zone_master:
		type = "master";
		break;
	case dns_zone_slave:
		type = "slave";
		break;
	case dns_zone_stub:
		type = "stub";
		break;
	case dns_zone_staticstub:
		type = "static-stub";
		break;
	case dns_zone_redirect:
		type = "redirect";
		break;
	default:
		type = "other";
		break;
	}
	dns_zone_log(zone, ISC_LOG_INFO, "(%s) removed", type);
	return (ISC_R_SUCCESS);
}

static void
cleanup_session_key(named_server_t *server, isc_mem_t *mctx) {
	if (server->session_keyfile != NULL) {
		isc_file_remove(server->session_keyfile);
		isc_mem_free(mctx, server->session_keyfile);
		server->session_keyfile = NULL;
	}

	if (server->session_keyname != NULL) {
		if (dns_name_dynamic(server->session_keyname))
			dns_name_free(server->session_keyname, mctx);
		isc_mem_put(mctx, server->session_keyname, sizeof(dns_name_t));
		server->session_keyname = NULL;
	}

	if (server->sessionkey != NULL)
		dns_tsigkey_detach(&server->sessionkey);

	server->session_keyalg = DST_ALG_UNKNOWN;
	server->session_keybits = 0;
}

static isc_result_t
generate_session_key(const char *filename, const char *keynamestr,
		     const dns_name_t *keyname, const char *algstr,
		     const dns_name_t *algname, unsigned int algtype,
		     isc_uint16_t bits, isc_mem_t *mctx,
		     dns_tsigkey_t **tsigkeyp)
{
	isc_result_t result = ISC_R_SUCCESS;
	dst_key_t *key = NULL;
	isc_buffer_t key_txtbuffer;
	isc_buffer_t key_rawbuffer;
	char key_txtsecret[256];
	char key_rawsecret[64];
	isc_region_t key_rawregion;
	isc_stdtime_t now;
	dns_tsigkey_t *tsigkey = NULL;
	FILE *fp = NULL;

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "generating session key for dynamic DNS");

	/* generate key */
	result = dst_key_generate(keyname, algtype, bits, 1, 0,
				  DNS_KEYPROTO_ANY, dns_rdataclass_in,
				  mctx, &key);
	if (result != ISC_R_SUCCESS)
		return (result);

	/*
	 * Dump the key to the buffer for later use.  Should be done before
	 * we transfer the ownership of key to tsigkey.
	 */
	isc_buffer_init(&key_rawbuffer, &key_rawsecret, sizeof(key_rawsecret));
	CHECK(dst_key_tobuffer(key, &key_rawbuffer));

	isc_buffer_usedregion(&key_rawbuffer, &key_rawregion);
	isc_buffer_init(&key_txtbuffer, &key_txtsecret, sizeof(key_txtsecret));
	CHECK(isc_base64_totext(&key_rawregion, -1, "", &key_txtbuffer));

	/* Store the key in tsigkey. */
	isc_stdtime_get(&now);
	CHECK(dns_tsigkey_createfromkey(dst_key_name(key), algname, key,
					ISC_FALSE, NULL, now, now, mctx, NULL,
					&tsigkey));

	/* Dump the key to the key file. */
	fp = named_os_openfile(filename, S_IRUSR|S_IWUSR, ISC_TRUE);
	if (fp == NULL) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "could not create %s", filename);
		result = ISC_R_NOPERM;
		goto cleanup;
	}

	fprintf(fp, "key \"%s\" {\n"
		"\talgorithm %s;\n"
		"\tsecret \"%.*s\";\n};\n", keynamestr, algstr,
		(int) isc_buffer_usedlength(&key_txtbuffer),
		(char*) isc_buffer_base(&key_txtbuffer));

	CHECK(isc_stdio_flush(fp));
	result = isc_stdio_close(fp);
	fp = NULL;
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	dst_key_free(&key);

	*tsigkeyp = tsigkey;

	return (ISC_R_SUCCESS);

  cleanup:
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "failed to generate session key "
		      "for dynamic DNS: %s", isc_result_totext(result));
	if (fp != NULL) {
		(void)isc_stdio_close(fp);
		(void)isc_file_remove(filename);
	}
	if (tsigkey != NULL)
		dns_tsigkey_detach(&tsigkey);
	if (key != NULL)
		dst_key_free(&key);

	return (result);
}

static isc_result_t
configure_session_key(const cfg_obj_t **maps, named_server_t *server,
		      isc_mem_t *mctx)
{
	const char *keyfile, *keynamestr, *algstr;
	unsigned int algtype;
	dns_fixedname_t fname;
	dns_name_t *keyname;
	const dns_name_t *algname;
	isc_buffer_t buffer;
	isc_uint16_t bits;
	const cfg_obj_t *obj;
	isc_boolean_t need_deleteold = ISC_FALSE;
	isc_boolean_t need_createnew = ISC_FALSE;
	isc_result_t result;

	obj = NULL;
	result = named_config_get(maps, "session-keyfile", &obj);
	if (result == ISC_R_SUCCESS) {
		if (cfg_obj_isvoid(obj))
			keyfile = NULL; /* disable it */
		else
			keyfile = cfg_obj_asstring(obj);
	} else
		keyfile = named_g_defaultsessionkeyfile;

	obj = NULL;
	result = named_config_get(maps, "session-keyname", &obj);
	INSIST(result == ISC_R_SUCCESS);
	keynamestr = cfg_obj_asstring(obj);
	isc_buffer_constinit(&buffer, keynamestr, strlen(keynamestr));
	isc_buffer_add(&buffer, strlen(keynamestr));
	keyname = dns_fixedname_initname(&fname);
	result = dns_name_fromtext(keyname, &buffer, dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS)
		return (result);

	obj = NULL;
	result = named_config_get(maps, "session-keyalg", &obj);
	INSIST(result == ISC_R_SUCCESS);
	algstr = cfg_obj_asstring(obj);
	algname = NULL;
	result = named_config_getkeyalgorithm2(algstr, &algname, &algtype,
					       &bits);
	if (result != ISC_R_SUCCESS) {
		const char *s = " (keeping current key)";

		cfg_obj_log(obj, named_g_lctx, ISC_LOG_ERROR, "session-keyalg: "
			    "unsupported or unknown algorithm '%s'%s",
			    algstr,
			    server->session_keyfile != NULL ? s : "");
		return (result);
	}

	/* See if we need to (re)generate a new key. */
	if (keyfile == NULL) {
		if (server->session_keyfile != NULL)
			need_deleteold = ISC_TRUE;
	} else if (server->session_keyfile == NULL)
		need_createnew = ISC_TRUE;
	else if (strcmp(keyfile, server->session_keyfile) != 0 ||
		 !dns_name_equal(server->session_keyname, keyname) ||
		 server->session_keyalg != algtype ||
		 server->session_keybits != bits) {
		need_deleteold = ISC_TRUE;
		need_createnew = ISC_TRUE;
	}

	if (need_deleteold) {
		INSIST(server->session_keyfile != NULL);
		INSIST(server->session_keyname != NULL);
		INSIST(server->sessionkey != NULL);

		cleanup_session_key(server, mctx);
	}

	if (need_createnew) {
		INSIST(server->sessionkey == NULL);
		INSIST(server->session_keyfile == NULL);
		INSIST(server->session_keyname == NULL);
		INSIST(server->session_keyalg == DST_ALG_UNKNOWN);
		INSIST(server->session_keybits == 0);

		server->session_keyname = isc_mem_get(mctx, sizeof(dns_name_t));
		if (server->session_keyname == NULL)
			goto cleanup;
		dns_name_init(server->session_keyname, NULL);
		CHECK(dns_name_dup(keyname, mctx, server->session_keyname));

		server->session_keyfile = isc_mem_strdup(mctx, keyfile);
		if (server->session_keyfile == NULL)
			goto cleanup;

		server->session_keyalg = algtype;
		server->session_keybits = bits;

		CHECK(generate_session_key(keyfile, keynamestr, keyname, algstr,
					   algname, algtype, bits, mctx,
					   &server->sessionkey));
	}

	return (result);

  cleanup:
	cleanup_session_key(server, mctx);
	return (result);
}

#ifndef HAVE_LMDB
static isc_result_t
count_newzones(dns_view_t *view, ns_cfgctx_t *nzcfg, int *num_zonesp) {
	isc_result_t result;

	/* The new zone file may not exist. That is OK. */
	if (!isc_file_exists(view->new_zone_file)) {
		*num_zonesp = 0;
		return (ISC_R_SUCCESS);
	}

	/*
	 * In the case of NZF files, we also parse the configuration in
	 * the file at this stage.
	 *
	 * This may be called in multiple views, so we reset
	 * the parser each time.
	 */
	cfg_parser_reset(named_g_addparser);
	result = cfg_parse_file(named_g_addparser, view->new_zone_file,
				&cfg_type_addzoneconf, &nzcfg->nzf_config);
	if (result == ISC_R_SUCCESS) {
		int num_zones;

		num_zones = count_zones(nzcfg->nzf_config);
		isc_log_write(named_g_lctx,
			      NAMED_LOGCATEGORY_GENERAL, NAMED_LOGMODULE_SERVER,
			      ISC_LOG_INFO,
			      "NZF file '%s' contains %d zones",
			      view->new_zone_file, num_zones);
		if (num_zonesp != NULL)
			*num_zonesp = num_zones;
	} else {
		isc_log_write(named_g_lctx,
			      NAMED_LOGCATEGORY_GENERAL, NAMED_LOGMODULE_SERVER,
			      ISC_LOG_ERROR,
			      "Error parsing NZF file '%s': %s",
			      view->new_zone_file,
			      isc_result_totext(result));
	}

	return (result);
}

#else /* HAVE_LMDB */

static isc_result_t
count_newzones(dns_view_t *view, ns_cfgctx_t *nzcfg, int *num_zonesp) {
	isc_result_t result;
	int n;

	UNUSED(nzcfg);

	REQUIRE(num_zonesp != NULL);

	CHECK(migrate_nzf(view));

	isc_log_write(named_g_lctx,
		      NAMED_LOGCATEGORY_GENERAL, NAMED_LOGMODULE_SERVER,
		      ISC_LOG_INFO, "loading NZD zone count from '%s' "
		      "for view '%s'",
		      view->new_zone_db, view->name);

	CHECK(nzd_count(view, &n));

	*num_zonesp = n;

	isc_log_write(named_g_lctx,
		      NAMED_LOGCATEGORY_GENERAL, NAMED_LOGMODULE_SERVER,
		      ISC_LOG_INFO,
		      "NZD database '%s' contains %d zones",
		      view->new_zone_db, n);

 cleanup:
	if (result != ISC_R_SUCCESS)
		*num_zonesp = 0;

	return (ISC_R_SUCCESS);
}

#endif /* HAVE_LMDB */

static isc_result_t
setup_newzones(dns_view_t *view, cfg_obj_t *config, cfg_obj_t *vconfig,
	       cfg_parser_t *conf_parser, cfg_aclconfctx_t *actx,
	       int *num_zones)
{
	isc_result_t result = ISC_R_SUCCESS;
	isc_boolean_t allow = ISC_FALSE;
	ns_cfgctx_t *nzcfg = NULL;
	const cfg_obj_t *maps[4];
	const cfg_obj_t *options = NULL, *voptions = NULL;
	const cfg_obj_t *nz = NULL;
	const cfg_obj_t *nzdir = NULL;
	const char *dir = NULL;
	const cfg_obj_t *obj = NULL;
	int i = 0;
	isc_uint64_t mapsize = 0ULL;

	REQUIRE(config != NULL);

	if (vconfig != NULL)
		voptions = cfg_tuple_get(vconfig, "options");
	if (voptions != NULL)
		maps[i++] = voptions;
	result = cfg_map_get(config, "options", &options);
	if (result == ISC_R_SUCCESS)
		maps[i++] = options;
	maps[i++] = named_g_defaults;
	maps[i] = NULL;

	result = named_config_get(maps, "allow-new-zones", &nz);
	if (result == ISC_R_SUCCESS)
		allow = cfg_obj_asboolean(nz);
	result = named_config_get(maps, "new-zones-directory", &nzdir);
	if (result == ISC_R_SUCCESS) {
		dir = cfg_obj_asstring(nzdir);
		if (dir != NULL) {
			result = isc_file_isdirectory(dir);
		}
		if (result != ISC_R_SUCCESS) {
			isc_log_write(named_g_lctx, DNS_LOGCATEGORY_SECURITY,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "invalid new-zones-directory %s: %s",
				      dir, isc_result_totext(result));
			return (result);
		}
		if (!isc_file_isdirwritable(dir)) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "new-zones-directory '%s' "
				      "is not writable", dir);
			return (ISC_R_NOPERM);
		}

		dns_view_setnewzonedir(view, dir);
	}

#ifdef HAVE_LMDB
	result = named_config_get(maps, "lmdb-mapsize", &obj);
	if (result == ISC_R_SUCCESS && obj != NULL) {
		mapsize = cfg_obj_asuint64(obj);
		if (mapsize < (1ULL << 20)) { /* 1 megabyte */
			cfg_obj_log(obj, named_g_lctx,
				    ISC_LOG_ERROR,
				    "'lmdb-mapsize "
				    "%" ISC_PRINT_QUADFORMAT "d' "
				    "is too small",
				    mapsize);
			return (ISC_R_FAILURE);
		} else if (mapsize > (1ULL << 40)) { /* 1 terabyte */
			cfg_obj_log(obj, named_g_lctx,
				    ISC_LOG_ERROR,
				    "'lmdb-mapsize "
				    "%" ISC_PRINT_QUADFORMAT "d' "
				    "is too large",
				    mapsize);
			return (ISC_R_FAILURE);
		}
	}
#else
	UNUSED(obj);
#endif /* HAVE_LMDB */

	/*
	 * A non-empty catalog-zones statement implies allow-new-zones
	 */
	if (!allow) {
		const cfg_obj_t *cz = NULL;
		result = named_config_get(maps, "catalog-zones", &cz);
		if (result == ISC_R_SUCCESS) {
			const cfg_listelt_t *e =
				cfg_list_first(cfg_tuple_get(cz, "zone list"));
			if (e != NULL)
				allow = ISC_TRUE;
		}
	}

	if (!allow) {
		dns_view_setnewzones(view, ISC_FALSE, NULL, NULL, 0ULL);
		if (num_zones != NULL)
			*num_zones = 0;
		return (ISC_R_SUCCESS);
	}

	nzcfg = isc_mem_get(view->mctx, sizeof(*nzcfg));
	if (nzcfg == NULL) {
		dns_view_setnewzones(view, ISC_FALSE, NULL, NULL, 0ULL);
		return (ISC_R_NOMEMORY);
	}

	/*
	 * We attach the parser that was used for config as well
	 * as the one that will be used for added zones, to avoid
	 * a shutdown race later.
	 */
	memset(nzcfg, 0, sizeof(*nzcfg));
	cfg_parser_attach(conf_parser, &nzcfg->conf_parser);
	cfg_parser_attach(named_g_addparser, &nzcfg->add_parser);
	isc_mem_attach(view->mctx, &nzcfg->mctx);
	cfg_aclconfctx_attach(actx, &nzcfg->actx);

	result = dns_view_setnewzones(view, ISC_TRUE, nzcfg,
				      newzone_cfgctx_destroy, mapsize);
	if (result != ISC_R_SUCCESS) {
		dns_view_setnewzones(view, ISC_FALSE, NULL, NULL, 0ULL);
		return (result);
	}

	cfg_obj_attach(config, &nzcfg->config);
	if (vconfig != NULL)
		cfg_obj_attach(vconfig, &nzcfg->vconfig);

	result = count_newzones(view, nzcfg, num_zones);
	return (result);
}

static void
configure_zone_setviewcommit(isc_result_t result, const cfg_obj_t *zconfig,
			     dns_view_t *view)
{
	const char *zname;
	dns_fixedname_t fixorigin;
	dns_name_t *origin;
	isc_result_t result2;
	dns_view_t *pview = NULL;
	dns_zone_t *zone = NULL;
	dns_zone_t *raw = NULL;

	zname = cfg_obj_asstring(cfg_tuple_get(zconfig, "name"));
	origin = dns_fixedname_initname(&fixorigin);

	result2 = dns_name_fromstring(origin, zname, 0, NULL);
	if (result2 != ISC_R_SUCCESS) {
		return;
	}

	result2 = dns_viewlist_find(&named_g_server->viewlist, view->name,
				    view->rdclass, &pview);
	if (result2 != ISC_R_SUCCESS) {
		return;
	}

	result2 = dns_view_findzone(pview, origin, &zone);
	if (result2 != ISC_R_SUCCESS) {
		dns_view_detach(&pview);
		return;
	}

	dns_zone_getraw(zone, &raw);

	if (result == ISC_R_SUCCESS) {
		dns_zone_setviewcommit(zone);
		if (raw != NULL)
			dns_zone_setviewcommit(raw);
	} else {
		dns_zone_setviewrevert(zone);
		if (raw != NULL)
			dns_zone_setviewrevert(raw);
	}

	if (raw != NULL) {
		dns_zone_detach(&raw);
	}

	dns_zone_detach(&zone);
	dns_view_detach(&pview);
}

#ifndef HAVE_LMDB

static isc_result_t
configure_newzones(dns_view_t *view, cfg_obj_t *config, cfg_obj_t *vconfig,
		   isc_mem_t *mctx, cfg_aclconfctx_t *actx)
{
	isc_result_t result;
	ns_cfgctx_t *nzctx;
	const cfg_obj_t *zonelist;
	const cfg_listelt_t *element;

	nzctx = view->new_zone_config;
	if (nzctx == NULL || nzctx->nzf_config == NULL) {
		return (ISC_R_SUCCESS);
	}

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "loading additional zones for view '%s'",
		      view->name);

	zonelist = NULL;
	cfg_map_get(nzctx->nzf_config, "zone", &zonelist);

	for (element = cfg_list_first(zonelist);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *zconfig = cfg_listelt_value(element);
		CHECK(configure_zone(config, zconfig, vconfig, mctx,
				     view, &named_g_server->viewlist, actx,
				     ISC_TRUE, ISC_FALSE, ISC_FALSE));
	}

	result = ISC_R_SUCCESS;

 cleanup:
	for (element = cfg_list_first(zonelist);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *zconfig = cfg_listelt_value(element);
		configure_zone_setviewcommit(result, zconfig, view);
	}

	return (result);
}

#else /* HAVE_LMDB */

static isc_result_t
data_to_cfg(dns_view_t *view, MDB_val *key, MDB_val *data,
	   isc_buffer_t **text, cfg_obj_t **zoneconfig)
{
	isc_result_t result;
	const char *zone_name;
	size_t zone_name_len;
	const char *zone_config;
	size_t zone_config_len;
	cfg_obj_t *zoneconf = NULL;

	REQUIRE(view != NULL);
	REQUIRE(key != NULL);
	REQUIRE(data != NULL);
	REQUIRE(text != NULL);
	REQUIRE(zoneconfig != NULL && *zoneconfig == NULL);

	if (*text == NULL) {
		result = isc_buffer_allocate(view->mctx, text, 256);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
	} else {
		isc_buffer_clear(*text);
	}

	zone_name = (const char *) key->mv_data;
	zone_name_len = key->mv_size;
	INSIST(zone_name != NULL && zone_name_len > 0);

	zone_config = (const char *) data->mv_data;
	zone_config_len = data->mv_size;
	INSIST(zone_config != NULL && zone_config_len > 0);

	/* zone zonename { config; }; */
	result = isc_buffer_reserve(text, 5 + zone_name_len + 1 +
				    zone_config_len + 2);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	putstr(text, "zone ");
	putmem(text, (const void *) zone_name, zone_name_len);
	putstr(text, " ");
	putmem(text, (const void *) zone_config, zone_config_len);
	putstr(text, ";\n");

	cfg_parser_reset(named_g_addparser);
	result = cfg_parse_buffer3(named_g_addparser, *text, zone_name, 0,
				   &cfg_type_addzoneconf, &zoneconf);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "parsing config for zone '%.*s' in "
			      "NZD database '%s' failed",
			      (int) zone_name_len, zone_name,
			      view->new_zone_db);
		goto cleanup;
	}

	*zoneconfig = zoneconf;
	zoneconf = NULL;
	result = ISC_R_SUCCESS;

 cleanup:
	if (zoneconf != NULL) {
		cfg_obj_destroy(named_g_addparser, &zoneconf);
	}

	return (result);
}

/*%
 * Prototype for a callback which can be used with for_all_newzone_cfgs().
 */
typedef isc_result_t (*newzone_cfg_cb_t)(const cfg_obj_t *zconfig,
					 cfg_obj_t *config, cfg_obj_t *vconfig,
					 isc_mem_t *mctx, dns_view_t *view,
					 cfg_aclconfctx_t *actx);

/*%
 * For each zone found in a NZD opened by the caller, create an object
 * representing its configuration and invoke "callback" with the created
 * object, "config", "vconfig", "mctx", "view" and "actx" as arguments (all
 * these are non-global variables required to invoke configure_zone()).
 * Immediately interrupt processing if an error is encountered while
 * transforming NZD data into a zone configuration object or if "callback"
 * returns an error.
 */
static isc_result_t
for_all_newzone_cfgs(newzone_cfg_cb_t callback, cfg_obj_t *config,
		     cfg_obj_t *vconfig, isc_mem_t *mctx, dns_view_t *view,
		     cfg_aclconfctx_t *actx, MDB_txn *txn, MDB_dbi dbi)
{
	const cfg_obj_t *zconfig, *zlist;
	isc_result_t result = ISC_R_SUCCESS;
	cfg_obj_t *zconfigobj = NULL;
	isc_buffer_t *text = NULL;
	MDB_cursor *cursor = NULL;
	MDB_val data, key;
	int status;

	status = mdb_cursor_open(txn, dbi, &cursor);
	if (status != MDB_SUCCESS) {
		return (ISC_R_FAILURE);
	}

	for (status = mdb_cursor_get(cursor, &key, &data, MDB_FIRST);
	     status == MDB_SUCCESS;
	     status = mdb_cursor_get(cursor, &key, &data, MDB_NEXT))
	{
		/*
		 * Create a configuration object from data fetched from NZD.
		 */
		result = data_to_cfg(view, &key, &data, &text, &zconfigobj);
		if (result != ISC_R_SUCCESS) {
			break;
		}

		/*
		 * Extract zone configuration from configuration object.
		 */
		zlist = NULL;
		result = cfg_map_get(zconfigobj, "zone", &zlist);
		if (result != ISC_R_SUCCESS) {
			break;
		} else if (!cfg_obj_islist(zlist)) {
			result = ISC_R_FAILURE;
			break;
		}
		zconfig = cfg_listelt_value(cfg_list_first(zlist));

		/*
		 * Invoke callback.
		 */
		result = callback(zconfig, config, vconfig, mctx, view, actx);
		if (result != ISC_R_SUCCESS) {
			break;
		}

		/*
		 * Destroy the configuration object created in this iteration.
		 */
		cfg_obj_destroy(named_g_addparser, &zconfigobj);
	}

	if (text != NULL) {
		isc_buffer_free(&text);
	}
	if (zconfigobj != NULL) {
		cfg_obj_destroy(named_g_addparser, &zconfigobj);
	}
	mdb_cursor_close(cursor);

	return (result);
}

/*%
 * Attempt to configure a zone found in NZD and return the result.
 */
static isc_result_t
configure_newzone(const cfg_obj_t *zconfig, cfg_obj_t *config,
		  cfg_obj_t *vconfig, isc_mem_t *mctx, dns_view_t *view,
		  cfg_aclconfctx_t *actx)
{
	return (configure_zone(config, zconfig, vconfig, mctx, view,
			       &named_g_server->viewlist, actx, ISC_TRUE,
			       ISC_FALSE, ISC_FALSE));
}

/*%
 * Revert new view assignment for a zone found in NZD.
 */
static isc_result_t
configure_newzone_revert(const cfg_obj_t *zconfig, cfg_obj_t *config,
			 cfg_obj_t *vconfig, isc_mem_t *mctx, dns_view_t *view,
			 cfg_aclconfctx_t *actx)
{
	UNUSED(config);
	UNUSED(vconfig);
	UNUSED(mctx);
	UNUSED(actx);

	configure_zone_setviewcommit(ISC_R_FAILURE, zconfig, view);

	return (ISC_R_SUCCESS);
}

static isc_result_t
configure_newzones(dns_view_t *view, cfg_obj_t *config, cfg_obj_t *vconfig,
		   isc_mem_t *mctx, cfg_aclconfctx_t *actx)
{
	isc_result_t result;
	MDB_txn *txn = NULL;
	MDB_dbi dbi;

	if (view->new_zone_config == NULL) {
		return (ISC_R_SUCCESS);
	}

	result = nzd_open(view, MDB_RDONLY, &txn, &dbi);
	if (result != ISC_R_SUCCESS) {
		return (ISC_R_SUCCESS);
	}

	isc_log_write(named_g_lctx,
		      NAMED_LOGCATEGORY_GENERAL, NAMED_LOGMODULE_SERVER,
		      ISC_LOG_INFO, "loading NZD configs from '%s' "
		      "for view '%s'",
		      view->new_zone_db, view->name);

	result = for_all_newzone_cfgs(configure_newzone, config, vconfig, mctx,
				      view, actx, txn, dbi);
	if (result != ISC_R_SUCCESS) {
		/*
		 * An error was encountered while attempting to configure zones
		 * found in NZD.  As this error may have been caused by a
		 * configure_zone() failure, try restoring a sane configuration
		 * by reattaching all zones found in NZD to the old view.  If
		 * this also fails, too bad, there is nothing more we can do in
		 * terms of trying to make things right.
		 */
		(void) for_all_newzone_cfgs(configure_newzone_revert, config,
					    vconfig, mctx, view, actx, txn,
					    dbi);
	}

	(void) nzd_close(&txn, ISC_FALSE);
	return (result);
}

static isc_result_t
get_newzone_config(dns_view_t *view, const char *zonename,
		   cfg_obj_t **zoneconfig)
{
	isc_result_t result;
	int status;
	cfg_obj_t *zoneconf = NULL;
	isc_buffer_t *text = NULL;
	MDB_txn *txn = NULL;
	MDB_dbi dbi;
	MDB_val key, data;
	char zname[DNS_NAME_FORMATSIZE];
	dns_fixedname_t fname;
	dns_name_t *name;
	isc_buffer_t b;

	INSIST(zoneconfig != NULL && *zoneconfig == NULL);

	CHECK(nzd_open(view, MDB_RDONLY, &txn, &dbi));

	isc_log_write(named_g_lctx,
		      NAMED_LOGCATEGORY_GENERAL, NAMED_LOGMODULE_SERVER,
		      ISC_LOG_INFO, "loading NZD config from '%s' "
		      "for zone '%s'",
		      view->new_zone_db, zonename);

	/* Normalize zone name */
	isc_buffer_constinit(&b, zonename, strlen(zonename));
	isc_buffer_add(&b, strlen(zonename));
	name = dns_fixedname_initname(&fname);
	CHECK(dns_name_fromtext(name, &b, dns_rootname,
				DNS_NAME_DOWNCASE, NULL));
	dns_name_format(name, zname, sizeof(zname));

	key.mv_data = zname;
	key.mv_size = strlen(zname);

	status = mdb_get(txn, dbi, &key, &data);
	if (status != MDB_SUCCESS) {
		CHECK(ISC_R_FAILURE);
	}

	CHECK(data_to_cfg(view, &key, &data, &text, &zoneconf));

	*zoneconfig = zoneconf;
	zoneconf = NULL;
	result = ISC_R_SUCCESS;

 cleanup:
	(void) nzd_close(&txn, ISC_FALSE);

	if (zoneconf != NULL) {
		cfg_obj_destroy(named_g_addparser, &zoneconf);
	}
	if (text != NULL) {
		isc_buffer_free(&text);
	}

	return (result);
}

#endif /* HAVE_LMDB */

static int
count_zones(const cfg_obj_t *conf) {
	const cfg_obj_t *zonelist = NULL;
	const cfg_listelt_t *element;
	int n = 0;

	REQUIRE(conf != NULL);

	cfg_map_get(conf, "zone", &zonelist);
	for (element = cfg_list_first(zonelist);
	     element != NULL;
	     element = cfg_list_next(element))
		n++;

	return (n);
}

static isc_result_t
check_lockfile(named_server_t *server, const cfg_obj_t *config,
	       isc_boolean_t first_time)
{
	isc_result_t result;
	const char *filename = NULL;
	const cfg_obj_t *maps[3];
	const cfg_obj_t *options;
	const cfg_obj_t *obj;
	int i;

	i = 0;
	options = NULL;
	result = cfg_map_get(config, "options", &options);
	if (result == ISC_R_SUCCESS)
		maps[i++] = options;
	maps[i++] = named_g_defaults;
	maps[i] = NULL;

	obj = NULL;
	(void) named_config_get(maps, "lock-file", &obj);

	if (!first_time) {
		if (obj != NULL && !cfg_obj_isstring(obj) &&
		    server->lockfile != NULL &&
		    strcmp(cfg_obj_asstring(obj), server->lockfile) != 0)
			isc_log_write(named_g_lctx,
				      NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER,
				      ISC_LOG_WARNING,
				      "changing 'lock-file' "
				      "has no effect until the "
				      "server is restarted");

		return (ISC_R_SUCCESS);
	}

	if (obj != NULL) {
		if (cfg_obj_isvoid(obj)) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_DEBUG(1),
				      "skipping lock-file check ");
			return (ISC_R_SUCCESS);
		} else if (named_g_forcelock) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
				      "'lock-file' has no effect "
				      "because the server was run with -X");
			server->lockfile = isc_mem_strdup(server->mctx,
						  named_g_defaultlockfile);
		} else {
			filename = cfg_obj_asstring(obj);
			server->lockfile = isc_mem_strdup(server->mctx,
							  filename);
		}

		if (server->lockfile == NULL)
			return (ISC_R_NOMEMORY);
	}

	if (named_g_forcelock && named_g_defaultlockfile != NULL) {
		INSIST(server->lockfile == NULL);
		server->lockfile = isc_mem_strdup(server->mctx,
						  named_g_defaultlockfile);
	}

	if (server->lockfile == NULL)
		return (ISC_R_SUCCESS);

	if (named_os_issingleton(server->lockfile))
		return (ISC_R_SUCCESS);

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "could not lock %s; another named "
		      "process may be running", server->lockfile);
	return (ISC_R_FAILURE);
}

static isc_result_t
load_configuration(const char *filename, named_server_t *server,
		   isc_boolean_t first_time)
{
	cfg_obj_t *config = NULL, *bindkeys = NULL;
	cfg_parser_t *conf_parser = NULL, *bindkeys_parser = NULL;
	const cfg_listelt_t *element;
	const cfg_obj_t *builtin_views;
	const cfg_obj_t *maps[3];
	const cfg_obj_t *obj;
	const cfg_obj_t *options;
	const cfg_obj_t *usev4ports, *avoidv4ports, *usev6ports, *avoidv6ports;
	const cfg_obj_t *views;
	dns_view_t *view = NULL;
	dns_view_t *view_next;
	dns_viewlist_t tmpviewlist;
	dns_viewlist_t viewlist, builtin_viewlist;
	in_port_t listen_port, udpport_low, udpport_high;
	int i, backlog;
	int num_zones = 0;
	isc_boolean_t exclusive = ISC_FALSE;
	isc_interval_t interval;
	isc_logconfig_t *logc = NULL;
	isc_portset_t *v4portset = NULL;
	isc_portset_t *v6portset = NULL;
	isc_resourcevalue_t nfiles;
	isc_result_t result, tresult;
	isc_uint32_t heartbeat_interval;
	isc_uint32_t interface_interval;
	isc_uint32_t reserved;
	isc_uint32_t udpsize;
	isc_uint32_t transfer_message_size;
	named_cache_t *nsc;
	named_cachelist_t cachelist, tmpcachelist;
	ns_altsecret_t *altsecret;
	ns_altsecretlist_t altsecrets, tmpaltsecrets;
	unsigned int maxsocks;
	isc_uint32_t softquota = 0;
	unsigned int initial, idle, keepalive, advertised;
	dns_aclenv_t *env =
		ns_interfacemgr_getaclenv(named_g_server->interfacemgr);

	ISC_LIST_INIT(viewlist);
	ISC_LIST_INIT(builtin_viewlist);
	ISC_LIST_INIT(cachelist);
	ISC_LIST_INIT(altsecrets);

	/* Create the ACL configuration context */
	if (named_g_aclconfctx != NULL) {
		cfg_aclconfctx_detach(&named_g_aclconfctx);
	}
	CHECK(cfg_aclconfctx_create(named_g_mctx, &named_g_aclconfctx));

	/*
	 * Shut down all dyndb instances.
	 */
	dns_dyndb_cleanup(ISC_FALSE);

	/*
	 * Parse the global default pseudo-config file.
	 */
	if (first_time) {
		result = named_config_parsedefaults(named_g_parser,
						    &named_g_config);
		if (result != ISC_R_SUCCESS) {
			named_main_earlyfatal("unable to load "
					      "internal defaults: %s",
					      isc_result_totext(result));
		}
		RUNTIME_CHECK(cfg_map_get(named_g_config, "options",
					  &named_g_defaults) == ISC_R_SUCCESS);
	}

	/*
	 * Parse the configuration file using the new config code.
	 */
	config = NULL;
	isc_log_write(named_g_lctx,
		      NAMED_LOGCATEGORY_GENERAL, NAMED_LOGMODULE_SERVER,
		      ISC_LOG_INFO, "loading configuration from '%s'",
		      filename);
	CHECK(cfg_parser_create(named_g_mctx, named_g_lctx,
				&conf_parser));
	cfg_parser_setcallback(conf_parser, directory_callback, NULL);
	result = cfg_parse_file(conf_parser, filename,
				&cfg_type_namedconf, &config);

	CHECK(result);

	/*
	 * Check the validity of the configuration.
	 */
	CHECK(bind9_check_namedconf(config, named_g_lctx, named_g_mctx));

	/*
	 * Fill in the maps array, used for resolving defaults.
	 */
	i = 0;
	options = NULL;
	result = cfg_map_get(config, "options", &options);
	if (result == ISC_R_SUCCESS) {
		maps[i++] = options;
	}
	maps[i++] = named_g_defaults;
	maps[i] = NULL;

	/*
	 * If bind.keys exists, load it.  If "dnssec-validation auto"
	 * is turned on, the root key found there will be used as a
	 * default trust anchor.
	 */
	obj = NULL;
	result = named_config_get(maps, "bindkeys-file", &obj);
	INSIST(result == ISC_R_SUCCESS);
	CHECKM(setstring(server, &server->bindkeysfile,
	       cfg_obj_asstring(obj)), "strdup");

	if (access(server->bindkeysfile, R_OK) == 0) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "reading built-in trust anchors "
			      "from file '%s'", server->bindkeysfile);

		CHECK(cfg_parser_create(named_g_mctx, named_g_lctx,
					&bindkeys_parser));

		result = cfg_parse_file(bindkeys_parser, server->bindkeysfile,
					&cfg_type_bindkeys, &bindkeys);
		CHECK(result);
	} else {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "unable to open '%s'; using built-in keys "
			      "instead", server->bindkeysfile);
	}

	/* Ensure exclusive access to configuration data. */
	if (!exclusive) {
		result = isc_task_beginexclusive(server->task);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		exclusive = ISC_TRUE;
	}

	/*
	 * Set process limits, which (usually) needs to be done as root.
	 */
	set_limits(maps);

	/*
	 * Check the process lockfile.
	 */
	CHECK(check_lockfile(server, config, first_time));

	/*
	 * Check if max number of open sockets that the system allows is
	 * sufficiently large.	Failing this condition is not necessarily fatal,
	 * but may cause subsequent runtime failures for a busy recursive
	 * server.
	 */
	result = isc_socketmgr_getmaxsockets(named_g_socketmgr, &maxsocks);
	if (result != ISC_R_SUCCESS) {
		maxsocks = 0;
	}
	result = isc_resource_getcurlimit(isc_resource_openfiles, &nfiles);
	if (result == ISC_R_SUCCESS && (isc_resourcevalue_t)maxsocks > nfiles) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
			      "max open files (%" ISC_PRINT_QUADFORMAT "u)"
			      " is smaller than max sockets (%u)",
			      nfiles, maxsocks);
	}

	/*
	 * Set the number of socket reserved for TCP, stdio etc.
	 */
	obj = NULL;
	result = named_config_get(maps, "reserved-sockets", &obj);
	INSIST(result == ISC_R_SUCCESS);
	reserved = cfg_obj_asuint32(obj);
	if (maxsocks != 0) {
		if (maxsocks < 128U) {			/* Prevent underflow. */
			reserved = 0;
		} else if (reserved > maxsocks - 128U) { /* Minimum UDP space. */
			reserved = maxsocks - 128;
		}
	}
	/* Minimum TCP/stdio space. */
	if (reserved < 128U) {
		reserved = 128;
	}
	if (reserved + 128U > maxsocks && maxsocks != 0) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
			      "less than 128 UDP sockets available after "
			      "applying 'reserved-sockets' and 'maxsockets'");
	}
	isc__socketmgr_setreserved(named_g_socketmgr, reserved);

#ifdef HAVE_GEOIP
	/*
	 * Initialize GeoIP databases from the configured location.
	 * This should happen before configuring any ACLs, so that we
	 * know what databases are available and can reject any GeoIP
	 * ACLs that can't work.
	 */
	obj = NULL;
	result = named_config_get(maps, "geoip-directory", &obj);
	if (result == ISC_R_SUCCESS && cfg_obj_isstring(obj)) {
		char *dir;
		DE_CONST(cfg_obj_asstring(obj), dir);
		named_geoip_load(dir);
	} else {
		named_geoip_load(NULL);
	}
	named_g_aclconfctx->geoip = named_g_geoip;

	obj = NULL;
	result = named_config_get(maps, "geoip-use-ecs", &obj);
	INSIST(result == ISC_R_SUCCESS);
	env->geoip_use_ecs = cfg_obj_asboolean(obj);
#endif /* HAVE_GEOIP */

	/*
	 * Configure various server options.
	 */
	configure_server_quota(maps, "transfers-out",
			       &server->sctx->xfroutquota);
	configure_server_quota(maps, "tcp-clients", &server->sctx->tcpquota);
	configure_server_quota(maps, "recursive-clients",
			       &server->sctx->recursionquota);

	if (server->sctx->recursionquota.max > 1000) {
		int margin = ISC_MAX(100, named_g_cpus + 1);
		if (margin > server->sctx->recursionquota.max - 100) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "'recursive-clients %d' too low when "
				      "running with %d worker threads",
				      server->sctx->recursionquota.max,
				      named_g_cpus);
			CHECK(ISC_R_RANGE);
		}
		softquota = server->sctx->recursionquota.max - margin;
	} else {
		softquota = (server->sctx->recursionquota.max * 90) / 100;
	}

	isc_quota_soft(&server->sctx->recursionquota, softquota);

	/*
	 * Set "blackhole". Only legal at options level; there is
	 * no default.
	 */
	CHECK(configure_view_acl(NULL, config, NULL, "blackhole", NULL,
				 named_g_aclconfctx, named_g_mctx,
				 &server->sctx->blackholeacl));
	if (server->sctx->blackholeacl != NULL) {
		dns_dispatchmgr_setblackhole(named_g_dispatchmgr,
					     server->sctx->blackholeacl);
	}

	/*
	 * Set "keep-response-order". Only legal at options or
	 * global defaults level.
	 */
	CHECK(configure_view_acl(NULL, config, named_g_config,
				 "keep-response-order", NULL,
				 named_g_aclconfctx, named_g_mctx,
				 &server->sctx->keepresporder));

	obj = NULL;
	result = named_config_get(maps, "match-mapped-addresses", &obj);
	INSIST(result == ISC_R_SUCCESS);
	env->match_mapped = cfg_obj_asboolean(obj);

	CHECKM(named_statschannels_configure(named_g_server, config,
					     named_g_aclconfctx),
	       "configuring statistics server(s)");

	obj = NULL;
	result = named_config_get(maps, "tcp-initial-timeout", &obj);
	INSIST(result == ISC_R_SUCCESS);
	initial = cfg_obj_asuint32(obj);
	if (initial > 1200) {
		cfg_obj_log(obj, named_g_lctx, ISC_LOG_WARNING,
			    "tcp-initial-timeout value is out of range: "
			    "lowering to 1200");
		initial = 1200;
	} else if (initial < 25) {
		cfg_obj_log(obj, named_g_lctx, ISC_LOG_WARNING,
			    "tcp-initial-timeout value is out of range: "
			    "raising to 25");
		initial = 25;
	}

	obj = NULL;
	result = named_config_get(maps, "tcp-idle-timeout", &obj);
	INSIST(result == ISC_R_SUCCESS);
	idle = cfg_obj_asuint32(obj);
	if (idle > 1200) {
		cfg_obj_log(obj, named_g_lctx, ISC_LOG_WARNING,
			    "tcp-idle-timeout value is out of range: "
			    "lowering to 1200");
		idle = 1200;
	} else if (idle < 1) {
		cfg_obj_log(obj, named_g_lctx, ISC_LOG_WARNING,
			    "tcp-idle-timeout value is out of range: "
			    "raising to 1");
		idle = 1;
	}

	obj = NULL;
	result = named_config_get(maps, "tcp-keepalive-timeout", &obj);
	INSIST(result == ISC_R_SUCCESS);
	keepalive = cfg_obj_asuint32(obj);
	if (keepalive > MAX_TCP_TIMEOUT) {
		cfg_obj_log(obj, named_g_lctx, ISC_LOG_WARNING,
			    "tcp-keepalive-timeout value is out of range: "
			    "lowering to %u", MAX_TCP_TIMEOUT);
		keepalive = MAX_TCP_TIMEOUT;
	} else if (keepalive < 1) {
		cfg_obj_log(obj, named_g_lctx, ISC_LOG_WARNING,
			    "tcp-keepalive-timeout value is out of range: "
			    "raising to 1");
		keepalive = 1;
	}

	obj = NULL;
	result = named_config_get(maps, "tcp-advertised-timeout", &obj);
	INSIST(result == ISC_R_SUCCESS);
	advertised = cfg_obj_asuint32(obj);
	if (advertised > MAX_TCP_TIMEOUT) {
		cfg_obj_log(obj, named_g_lctx, ISC_LOG_WARNING,
			    "tcp-advertized-timeout value is out of range: "
			    "lowering to %u", MAX_TCP_TIMEOUT);
		advertised = MAX_TCP_TIMEOUT;
	}

	ns_server_settimeouts(named_g_server->sctx,
			      initial, idle, keepalive, advertised);

	/*
	 * Configure sets of UDP query source ports.
	 */
	CHECKM(isc_portset_create(named_g_mctx, &v4portset),
	       "creating UDP port set");
	CHECKM(isc_portset_create(named_g_mctx, &v6portset),
	       "creating UDP port set");

	usev4ports = NULL;
	usev6ports = NULL;
	avoidv4ports = NULL;
	avoidv6ports = NULL;

	(void)named_config_get(maps, "use-v4-udp-ports", &usev4ports);
	if (usev4ports != NULL) {
		portset_fromconf(v4portset, usev4ports, ISC_TRUE);
	} else {
		CHECKM(isc_net_getudpportrange(AF_INET, &udpport_low,
					       &udpport_high),
		       "get the default UDP/IPv4 port range");
		if (udpport_low == udpport_high) {
			isc_portset_add(v4portset, udpport_low);
		} else {
			isc_portset_addrange(v4portset, udpport_low,
					     udpport_high);
		}
		if (!ns_server_getoption(server->sctx, NS_SERVER_DISABLE4)) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
				      "using default UDP/IPv4 port range: "
				      "[%d, %d]", udpport_low, udpport_high);
		}
	}
	(void)named_config_get(maps, "avoid-v4-udp-ports", &avoidv4ports);
	if (avoidv4ports != NULL) {
		portset_fromconf(v4portset, avoidv4ports, ISC_FALSE);
	}

	(void)named_config_get(maps, "use-v6-udp-ports", &usev6ports);
	if (usev6ports != NULL) {
		portset_fromconf(v6portset, usev6ports, ISC_TRUE);
	} else {
		CHECKM(isc_net_getudpportrange(AF_INET6, &udpport_low,
					       &udpport_high),
		       "get the default UDP/IPv6 port range");
		if (udpport_low == udpport_high) {
			isc_portset_add(v6portset, udpport_low);
		} else {
			isc_portset_addrange(v6portset, udpport_low,
					     udpport_high);
		}
		if (!ns_server_getoption(server->sctx, NS_SERVER_DISABLE6)) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
				      "using default UDP/IPv6 port range: "
				      "[%d, %d]", udpport_low, udpport_high);
		}
	}
	(void)named_config_get(maps, "avoid-v6-udp-ports", &avoidv6ports);
	if (avoidv6ports != NULL) {
		portset_fromconf(v6portset, avoidv6ports, ISC_FALSE);
	}

	dns_dispatchmgr_setavailports(named_g_dispatchmgr, v4portset,
				      v6portset);

	/*
	 * Set the EDNS UDP size when we don't match a view.
	 */
	obj = NULL;
	result = named_config_get(maps, "edns-udp-size", &obj);
	INSIST(result == ISC_R_SUCCESS);
	udpsize = cfg_obj_asuint32(obj);
	if (udpsize < 512) {
		udpsize = 512;
	}
	if (udpsize > 4096) {
		udpsize = 4096;
	}
	server->sctx->udpsize = (isc_uint16_t)udpsize;

	/* Set the transfer message size for TCP */
	obj = NULL;
	result = named_config_get(maps, "transfer-message-size", &obj);
	INSIST(result == ISC_R_SUCCESS);
	transfer_message_size = cfg_obj_asuint32(obj);
	if (transfer_message_size < 512) {
		transfer_message_size = 512;
	} else if (transfer_message_size > 65535) {
		transfer_message_size = 65535;
	}
	server->sctx->transfer_tcp_message_size =
		(isc_uint16_t) transfer_message_size;

	/*
	 * Configure the zone manager.
	 */
	obj = NULL;
	result = named_config_get(maps, "transfers-in", &obj);
	INSIST(result == ISC_R_SUCCESS);
	dns_zonemgr_settransfersin(server->zonemgr, cfg_obj_asuint32(obj));

	obj = NULL;
	result = named_config_get(maps, "transfers-per-ns", &obj);
	INSIST(result == ISC_R_SUCCESS);
	dns_zonemgr_settransfersperns(server->zonemgr, cfg_obj_asuint32(obj));

	obj = NULL;
	result = named_config_get(maps, "notify-rate", &obj);
	INSIST(result == ISC_R_SUCCESS);
	dns_zonemgr_setnotifyrate(server->zonemgr, cfg_obj_asuint32(obj));

	obj = NULL;
	result = named_config_get(maps, "startup-notify-rate", &obj);
	INSIST(result == ISC_R_SUCCESS);
	dns_zonemgr_setstartupnotifyrate(server->zonemgr,
					 cfg_obj_asuint32(obj));

	obj = NULL;
	result = named_config_get(maps, "serial-query-rate", &obj);
	INSIST(result == ISC_R_SUCCESS);
	dns_zonemgr_setserialqueryrate(server->zonemgr, cfg_obj_asuint32(obj));

	/*
	 * Determine which port to use for listening for incoming connections.
	 */
	if (named_g_port != 0) {
		listen_port = named_g_port;
	} else {
		CHECKM(named_config_getport(config, &listen_port), "port");
	}

	/*
	 * Determing the default DSCP code point.
	 */
	CHECKM(named_config_getdscp(config, &named_g_dscp), "dscp");

	/*
	 * Find the listen queue depth.
	 */
	obj = NULL;
	result = named_config_get(maps, "tcp-listen-queue", &obj);
	INSIST(result == ISC_R_SUCCESS);
	backlog = cfg_obj_asuint32(obj);
	if ((backlog > 0) && (backlog < 10)) {
		backlog = 10;
	}
	ns_interfacemgr_setbacklog(server->interfacemgr, backlog);

	/*
	 * Configure the interface manager according to the "listen-on"
	 * statement.
	 */
	{
		const cfg_obj_t *clistenon = NULL;
		ns_listenlist_t *listenon = NULL;

		clistenon = NULL;
		/*
		 * Even though listen-on is present in the default
		 * configuration, this way is easier.
		 */
		if (options != NULL) {
			(void)cfg_map_get(options, "listen-on", &clistenon);
		}
		if (clistenon != NULL) {
			/* check return code? */
			(void)ns_listenlist_fromconfig(clistenon, config,
						       named_g_aclconfctx,
						       named_g_mctx, AF_INET,
						       &listenon);
		} else {
			/*
			 * Not specified, use default.
			 */
			CHECK(ns_listenlist_default(named_g_mctx, listen_port,
						    -1, ISC_TRUE, &listenon));
		}
		if (listenon != NULL) {
			ns_interfacemgr_setlistenon4(server->interfacemgr,
						     listenon);
			ns_listenlist_detach(&listenon);
		}
	}
	/*
	 * Ditto for IPv6.
	 */
	{
		const cfg_obj_t *clistenon = NULL;
		ns_listenlist_t *listenon = NULL;

		if (options != NULL) {
			(void)cfg_map_get(options, "listen-on-v6", &clistenon);
		}
		if (clistenon != NULL) {
			/* check return code? */
			(void)ns_listenlist_fromconfig(clistenon, config,
						       named_g_aclconfctx,
						       named_g_mctx, AF_INET6,
						       &listenon);
		} else {
			/*
			 * Not specified, use default.
			 */
			CHECK(ns_listenlist_default(named_g_mctx, listen_port,
						    -1, ISC_TRUE, &listenon));
		}
		if (listenon != NULL) {
			ns_interfacemgr_setlistenon6(server->interfacemgr,
						     listenon);
			ns_listenlist_detach(&listenon);
		}
	}

	/*
	 * Rescan the interface list to pick up changes in the
	 * listen-on option.  It's important that we do this before we try
	 * to configure the query source, since the dispatcher we use might
	 * be shared with an interface.
	 */
	result = ns_interfacemgr_scan(server->interfacemgr, ISC_TRUE);

	/*
	 * Check that named is able to TCP listen on at least one
	 * interface. Otherwise, another named process could be running
	 * and we should fail.
	 */
	if (first_time && (result == ISC_R_ADDRINUSE)) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "unable to listen on any configured interfaces");
		result = ISC_R_FAILURE;
		goto cleanup;
	}

	/*
	 * Arrange for further interface scanning to occur periodically
	 * as specified by the "interface-interval" option.
	 */
	obj = NULL;
	result = named_config_get(maps, "interface-interval", &obj);
	INSIST(result == ISC_R_SUCCESS);
	interface_interval = cfg_obj_asuint32(obj) * 60;
	if (interface_interval == 0) {
		CHECK(isc_timer_reset(server->interface_timer,
				      isc_timertype_inactive,
				      NULL, NULL, ISC_TRUE));
	} else if (server->interface_interval != interface_interval) {
		isc_interval_set(&interval, interface_interval, 0);
		CHECK(isc_timer_reset(server->interface_timer,
				      isc_timertype_ticker,
				      NULL, &interval, ISC_FALSE));
	}
	server->interface_interval = interface_interval;

	/*
	 * Enable automatic interface scans.
	 */
	obj = NULL;
	result = named_config_get(maps, "automatic-interface-scan", &obj);
	INSIST(result == ISC_R_SUCCESS);
	server->sctx->interface_auto = cfg_obj_asboolean(obj);

	/*
	 * Configure the dialup heartbeat timer.
	 */
	obj = NULL;
	result = named_config_get(maps, "heartbeat-interval", &obj);
	INSIST(result == ISC_R_SUCCESS);
	heartbeat_interval = cfg_obj_asuint32(obj) * 60;
	if (heartbeat_interval == 0) {
		CHECK(isc_timer_reset(server->heartbeat_timer,
				      isc_timertype_inactive,
				      NULL, NULL, ISC_TRUE));
	} else if (server->heartbeat_interval != heartbeat_interval) {
		isc_interval_set(&interval, heartbeat_interval, 0);
		CHECK(isc_timer_reset(server->heartbeat_timer,
				      isc_timertype_ticker,
				      NULL, &interval, ISC_FALSE));
	}
	server->heartbeat_interval = heartbeat_interval;

	isc_interval_set(&interval, 1200, 0);
	CHECK(isc_timer_reset(server->pps_timer, isc_timertype_ticker, NULL,
			      &interval, ISC_FALSE));

	isc_interval_set(&interval, named_g_tat_interval, 0);
	CHECK(isc_timer_reset(server->tat_timer, isc_timertype_ticker, NULL,
			      &interval, ISC_FALSE));

	/*
	 * Write the PID file.
	 */
	obj = NULL;
	if (named_config_get(maps, "pid-file", &obj) == ISC_R_SUCCESS) {
		if (cfg_obj_isvoid(obj)) {
			named_os_writepidfile(NULL, first_time);
		} else {
			named_os_writepidfile(cfg_obj_asstring(obj),
					      first_time);
		}
	} else {
		named_os_writepidfile(named_g_defaultpidfile, first_time);
	}

	/*
	 * Configure the server-wide session key.  This must be done before
	 * configure views because zone configuration may need to know
	 * session-keyname.
	 *
	 * Failure of session key generation isn't fatal at this time; if it
	 * turns out that a session key is really needed but doesn't exist,
	 * we'll treat it as a fatal error then.
	 */
	(void)configure_session_key(maps, server, named_g_mctx);

	views = NULL;
	(void)cfg_map_get(config, "view", &views);

	/*
	 * Create the views and count all the configured zones in
	 * order to correctly size the zone manager's task table.
	 * (We only count zones for configured views; the built-in
	 * "bind" view can be ignored as it only adds a negligible
	 * number of zones.)
	 *
	 * If we're allowing new zones, we need to be able to find the
	 * new zone file and count those as well.  So we setup the new
	 * zone configuration context, but otherwise view configuration
	 * waits until after the zone manager's task list has been sized.
	 */
	for (element = cfg_list_first(views);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		cfg_obj_t *vconfig = cfg_listelt_value(element);
		const cfg_obj_t *voptions = cfg_tuple_get(vconfig, "options");
		int nzf_num_zones;

		view = NULL;

		CHECK(create_view(vconfig, &viewlist, &view));
		INSIST(view != NULL);

		num_zones += count_zones(voptions);

		CHECK(setup_newzones(view, config, vconfig, conf_parser,
				     named_g_aclconfctx, &nzf_num_zones));
		num_zones += nzf_num_zones;

		dns_view_detach(&view);
	}

	/*
	 * If there were no explicit views then we do the default
	 * view here.
	 */
	if (views == NULL) {
		int nzf_num_zones;

		CHECK(create_view(NULL, &viewlist, &view));
		INSIST(view != NULL);

		num_zones = count_zones(config);

		CHECK(setup_newzones(view, config, NULL, conf_parser,
				     named_g_aclconfctx, &nzf_num_zones));
		num_zones += nzf_num_zones;

		dns_view_detach(&view);
	}

	/*
	 * Zones have been counted; set the zone manager task pool size.
	 */
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "sizing zone task pool based on %d zones", num_zones);
	CHECK(dns_zonemgr_setsize(named_g_server->zonemgr, num_zones));

	/*
	 * Configure and freeze all explicit views.  Explicit
	 * views that have zones were already created at parsing
	 * time, but views with no zones must be created here.
	 */
	for (element = cfg_list_first(views);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		cfg_obj_t *vconfig = cfg_listelt_value(element);

		view = NULL;
		CHECK(find_view(vconfig, &viewlist, &view));
		CHECK(configure_view(view, &viewlist, config, vconfig,
				     &cachelist, bindkeys, named_g_mctx,
				     named_g_aclconfctx, ISC_TRUE));
		dns_view_freeze(view);
		dns_view_detach(&view);
	}

	/*
	 * Make sure we have a default view if and only if there
	 * were no explicit views.
	 */
	if (views == NULL) {
		view = NULL;
		CHECK(find_view(NULL, &viewlist, &view));
		CHECK(configure_view(view, &viewlist, config, NULL,
				     &cachelist, bindkeys,
				     named_g_mctx, named_g_aclconfctx,
				     ISC_TRUE));
		dns_view_freeze(view);
		dns_view_detach(&view);
	}

	/*
	 * Create (or recreate) the built-in views.
	 */
	builtin_views = NULL;
	RUNTIME_CHECK(cfg_map_get(named_g_config, "view",
				  &builtin_views) == ISC_R_SUCCESS);
	for (element = cfg_list_first(builtin_views);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		cfg_obj_t *vconfig = cfg_listelt_value(element);

		CHECK(create_view(vconfig, &builtin_viewlist, &view));
		CHECK(configure_view(view, &viewlist, config, vconfig,
				     &cachelist, bindkeys,
				     named_g_mctx, named_g_aclconfctx,
				     ISC_FALSE));
		dns_view_freeze(view);
		dns_view_detach(&view);
		view = NULL;
	}

	/* Now combine the two viewlists into one */
	ISC_LIST_APPENDLIST(viewlist, builtin_viewlist, link);

	/*
	 * Commit any dns_zone_setview() calls on all zones in the new
	 * view.
	 */
	for (view = ISC_LIST_HEAD(viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		dns_view_setviewcommit(view);
	}

	/* Swap our new view list with the production one. */
	tmpviewlist = server->viewlist;
	server->viewlist = viewlist;
	viewlist = tmpviewlist;

	/* Make the view list available to each of the views */
	view = ISC_LIST_HEAD(server->viewlist);
	while (view != NULL) {
		view->viewlist = &server->viewlist;
		view = ISC_LIST_NEXT(view, link);
	}

	/* Swap our new cache list with the production one. */
	tmpcachelist = server->cachelist;
	server->cachelist = cachelist;
	cachelist = tmpcachelist;

	/* Load the TKEY information from the configuration. */
	if (options != NULL) {
		dns_tkeyctx_t *t = NULL;
		CHECKM(named_tkeyctx_fromconfig(options, named_g_mctx,
						named_g_entropy, &t),
		       "configuring TKEY");
		if (server->sctx->tkeyctx != NULL) {
			dns_tkeyctx_destroy(&server->sctx->tkeyctx);
		}
		server->sctx->tkeyctx = t;
	}

	/*
	 * Bind the control port(s).
	 */
	CHECKM(named_controls_configure(named_g_server->controls, config,
					named_g_aclconfctx),
	       "binding control channel(s)");

	/*
	 * Open the source of entropy.
	 */
	if (first_time) {
		const char *randomdev = NULL;
		int level = ISC_LOG_ERROR;
		obj = NULL;
		result = named_config_get(maps, "random-device", &obj);
		if (result == ISC_R_SUCCESS) {
			if (!cfg_obj_isvoid(obj)) {
				level = ISC_LOG_INFO;
				randomdev = cfg_obj_asstring(obj);
			}
		}
		if (randomdev == NULL) {
#ifdef ISC_PLATFORM_CRYPTORANDOM
			isc_entropy_usehook(named_g_entropy, ISC_TRUE);
#else
			if ((obj != NULL) && !cfg_obj_isvoid(obj))
				level = ISC_LOG_INFO;
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, level,
				      "no source of entropy found");
			if ((obj == NULL) || cfg_obj_isvoid(obj)) {
				CHECK(ISC_R_FAILURE);
			}
#endif
		} else {
			result = isc_entropy_createfilesource(named_g_entropy,
							      randomdev);
#ifdef PATH_RANDOMDEV
			if (named_g_fallbackentropy != NULL) {
				level = ISC_LOG_INFO;
			}
#endif
			if (result != ISC_R_SUCCESS) {
				isc_log_write(named_g_lctx,
					      NAMED_LOGCATEGORY_GENERAL,
					      NAMED_LOGMODULE_SERVER,
					      level,
					      "could not open "
					      "entropy source %s: %s",
					      randomdev,
					      isc_result_totext(result));
			}
#ifdef PATH_RANDOMDEV
			if (named_g_fallbackentropy != NULL) {
				if (result != ISC_R_SUCCESS) {
					isc_log_write(named_g_lctx,
						      NAMED_LOGCATEGORY_GENERAL,
						      NAMED_LOGMODULE_SERVER,
						      ISC_LOG_INFO,
						      "using pre-chroot "
						      "entropy source %s",
						      PATH_RANDOMDEV);
					isc_entropy_detach(&named_g_entropy);
					isc_entropy_attach(
						   named_g_fallbackentropy,
						   &named_g_entropy);
				}
				isc_entropy_detach(&named_g_fallbackentropy);
			}
#endif
		}
	}

#ifdef HAVE_LMDB
	/*
	 * If we're using LMDB, we may have created newzones databases
	 * as root, making it impossible to reopen them later after
	 * switching to a new userid. We close them now, and reopen
	 * after relinquishing privileges them.
	 */
	if (first_time) {
		for (view = ISC_LIST_HEAD(server->viewlist);
		     view != NULL;
		     view = ISC_LIST_NEXT(view, link))
		{
			nzd_env_close(view);
		}
	}
#endif /* HAVE_LMDB */

	/*
	 * Relinquish root privileges.
	 */
	if (first_time) {
		named_os_changeuser();
	}

#if 0
	/*
	 * Check that the working directory is writable.
	 */
	if (!isc_file_isdirwritable(".")) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "the working directory is not writable");
		result = ISC_R_NOPERM;
		goto cleanup;
	}
#endif
#ifdef HAVE_LMDB
	/*
	 * Reopen NZD databases.
	 */
	if (first_time) {
		for (view = ISC_LIST_HEAD(server->viewlist);
		     view != NULL;
		     view = ISC_LIST_NEXT(view, link))
		{
			nzd_env_reopen(view);
		}
	}
#endif /* HAVE_LMDB */

	/*
	 * Configure the logging system.
	 *
	 * Do this after changing UID to make sure that any log
	 * files specified in named.conf get created by the
	 * unprivileged user, not root.
	 */
	if (named_g_logstderr) {
		const cfg_obj_t *logobj = NULL;

		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "not using config file logging "
			      "statement for logging due to "
			      "-g option");

		(void)cfg_map_get(config, "logging", &logobj);
		if (logobj != NULL) {
			result = named_logconfig(NULL, logobj);
			if (result != ISC_R_SUCCESS) {
				isc_log_write(named_g_lctx,
					      NAMED_LOGCATEGORY_GENERAL,
					      NAMED_LOGMODULE_SERVER,
					      ISC_LOG_ERROR,
					      "checking logging configuration "
					      "failed: %s",
					      isc_result_totext(result));
				goto cleanup;
			}
		}
	} else {
		const cfg_obj_t *logobj = NULL;

		CHECKM(isc_logconfig_create(named_g_lctx, &logc),
		       "creating new logging configuration");

		logobj = NULL;
		(void)cfg_map_get(config, "logging", &logobj);
		if (logobj != NULL) {
			CHECKM(named_logconfig(logc, logobj),
			       "configuring logging");
		} else {
			CHECKM(named_log_setdefaultchannels(logc),
			       "setting up default logging channels");
			CHECKM(named_log_setunmatchedcategory(logc),
			       "setting up default 'category unmatched'");
			CHECKM(named_log_setdefaultcategory(logc),
			       "setting up default 'category default'");
		}

		CHECKM(isc_logconfig_use(named_g_lctx, logc),
		       "installing logging configuration");
		logc = NULL;

		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_DEBUG(1),
			      "now using logging configuration from "
			      "config file");
	}

	/*
	 * Set the default value of the query logging flag depending
	 * whether a "queries" category has been defined.  This is
	 * a disgusting hack, but we need to do this for BIND 8
	 * compatibility.
	 */
	if (first_time) {
		const cfg_obj_t *logobj = NULL;
		const cfg_obj_t *categories = NULL;

		obj = NULL;
		if (named_config_get(maps, "querylog", &obj) == ISC_R_SUCCESS) {
			ns_server_setoption(server->sctx,
					    NS_SERVER_LOGQUERIES,
					    cfg_obj_asboolean(obj));
		} else {

			(void)cfg_map_get(config, "logging", &logobj);
			if (logobj != NULL)
				(void)cfg_map_get(logobj, "category",
						  &categories);
			if (categories != NULL) {
				for (element = cfg_list_first(categories);
				     element != NULL;
				     element = cfg_list_next(element))
				{
					const cfg_obj_t *catobj;
					const char *str;

					obj = cfg_listelt_value(element);
					catobj = cfg_tuple_get(obj, "name");
					str = cfg_obj_asstring(catobj);
					if (strcasecmp(str, "queries") == 0)
						ns_server_setoption(
						    server->sctx,
						    NS_SERVER_LOGQUERIES,
						    ISC_TRUE);
				}
			}
		}
	}

	obj = NULL;
	if (options != NULL &&
	    cfg_map_get(options, "memstatistics", &obj) == ISC_R_SUCCESS)
	{
		named_g_memstatistics = cfg_obj_asboolean(obj);
	} else {
		named_g_memstatistics =
			ISC_TF((isc_mem_debugging & ISC_MEM_DEBUGRECORD) != 0);
	}

	obj = NULL;
	if (named_config_get(maps, "memstatistics-file", &obj) == ISC_R_SUCCESS)
	{
		named_main_setmemstats(cfg_obj_asstring(obj));
	} else if (named_g_memstatistics) {
		named_main_setmemstats("named.memstats");
	} else {
		named_main_setmemstats(NULL);
	}

	obj = NULL;
	result = named_config_get(maps, "statistics-file", &obj);
	INSIST(result == ISC_R_SUCCESS);
	CHECKM(setstring(server, &server->statsfile, cfg_obj_asstring(obj)),
	       "strdup");

	obj = NULL;
	result = named_config_get(maps, "dump-file", &obj);
	INSIST(result == ISC_R_SUCCESS);
	CHECKM(setstring(server, &server->dumpfile, cfg_obj_asstring(obj)),
	       "strdup");

	obj = NULL;
	result = named_config_get(maps, "secroots-file", &obj);
	INSIST(result == ISC_R_SUCCESS);
	CHECKM(setstring(server, &server->secrootsfile, cfg_obj_asstring(obj)),
	       "strdup");

	obj = NULL;
	result = named_config_get(maps, "recursing-file", &obj);
	INSIST(result == ISC_R_SUCCESS);
	CHECKM(setstring(server, &server->recfile, cfg_obj_asstring(obj)),
	       "strdup");

	obj = NULL;
	result = named_config_get(maps, "version", &obj);
	if (result == ISC_R_SUCCESS) {
		CHECKM(setoptstring(server, &server->version, obj), "strdup");
		server->version_set = ISC_TRUE;
	} else {
		server->version_set = ISC_FALSE;
	}

	obj = NULL;
	result = named_config_get(maps, "hostname", &obj);
	if (result == ISC_R_SUCCESS) {
		CHECKM(setoptstring(server, &server->hostname, obj), "strdup");
		server->hostname_set = ISC_TRUE;
	} else {
		server->hostname_set = ISC_FALSE;
	}

	obj = NULL;
	result = named_config_get(maps, "server-id", &obj);
	server->sctx->gethostname = NULL;
	if (result == ISC_R_SUCCESS && cfg_obj_isboolean(obj)) {
		/* The parser translates "hostname" to ISC_TRUE */
		server->sctx->gethostname = named_os_gethostname;
		result = ns_server_setserverid(server->sctx, NULL);
	} else if (result == ISC_R_SUCCESS && !cfg_obj_isvoid(obj)) {
		/* Found a quoted string */
		result = ns_server_setserverid(server->sctx,
					       cfg_obj_asstring(obj));
	} else {
		result = ns_server_setserverid(server->sctx, NULL);
	}
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	obj = NULL;
	result = named_config_get(maps, "flush-zones-on-shutdown", &obj);
	if (result == ISC_R_SUCCESS) {
		server->flushonshutdown = cfg_obj_asboolean(obj);
	} else {
		server->flushonshutdown = ISC_FALSE;
	}

	obj = NULL;
	result = named_config_get(maps, "answer-cookie", &obj);
	INSIST(result == ISC_R_SUCCESS);
	server->sctx->answercookie = cfg_obj_asboolean(obj);

	obj = NULL;
	result = named_config_get(maps, "cookie-algorithm", &obj);
	INSIST(result == ISC_R_SUCCESS);
	if (strcasecmp(cfg_obj_asstring(obj), "aes") == 0) {
#if defined(HAVE_OPENSSL_AES) || defined(HAVE_OPENSSL_EVP_AES)
		server->sctx->cookiealg = ns_cookiealg_aes;
#else
		INSIST(0);
#endif
	} else if (strcasecmp(cfg_obj_asstring(obj), "sha1") == 0) {
		server->sctx->cookiealg = ns_cookiealg_sha1;
	} else if (strcasecmp(cfg_obj_asstring(obj), "sha256") == 0) {
		server->sctx->cookiealg = ns_cookiealg_sha256;
	} else {
		INSIST(0);
	}

	obj = NULL;
	result = named_config_get(maps, "cookie-secret", &obj);
	if (result == ISC_R_SUCCESS) {
		const char *str;
		isc_boolean_t first = ISC_TRUE;
		isc_buffer_t b;
		unsigned int usedlength;

		for (element = cfg_list_first(obj);
		     element != NULL;
		     element = cfg_list_next(element))
		{
			obj = cfg_listelt_value(element);
			str = cfg_obj_asstring(obj);

			if (first) {
				memset(server->sctx->secret, 0,
				       sizeof(server->sctx->secret));
				isc_buffer_init(&b, server->sctx->secret,
						sizeof(server->sctx->secret));
				result = isc_hex_decodestring(str, &b);
				if (result != ISC_R_SUCCESS &&
				    result != ISC_R_NOSPACE)
				{
					goto cleanup;
				}
				first = ISC_FALSE;
			} else {
				altsecret = isc_mem_get(server->sctx->mctx,
							sizeof(*altsecret));
				if (altsecret == NULL) {
					result = ISC_R_NOMEMORY;
					goto cleanup;
				}
				isc_buffer_init(&b, altsecret->secret,
						sizeof(altsecret->secret));
				result = isc_hex_decodestring(str, &b);
				if (result != ISC_R_SUCCESS &&
				    result != ISC_R_NOSPACE)
				{
					isc_mem_put(server->sctx->mctx,
						    altsecret,
						    sizeof(*altsecret));
					goto cleanup;
				}
				ISC_LIST_INITANDAPPEND(altsecrets,
						       altsecret, link);
			}

			usedlength = isc_buffer_usedlength(&b);
			switch (server->sctx->cookiealg) {
			case ns_cookiealg_aes:
				if (usedlength != ISC_AES128_KEYLENGTH) {
					CHECKM(ISC_R_RANGE,
					       "AES cookie-secret must be "
					       "128 bits");
				}
				break;
			case ns_cookiealg_sha1:
				if (usedlength != ISC_SHA1_DIGESTLENGTH) {
					CHECKM(ISC_R_RANGE,
					       "SHA1 cookie-secret must be "
					       "160 bits");
				}
				break;
			case ns_cookiealg_sha256:
				if (usedlength != ISC_SHA256_DIGESTLENGTH) {
					CHECKM(ISC_R_RANGE,
					       "SHA256 cookie-secret must be "
					       "256 bits");
				}
				break;
			}
		}
	} else {
		result = isc_entropy_getdata(named_g_entropy,
					     server->sctx->secret,
					     sizeof(server->sctx->secret),
					     NULL,
					     0);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
	}

	/*
	 * Swap altsecrets lists.
	 */
	tmpaltsecrets = server->sctx->altsecrets;
	server->sctx->altsecrets = altsecrets;
	altsecrets = tmpaltsecrets;

	(void) named_server_loadnta(server);

#ifdef USE_DNSRPS
	/*
	 * Start and connect to the DNS Response Policy Service
	 * daemon, dnsrpzd, for each view that uses DNSRPS.
	 */
	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link)) {
		result = dns_dnsrps_connect(view->rpzs);
		if (result != ISC_R_SUCCESS) {
			view = NULL;
			goto cleanup;
		}
	}
#endif

	result = ISC_R_SUCCESS;

 cleanup:
	if (logc != NULL) {
		isc_logconfig_destroy(&logc);
	}

	if (v4portset != NULL) {
		isc_portset_destroy(named_g_mctx, &v4portset);
	}

	if (v6portset != NULL) {
		isc_portset_destroy(named_g_mctx, &v6portset);
	}

	if (conf_parser != NULL) {
		if (config != NULL) {
			cfg_obj_destroy(conf_parser, &config);
		}
		cfg_parser_destroy(&conf_parser);
	}

	if (bindkeys_parser != NULL) {
		if (bindkeys != NULL) {
			cfg_obj_destroy(bindkeys_parser, &bindkeys);
		}
		cfg_parser_destroy(&bindkeys_parser);
	}

	if (view != NULL) {
		dns_view_detach(&view);
	}

	ISC_LIST_APPENDLIST(viewlist, builtin_viewlist, link);

	/*
	 * This cleans up either the old production view list
	 * or our temporary list depending on whether they
	 * were swapped above or not.
	 */
	for (view = ISC_LIST_HEAD(viewlist);
	     view != NULL;
	     view = view_next) {
		view_next = ISC_LIST_NEXT(view, link);
		ISC_LIST_UNLINK(viewlist, view, link);
		if (result == ISC_R_SUCCESS &&
		    strcmp(view->name, "_bind") != 0)
		{
			dns_view_setviewrevert(view);
			(void)dns_zt_apply(view->zonetable, ISC_FALSE,
					   removed, view);
		}
		dns_view_detach(&view);
	}

	/* Same cleanup for cache list. */
	while ((nsc = ISC_LIST_HEAD(cachelist)) != NULL) {
		ISC_LIST_UNLINK(cachelist, nsc, link);
		dns_cache_detach(&nsc->cache);
		isc_mem_put(server->mctx, nsc, sizeof(*nsc));
	}

	/* Cleanup for altsecrets list. */
	while ((altsecret = ISC_LIST_HEAD(altsecrets)) != NULL) {
		ISC_LIST_UNLINK(altsecrets, altsecret, link);
		isc_mem_put(server->sctx->mctx, altsecret, sizeof(*altsecret));
	}

	/*
	 * Adjust the listening interfaces in accordance with the source
	 * addresses specified in views and zones.
	 */
	if (isc_net_probeipv6() == ISC_R_SUCCESS) {
		adjust_interfaces(server, named_g_mctx);
	}

	/*
	 * Record the time of most recent configuration
	 */
	tresult = isc_time_now(&named_g_configtime);
	if (tresult != ISC_R_SUCCESS) {
		named_main_earlyfatal("isc_time_now() failed: %s",
				      isc_result_totext(result));
	}

	/* Relinquish exclusive access to configuration data. */
	if (exclusive) {
		isc_task_endexclusive(server->task);
	}

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_DEBUG(1),
		      "load_configuration: %s",
		      isc_result_totext(result));

	return (result);
}

static isc_result_t
view_loaded(void *arg) {
	isc_result_t result;
	ns_zoneload_t *zl = (ns_zoneload_t *) arg;
	named_server_t *server = zl->server;
	isc_boolean_t reconfig = zl->reconfig;
	unsigned int refs;


	/*
	 * Force zone maintenance.  Do this after loading
	 * so that we know when we need to force AXFR of
	 * slave zones whose master files are missing.
	 *
	 * We use the zoneload reference counter to let us
	 * know when all views are finished.
	 */
	isc_refcount_decrement(&zl->refs, &refs);
	if (refs != 0)
		return (ISC_R_SUCCESS);

	isc_refcount_destroy(&zl->refs);
	isc_mem_put(server->mctx, zl, sizeof (*zl));

	/*
	 * To maintain compatibility with log parsing tools that might
	 * be looking for this string after "rndc reconfig", we keep it
	 * as it is
	 */
	if (reconfig) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "any newly configured zones are now loaded");
	} else {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_NOTICE,
			      "all zones loaded");
	}

	CHECKFATAL(dns_zonemgr_forcemaint(server->zonemgr),
		   "forcing zone maintenance");

	named_os_started();
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_NOTICE, "running");

	return (ISC_R_SUCCESS);
}

static isc_result_t
load_zones(named_server_t *server, isc_boolean_t init, isc_boolean_t reconfig) {
	isc_result_t result;
	dns_view_t *view;
	ns_zoneload_t *zl;
	unsigned int refs = 0;

	zl = isc_mem_get(server->mctx, sizeof (*zl));
	if (zl == NULL)
		return (ISC_R_NOMEMORY);
	zl->server = server;
	zl->reconfig = reconfig;

	result = isc_task_beginexclusive(server->task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	isc_refcount_init(&zl->refs, 1);

	/*
	 * Schedule zones to be loaded from disk.
	 */
	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		if (view->managed_keys != NULL) {
			result = dns_zone_load(view->managed_keys);
			if (result != ISC_R_SUCCESS &&
			    result != DNS_R_UPTODATE &&
			    result != DNS_R_CONTINUE)
				goto cleanup;
		}
		if (view->redirect != NULL) {
			result = dns_zone_load(view->redirect);
			if (result != ISC_R_SUCCESS &&
			    result != DNS_R_UPTODATE &&
			    result != DNS_R_CONTINUE)
				goto cleanup;
		}

		/*
		 * 'dns_view_asyncload' calls view_loaded if there are no
		 * zones.
		 */
		isc_refcount_increment(&zl->refs, NULL);
		CHECK(dns_view_asyncload(view, view_loaded, zl));
	}

 cleanup:
	isc_refcount_decrement(&zl->refs, &refs);
	if (refs == 0) {
		isc_refcount_destroy(&zl->refs);
		isc_mem_put(server->mctx, zl, sizeof (*zl));
	} else if (init) {
		/*
		 * Place the task manager into privileged mode.  This
		 * ensures that after we leave task-exclusive mode, no
		 * other tasks will be able to run except for the ones
		 * that are loading zones. (This should only be done during
		 * the initial server setup; it isn't necessary during
		 * a reload.)
		 */
		isc_taskmgr_setmode(named_g_taskmgr,
				    isc_taskmgrmode_privileged);
	}

	isc_task_endexclusive(server->task);
	return (result);
}

static void
run_server(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	named_server_t *server = (named_server_t *)event->ev_arg;

	INSIST(task == server->task);

	isc_event_free(&event);

	CHECKFATAL(dns_dispatchmgr_create(named_g_mctx, named_g_entropy,
					  &named_g_dispatchmgr),
		   "creating dispatch manager");

	dns_dispatchmgr_setstats(named_g_dispatchmgr, server->resolverstats);

#ifdef HAVE_GEOIP
	CHECKFATAL(ns_interfacemgr_create(named_g_mctx, server->sctx,
					  named_g_taskmgr, named_g_timermgr,
					  named_g_socketmgr,
					  named_g_dispatchmgr,
					  server->task, named_g_udpdisp,
					  named_g_geoip,
					  &server->interfacemgr),
		   "creating interface manager");
#else
	CHECKFATAL(ns_interfacemgr_create(named_g_mctx, server->sctx,
					  named_g_taskmgr, named_g_timermgr,
					  named_g_socketmgr,
					  named_g_dispatchmgr,
					  server->task, named_g_udpdisp,
					  NULL,
					  &server->interfacemgr),
		   "creating interface manager");
#endif

	CHECKFATAL(isc_timer_create(named_g_timermgr, isc_timertype_inactive,
				    NULL, NULL, server->task,
				    interface_timer_tick,
				    server, &server->interface_timer),
		   "creating interface timer");

	CHECKFATAL(isc_timer_create(named_g_timermgr, isc_timertype_inactive,
				    NULL, NULL, server->task,
				    heartbeat_timer_tick,
				    server, &server->heartbeat_timer),
		   "creating heartbeat timer");

	CHECKFATAL(isc_timer_create(named_g_timermgr, isc_timertype_inactive,
				    NULL, NULL, server->task, tat_timer_tick,
				    server, &server->tat_timer),
		   "creating trust anchor telemetry timer");

	CHECKFATAL(isc_timer_create(named_g_timermgr, isc_timertype_inactive,
				    NULL, NULL, server->task, pps_timer_tick,
				    server, &server->pps_timer),
		   "creating pps timer");

	CHECKFATAL(cfg_parser_create(named_g_mctx, named_g_lctx,
				     &named_g_parser),
		   "creating default configuration parser");

	CHECKFATAL(cfg_parser_create(named_g_mctx, named_g_lctx,
				     &named_g_addparser),
		   "creating additional configuration parser");

	CHECKFATAL(load_configuration(named_g_conffile, server, ISC_TRUE),
		   "loading configuration");

	isc_hash_init();

	CHECKFATAL(load_zones(server, ISC_TRUE, ISC_FALSE), "loading zones");
#ifdef ENABLE_AFL
	named_g_run_done = ISC_TRUE;
#endif
}

void
named_server_flushonshutdown(named_server_t *server, isc_boolean_t flush) {
	REQUIRE(NAMED_SERVER_VALID(server));

	server->flushonshutdown = flush;
}

static void
shutdown_server(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	dns_view_t *view, *view_next;
	named_server_t *server = (named_server_t *)event->ev_arg;
	isc_boolean_t flush = server->flushonshutdown;
	named_cache_t *nsc;

	UNUSED(task);
	INSIST(task == server->task);

	result = isc_task_beginexclusive(server->task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "shutting down%s",
		      flush ? ": flushing changes" : "");

	named_statschannels_shutdown(server);
	named_controls_shutdown(server->controls);
	end_reserved_dispatches(server, ISC_TRUE);
	cleanup_session_key(server, server->mctx);

	if (named_g_aclconfctx != NULL)
		cfg_aclconfctx_detach(&named_g_aclconfctx);

	cfg_obj_destroy(named_g_parser, &named_g_config);
	cfg_parser_destroy(&named_g_parser);
	cfg_parser_destroy(&named_g_addparser);

	(void) named_server_saventa(server);

	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = view_next) {
		view_next = ISC_LIST_NEXT(view, link);
		ISC_LIST_UNLINK(server->viewlist, view, link);
		if (flush)
			dns_view_flushanddetach(&view);
		else
			dns_view_detach(&view);
	}

	dns_dyndb_cleanup(ISC_TRUE);

	while ((nsc = ISC_LIST_HEAD(server->cachelist)) != NULL) {
		ISC_LIST_UNLINK(server->cachelist, nsc, link);
		dns_cache_detach(&nsc->cache);
		isc_mem_put(server->mctx, nsc, sizeof(*nsc));
	}

	isc_timer_detach(&server->interface_timer);
	isc_timer_detach(&server->heartbeat_timer);
	isc_timer_detach(&server->pps_timer);
	isc_timer_detach(&server->tat_timer);

	ns_interfacemgr_shutdown(server->interfacemgr);
	ns_interfacemgr_detach(&server->interfacemgr);

	dns_dispatchmgr_destroy(&named_g_dispatchmgr);

	dns_zonemgr_shutdown(server->zonemgr);

	if (named_g_sessionkey != NULL) {
		dns_tsigkey_detach(&named_g_sessionkey);
		dns_name_free(&named_g_sessionkeyname, server->mctx);
	}
#ifdef HAVE_DNSTAP
	dns_dt_shutdown();
#endif
#ifdef HAVE_GEOIP
	dns_geoip_shutdown();
#endif

	dns_db_detach(&server->in_roothints);

	isc_task_endexclusive(server->task);

	isc_task_detach(&server->task);

	isc_event_free(&event);
}

/*%
 * Find a view that matches the source and destination addresses of a query.
 */
static isc_result_t
get_matching_view(isc_netaddr_t *srcaddr, isc_netaddr_t *destaddr,
		  dns_message_t *message, dns_aclenv_t *env, dns_ecs_t *ecs,
		  isc_result_t *sigresult, dns_view_t **viewp)
{
	dns_view_t *view;

	REQUIRE(message != NULL);
	REQUIRE(sigresult != NULL);
	REQUIRE(viewp != NULL && *viewp == NULL);

	for (view = ISC_LIST_HEAD(named_g_server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		if (message->rdclass == view->rdclass ||
		    message->rdclass == dns_rdataclass_any)
		{
			dns_name_t *tsig = NULL;
			isc_netaddr_t *addr = NULL;
			isc_uint8_t *scope = NULL;
			isc_uint8_t source = 0;

			*sigresult = dns_message_rechecksig(message, view);
			if (*sigresult == ISC_R_SUCCESS) {
				dns_tsigkey_t *tsigkey;

				tsigkey = message->tsigkey;
				tsig = dns_tsigkey_identity(tsigkey);
			}

			if (ecs != NULL) {
				addr = &ecs->addr;
				source = ecs->source;
				scope = &ecs->scope;
			}

			if (dns_acl_allowed(srcaddr, tsig, addr, source,
					    scope, view->matchclients, env) &&
			    dns_acl_allowed(destaddr, tsig, NULL, 0, NULL,
					    view->matchdestinations, env) &&
			    !(view->matchrecursiveonly &&
			      (message->flags & DNS_MESSAGEFLAG_RD) == 0))
			{
				dns_view_attach(view, viewp);
				return (ISC_R_SUCCESS);
			}
		}
	}

	return (ISC_R_NOTFOUND);
}

void
named_server_create(isc_mem_t *mctx, named_server_t **serverp) {
	isc_result_t result;
	named_server_t *server = isc_mem_get(mctx, sizeof(*server));

	if (server == NULL)
		fatal("allocating server object", ISC_R_NOMEMORY);

	server->mctx = mctx;
	server->task = NULL;
	server->zonemgr = NULL;

#ifdef USE_DNSRPS
	CHECKFATAL(dns_dnsrps_server_create(),
		   "initializing RPZ service interface");
#endif

	/* Initialize server data structures. */
	server->interfacemgr = NULL;
	ISC_LIST_INIT(server->viewlist);
	server->in_roothints = NULL;

	/* Must be first. */
	CHECKFATAL(dst_lib_init2(named_g_mctx, named_g_entropy,
				 named_g_engine, ISC_ENTROPY_GOODONLY),
		   "initializing DST");

	CHECKFATAL(dns_rootns_create(mctx, dns_rdataclass_in, NULL,
				     &server->in_roothints),
		   "setting up root hints");

	CHECKFATAL(isc_mutex_init(&server->reload_event_lock),
		   "initializing reload event lock");
	server->reload_event =
		isc_event_allocate(named_g_mctx, server,
				   NAMED_EVENT_RELOAD,
				   named_server_reload,
				   server,
				   sizeof(isc_event_t));
	CHECKFATAL(server->reload_event == NULL ?
		   ISC_R_NOMEMORY : ISC_R_SUCCESS,
		   "allocating reload event");

	/*
	 * Setup the server task, which is responsible for coordinating
	 * startup and shutdown of the server, as well as all exclusive
	 * tasks.
	 */
	CHECKFATAL(isc_task_create(named_g_taskmgr, 0, &server->task),
		   "creating server task");
	isc_task_setname(server->task, "server", server);
	isc_taskmgr_setexcltask(named_g_taskmgr, server->task);

	server->sctx = NULL;
	CHECKFATAL(ns_server_create(mctx, named_g_entropy,
				    get_matching_view,
				    &server->sctx),
		   "creating server context");

#ifdef HAVE_GEOIP
	/*
	 * GeoIP must be initialized before the interface
	 * manager (which includes the ACL environment)
	 * is created
	 */
	named_geoip_init();
#endif

#ifdef ENABLE_AFL
	server->sctx->fuzztype = named_g_fuzz_type;
	server->sctx->fuzznotify = named_fuzz_notify;
#endif

	CHECKFATAL(isc_task_onshutdown(server->task, shutdown_server, server),
		   "isc_task_onshutdown");
	CHECKFATAL(isc_app_onrun(named_g_mctx, server->task,
				 run_server, server),
		   "isc_app_onrun");

	server->interface_timer = NULL;
	server->heartbeat_timer = NULL;
	server->pps_timer = NULL;
	server->tat_timer = NULL;

	server->interface_interval = 0;
	server->heartbeat_interval = 0;

	CHECKFATAL(dns_zonemgr_create(named_g_mctx, named_g_taskmgr,
				      named_g_timermgr, named_g_socketmgr,
				      &server->zonemgr),
		   "dns_zonemgr_create");
	CHECKFATAL(dns_zonemgr_setsize(server->zonemgr, 1000),
		   "dns_zonemgr_setsize");

	server->statsfile = isc_mem_strdup(server->mctx, "named.stats");
	CHECKFATAL(server->statsfile == NULL ? ISC_R_NOMEMORY : ISC_R_SUCCESS,
		   "isc_mem_strdup");

	server->bindkeysfile = isc_mem_strdup(server->mctx, "bind.keys");
	CHECKFATAL(server->bindkeysfile == NULL ? ISC_R_NOMEMORY :
						  ISC_R_SUCCESS,
		   "isc_mem_strdup");

	server->dumpfile = isc_mem_strdup(server->mctx, "named_dump.db");
	CHECKFATAL(server->dumpfile == NULL ? ISC_R_NOMEMORY : ISC_R_SUCCESS,
		   "isc_mem_strdup");

	server->secrootsfile = isc_mem_strdup(server->mctx, "named.secroots");
	CHECKFATAL(server->secrootsfile == NULL ? ISC_R_NOMEMORY :
						  ISC_R_SUCCESS,
		   "isc_mem_strdup");

	server->recfile = isc_mem_strdup(server->mctx, "named.recursing");
	CHECKFATAL(server->recfile == NULL ? ISC_R_NOMEMORY : ISC_R_SUCCESS,
		   "isc_mem_strdup");

	server->hostname_set = ISC_FALSE;
	server->hostname = NULL;
	server->version_set = ISC_FALSE;
	server->version = NULL;

	server->zonestats = NULL;
	server->resolverstats = NULL;
	server->sockstats = NULL;
	CHECKFATAL(isc_stats_create(server->mctx, &server->sockstats,
				    isc_sockstatscounter_max),
		   "isc_stats_create");
	isc_socketmgr_setstats(named_g_socketmgr, server->sockstats);

	CHECKFATAL(isc_stats_create(named_g_mctx, &server->zonestats,
				    dns_zonestatscounter_max),
		   "dns_stats_create (zone)");

	CHECKFATAL(isc_stats_create(named_g_mctx, &server->resolverstats,
				    dns_resstatscounter_max),
		   "dns_stats_create (resolver)");

	server->flushonshutdown = ISC_FALSE;

	server->controls = NULL;
	CHECKFATAL(named_controls_create(server, &server->controls),
		   "named_controls_create");
	server->dispatchgen = 0;
	ISC_LIST_INIT(server->dispatches);

	ISC_LIST_INIT(server->statschannels);

	ISC_LIST_INIT(server->cachelist);

	server->sessionkey = NULL;
	server->session_keyfile = NULL;
	server->session_keyname = NULL;
	server->session_keyalg = DST_ALG_UNKNOWN;
	server->session_keybits = 0;

	server->lockfile = NULL;

	server->dtenv = NULL;

	server->magic = NAMED_SERVER_MAGIC;
	*serverp = server;
}

void
named_server_destroy(named_server_t **serverp) {
	named_server_t *server = *serverp;
	REQUIRE(NAMED_SERVER_VALID(server));

#ifdef HAVE_DNSTAP
	if (server->dtenv != NULL)
		dns_dt_detach(&server->dtenv);
#endif /* HAVE_DNSTAP */

#ifdef USE_DNSRPS
	dns_dnsrps_server_destroy();
#endif

	named_controls_destroy(&server->controls);

	isc_stats_detach(&server->zonestats);
	isc_stats_detach(&server->sockstats);
	isc_stats_detach(&server->resolverstats);

	if (server->sctx != NULL)
		ns_server_detach(&server->sctx);

	isc_mem_free(server->mctx, server->statsfile);
	isc_mem_free(server->mctx, server->bindkeysfile);
	isc_mem_free(server->mctx, server->dumpfile);
	isc_mem_free(server->mctx, server->secrootsfile);
	isc_mem_free(server->mctx, server->recfile);

	if (server->version != NULL)
		isc_mem_free(server->mctx, server->version);
	if (server->hostname != NULL)
		isc_mem_free(server->mctx, server->hostname);
	if (server->lockfile != NULL)
		isc_mem_free(server->mctx, server->lockfile);

	if (server->zonemgr != NULL)
		dns_zonemgr_detach(&server->zonemgr);

	dst_lib_destroy();

	isc_event_free(&server->reload_event);

	INSIST(ISC_LIST_EMPTY(server->viewlist));
	INSIST(ISC_LIST_EMPTY(server->cachelist));

	server->magic = 0;
	isc_mem_put(server->mctx, server, sizeof(*server));
	*serverp = NULL;
}

static void
fatal(const char *msg, isc_result_t result) {
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_CRITICAL,
		      "%s: %s", msg, isc_result_totext(result));
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_CRITICAL,
		      "exiting (due to fatal error)");
	named_os_shutdown();
	exit(1);
}

static void
start_reserved_dispatches(named_server_t *server) {

	REQUIRE(NAMED_SERVER_VALID(server));

	server->dispatchgen++;
}

static void
end_reserved_dispatches(named_server_t *server, isc_boolean_t all) {
	named_dispatch_t *dispatch, *nextdispatch;

	REQUIRE(NAMED_SERVER_VALID(server));

	for (dispatch = ISC_LIST_HEAD(server->dispatches);
	     dispatch != NULL;
	     dispatch = nextdispatch) {
		nextdispatch = ISC_LIST_NEXT(dispatch, link);
		if (!all && server->dispatchgen == dispatch-> dispatchgen)
			continue;
		ISC_LIST_UNLINK(server->dispatches, dispatch, link);
		dns_dispatch_detach(&dispatch->dispatch);
		isc_mem_put(server->mctx, dispatch, sizeof(*dispatch));
	}
}

void
named_add_reserved_dispatch(named_server_t *server,
			    const isc_sockaddr_t *addr)
{
	named_dispatch_t *dispatch;
	in_port_t port;
	char addrbuf[ISC_SOCKADDR_FORMATSIZE];
	isc_result_t result;
	unsigned int attrs, attrmask;

	REQUIRE(NAMED_SERVER_VALID(server));

	port = isc_sockaddr_getport(addr);
	if (port == 0 || port >= 1024)
		return;

	for (dispatch = ISC_LIST_HEAD(server->dispatches);
	     dispatch != NULL;
	     dispatch = ISC_LIST_NEXT(dispatch, link)) {
		if (isc_sockaddr_equal(&dispatch->addr, addr))
			break;
	}
	if (dispatch != NULL) {
		dispatch->dispatchgen = server->dispatchgen;
		return;
	}

	dispatch = isc_mem_get(server->mctx, sizeof(*dispatch));
	if (dispatch == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	dispatch->addr = *addr;
	dispatch->dispatchgen = server->dispatchgen;
	dispatch->dispatch = NULL;

	attrs = 0;
	attrs |= DNS_DISPATCHATTR_UDP;
	switch (isc_sockaddr_pf(addr)) {
	case AF_INET:
		attrs |= DNS_DISPATCHATTR_IPV4;
		break;
	case AF_INET6:
		attrs |= DNS_DISPATCHATTR_IPV6;
		break;
	default:
		result = ISC_R_NOTIMPLEMENTED;
		goto cleanup;
	}
	attrmask = 0;
	attrmask |= DNS_DISPATCHATTR_UDP;
	attrmask |= DNS_DISPATCHATTR_TCP;
	attrmask |= DNS_DISPATCHATTR_IPV4;
	attrmask |= DNS_DISPATCHATTR_IPV6;

	result = dns_dispatch_getudp(named_g_dispatchmgr, named_g_socketmgr,
				     named_g_taskmgr, &dispatch->addr, 4096,
				     UDPBUFFERS, 32768, 16411, 16433,
				     attrs, attrmask, &dispatch->dispatch);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	ISC_LIST_INITANDPREPEND(server->dispatches, dispatch, link);

	return;

 cleanup:
	if (dispatch != NULL)
		isc_mem_put(server->mctx, dispatch, sizeof(*dispatch));
	isc_sockaddr_format(addr, addrbuf, sizeof(addrbuf));
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
		      "unable to create dispatch for reserved port %s: %s",
		      addrbuf, isc_result_totext(result));
}


static isc_result_t
loadconfig(named_server_t *server) {
	isc_result_t result;
	start_reserved_dispatches(server);
	result = load_configuration(named_g_conffile, server, ISC_FALSE);
	if (result == ISC_R_SUCCESS) {
		end_reserved_dispatches(server, ISC_FALSE);
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "reloading configuration succeeded");
	} else {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "reloading configuration failed: %s",
			      isc_result_totext(result));
	}

	return (result);
}

static isc_result_t
reload(named_server_t *server) {
	isc_result_t result;
	CHECK(loadconfig(server));

	result = load_zones(server, ISC_FALSE, ISC_FALSE);
	if (result == ISC_R_SUCCESS)
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "reloading zones succeeded");
	else
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "reloading zones failed: %s",
			      isc_result_totext(result));

 cleanup:
	return (result);
}

/*
 * Handle a reload event (from SIGHUP).
 */
static void
named_server_reload(isc_task_t *task, isc_event_t *event) {
	named_server_t *server = (named_server_t *)event->ev_arg;

	INSIST(task = server->task);
	UNUSED(task);

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "received SIGHUP signal to reload zones");
	(void)reload(server);

	LOCK(&server->reload_event_lock);
	INSIST(server->reload_event == NULL);
	server->reload_event = event;
	UNLOCK(&server->reload_event_lock);
}

void
named_server_reloadwanted(named_server_t *server) {
	LOCK(&server->reload_event_lock);
	if (server->reload_event != NULL)
		isc_task_send(server->task, &server->reload_event);
	UNLOCK(&server->reload_event_lock);
}

void
named_server_scan_interfaces(named_server_t *server) {
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_DEBUG(1),
		      "automatic interface rescan");

	ns_interfacemgr_scan(server->interfacemgr, ISC_TRUE);
}

/*
 * Get the next token from lexer 'lex'.
 *
 * NOTE: the token value for string tokens always uses the same pointer
 * value.  Multiple calls to this function on the same lexer will always
 * return either that value (lex->data) or NULL. It is necessary to copy
 * the token into local storage if it needs to be referenced after the next
 * call to next_token().
 */
static char *
next_token(isc_lex_t *lex, isc_buffer_t **text) {
	isc_result_t result;
	isc_token_t token;

	token.type = isc_tokentype_unknown;
	result = isc_lex_gettoken(lex, ISC_LEXOPT_EOF|ISC_LEXOPT_QSTRING,
				  &token);

	switch (result) {
	case ISC_R_NOMORE:
		(void) isc_lex_close(lex);
		break;
	case ISC_R_SUCCESS:
		if (token.type == isc_tokentype_eof)
			(void) isc_lex_close(lex);
		break;
	case ISC_R_NOSPACE:
		if (text != NULL) {
			(void) putstr(text, "token too large");
			(void) putnull(text);
		}
		return (NULL);
	default:
		if (text != NULL) {
			(void) putstr(text, isc_result_totext(result));
			(void) putnull(text);
		}
		return (NULL);
	}

	if (token.type == isc_tokentype_string ||
	    token.type == isc_tokentype_qstring)
		return (token.value.as_textregion.base);

	return (NULL);
}

/*
 * Find the zone specified in the control channel command, if any.
 * If a zone is specified, point '*zonep' at it, otherwise
 * set '*zonep' to NULL, and f 'zonename' is not NULL, copy
 * the zone name into it (N.B. 'zonename' must have space to hold
 * a full DNS name).
 *
 * If 'zonetxt' is set, the caller has already pulled a token
 * off the command line that is to be used as the zone name. (This
 * is sometimes done when it's necessary to check for an optional
 * argument before the zone name, as in "rndc sync [-clean] zone".)
 */
static isc_result_t
zone_from_args(named_server_t *server, isc_lex_t *lex, const char *zonetxt,
	       dns_zone_t **zonep, char *zonename,
	       isc_buffer_t **text, isc_boolean_t skip)
{
	char *ptr;
	char *classtxt;
	const char *viewtxt = NULL;
	dns_fixedname_t fname;
	dns_name_t *name;
	isc_result_t result;
	dns_view_t *view = NULL;
	dns_rdataclass_t rdclass;
	char problem[DNS_NAME_FORMATSIZE + 500] = "";
	char zonebuf[DNS_NAME_FORMATSIZE];
	isc_boolean_t redirect = ISC_FALSE;

	REQUIRE(zonep != NULL && *zonep == NULL);

	if (skip) {
		/* Skip the command name. */
		ptr = next_token(lex, text);
		if (ptr == NULL)
			return (ISC_R_UNEXPECTEDEND);
	}

	/* Look for the zone name. */
	if (zonetxt == NULL)
		zonetxt = next_token(lex, text);
	if (zonetxt == NULL)
		return (ISC_R_SUCCESS);

	/* Copy zonetxt because it'll be overwritten by next_token() */
	/* To locate a zone named "-redirect" use "-redirect." */
	if (strcmp(zonetxt, "-redirect") == 0) {
		redirect = ISC_TRUE;
		strlcpy(zonebuf, ".", DNS_NAME_FORMATSIZE);
	} else
		strlcpy(zonebuf, zonetxt, DNS_NAME_FORMATSIZE);
	if (zonename != NULL)
		strlcpy(zonename, redirect ? "." : zonetxt,
			DNS_NAME_FORMATSIZE);

	name = dns_fixedname_initname(&fname);
	CHECK(dns_name_fromstring(name, zonebuf, 0, NULL));

	/* Look for the optional class name. */
	classtxt = next_token(lex, text);
	if (classtxt != NULL) {
		isc_textregion_t r;
		r.base = classtxt;
		r.length = strlen(classtxt);
		CHECK(dns_rdataclass_fromtext(&rdclass, &r));

		/* Look for the optional view name. */
		viewtxt = next_token(lex, text);
	} else
		rdclass = dns_rdataclass_in;

	if (viewtxt == NULL) {
		if (redirect) {
			result = dns_viewlist_find(&server->viewlist,
						   "_default",
						   dns_rdataclass_in,
						   &view);
			if (result != ISC_R_SUCCESS ||
			    view->redirect == NULL)
			{
				result = ISC_R_NOTFOUND;
				snprintf(problem, sizeof(problem),
					 "redirect zone not found in "
					 "_default view");
			} else {
				dns_zone_attach(view->redirect, zonep);
				result = ISC_R_SUCCESS;
			}
		} else {
			result = dns_viewlist_findzone(&server->viewlist,
					  name, ISC_TF(classtxt == NULL),
					  rdclass, zonep);
			if (result == ISC_R_NOTFOUND)
				snprintf(problem, sizeof(problem),
					 "no matching zone '%s' in any view",
					 zonebuf);
			else if (result == ISC_R_MULTIPLE)
				snprintf(problem, sizeof(problem),
					 "zone '%s' was found in multiple "
					 "views", zonebuf);
		}
	} else {
		result = dns_viewlist_find(&server->viewlist, viewtxt,
					   rdclass, &view);
		if (result != ISC_R_SUCCESS) {
			snprintf(problem, sizeof(problem),
				 "no matching view '%s'", viewtxt);
			goto report;
		}

		if (redirect) {
			if (view->redirect != NULL) {
				dns_zone_attach(view->redirect, zonep);
				result = ISC_R_SUCCESS;
			} else
				result = ISC_R_NOTFOUND;
		} else
			result = dns_zt_find(view->zonetable, name, 0,
					     NULL, zonep);
		if (result != ISC_R_SUCCESS)
			snprintf(problem, sizeof(problem),
				 "no matching zone '%s' in view '%s'",
				 zonebuf, viewtxt);
	}

	/* Partial match? */
	if (result != ISC_R_SUCCESS && *zonep != NULL)
		dns_zone_detach(zonep);
	if (result == DNS_R_PARTIALMATCH)
		result = ISC_R_NOTFOUND;
 report:
	if (result != ISC_R_SUCCESS) {
		isc_result_t tresult;

		tresult = putstr(text, problem);
		if (tresult == ISC_R_SUCCESS)
			(void) putnull(text);
	}

 cleanup:
	if (view != NULL)
		dns_view_detach(&view);

	return (result);
}

/*
 * Act on a "retransfer" command from the command channel.
 */
isc_result_t
named_server_retransfercommand(named_server_t *server, isc_lex_t *lex,
			       isc_buffer_t **text)
{
	isc_result_t result;
	dns_zone_t *zone = NULL;
	dns_zone_t *raw = NULL;
	dns_zonetype_t type;

	result = zone_from_args(server, lex, NULL, &zone, NULL,
				text, ISC_TRUE);
	if (result != ISC_R_SUCCESS)
		return (result);
	if (zone == NULL)
		return (ISC_R_UNEXPECTEDEND);
	dns_zone_getraw(zone, &raw);
	if (raw != NULL) {
		dns_zone_detach(&zone);
		dns_zone_attach(raw, &zone);
		dns_zone_detach(&raw);
	}
	type = dns_zone_gettype(zone);
	if (type == dns_zone_slave || type == dns_zone_stub ||
	    (type == dns_zone_redirect &&
	     dns_zone_getredirecttype(zone) == dns_zone_slave))
		dns_zone_forcereload(zone);
	else
		result = ISC_R_NOTFOUND;
	dns_zone_detach(&zone);
	return (result);
}

/*
 * Act on a "reload" command from the command channel.
 */
isc_result_t
named_server_reloadcommand(named_server_t *server, isc_lex_t *lex,
			   isc_buffer_t **text)
{
	isc_result_t result;
	dns_zone_t *zone = NULL;
	dns_zonetype_t type;
	const char *msg = NULL;

	result = zone_from_args(server, lex, NULL, &zone, NULL,
				text, ISC_TRUE);
	if (result != ISC_R_SUCCESS)
		return (result);
	if (zone == NULL) {
		result = reload(server);
		if (result == ISC_R_SUCCESS)
			msg = "server reload successful";
	} else {
		type = dns_zone_gettype(zone);
		if (type == dns_zone_slave || type == dns_zone_stub) {
			dns_zone_refresh(zone);
			dns_zone_detach(&zone);
			msg = "zone refresh queued";
		} else {
			result = dns_zone_load(zone);
			dns_zone_detach(&zone);
			switch (result) {
			case ISC_R_SUCCESS:
				 msg = "zone reload successful";
				 break;
			case DNS_R_CONTINUE:
				msg = "zone reload queued";
				result = ISC_R_SUCCESS;
				break;
			case DNS_R_UPTODATE:
				msg = "zone reload up-to-date";
				result = ISC_R_SUCCESS;
				break;
			default:
				/* failure message will be generated by rndc */
				break;
			}
		}
	}
	if (msg != NULL) {
		(void) putstr(text, msg);
		(void) putnull(text);
	}
	return (result);
}

/*
 * Act on a "reconfig" command from the command channel.
 */
isc_result_t
named_server_reconfigcommand(named_server_t *server) {
	isc_result_t result;

	CHECK(loadconfig(server));

	result = load_zones(server, ISC_FALSE, ISC_TRUE);
	if (result == ISC_R_SUCCESS)
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "scheduled loading new zones");
	else
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "loading new zones failed: %s",
			      isc_result_totext(result));
cleanup:
	return (result);
}

/*
 * Act on a "notify" command from the command channel.
 */
isc_result_t
named_server_notifycommand(named_server_t *server, isc_lex_t *lex,
			   isc_buffer_t **text)
{
	isc_result_t result;
	dns_zone_t *zone = NULL;
	const char msg[] = "zone notify queued";

	result = zone_from_args(server, lex, NULL, &zone, NULL,
				text, ISC_TRUE);
	if (result != ISC_R_SUCCESS)
		return (result);
	if (zone == NULL)
		return (ISC_R_UNEXPECTEDEND);

	dns_zone_notify(zone);
	dns_zone_detach(&zone);
	(void) putstr(text, msg);
	(void) putnull(text);

	return (ISC_R_SUCCESS);
}

/*
 * Act on a "refresh" command from the command channel.
 */
isc_result_t
named_server_refreshcommand(named_server_t *server, isc_lex_t *lex,
			    isc_buffer_t **text)
{
	isc_result_t result;
	dns_zone_t *zone = NULL, *raw = NULL;
	const char msg1[] = "zone refresh queued";
	const char msg2[] = "not a slave or stub zone";
	dns_zonetype_t type;

	result = zone_from_args(server, lex, NULL, &zone, NULL,
				text, ISC_TRUE);
	if (result != ISC_R_SUCCESS)
		return (result);
	if (zone == NULL)
		return (ISC_R_UNEXPECTEDEND);

	dns_zone_getraw(zone, &raw);
	if (raw != NULL) {
		dns_zone_detach(&zone);
		dns_zone_attach(raw, &zone);
		dns_zone_detach(&raw);
	}

	type = dns_zone_gettype(zone);
	if (type == dns_zone_slave || type == dns_zone_stub) {
		dns_zone_refresh(zone);
		dns_zone_detach(&zone);
		(void) putstr(text, msg1);
		(void) putnull(text);
		return (ISC_R_SUCCESS);
	}

	dns_zone_detach(&zone);
	(void) putstr(text, msg2);
	(void) putnull(text);
	return (ISC_R_FAILURE);
}

isc_result_t
named_server_togglequerylog(named_server_t *server, isc_lex_t *lex) {
	isc_boolean_t prev, value;
	char *ptr;

	/* Skip the command name. */
	ptr = next_token(lex, NULL);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	prev = ns_server_getoption(server->sctx, NS_SERVER_LOGQUERIES);

	ptr = next_token(lex, NULL);
	if (ptr == NULL) {
		value = !prev;
	} else if (!strcasecmp(ptr, "on") || !strcasecmp(ptr, "yes") ||
		   !strcasecmp(ptr, "enable") || !strcasecmp(ptr, "true")) {
		value = ISC_TRUE;
	} else if (!strcasecmp(ptr, "off") || !strcasecmp(ptr, "no") ||
		   !strcasecmp(ptr, "disable") || !strcasecmp(ptr, "false")) {
		value = ISC_FALSE;
	} else {
		return (DNS_R_SYNTAX);
	}

	if (value == prev)
		return (ISC_R_SUCCESS);

	ns_server_setoption(server->sctx, NS_SERVER_LOGQUERIES, value);

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "query logging is now %s", value ? "on" : "off");
	return (ISC_R_SUCCESS);
}

static isc_result_t
ns_listenlist_fromconfig(const cfg_obj_t *listenlist, const cfg_obj_t *config,
			 cfg_aclconfctx_t *actx, isc_mem_t *mctx,
			 isc_uint16_t family, ns_listenlist_t **target)
{
	isc_result_t result;
	const cfg_listelt_t *element;
	ns_listenlist_t *dlist = NULL;

	REQUIRE(target != NULL && *target == NULL);

	result = ns_listenlist_create(mctx, &dlist);
	if (result != ISC_R_SUCCESS)
		return (result);

	for (element = cfg_list_first(listenlist);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		ns_listenelt_t *delt = NULL;
		const cfg_obj_t *listener = cfg_listelt_value(element);
		result = ns_listenelt_fromconfig(listener, config, actx,
						 mctx, family, &delt);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		ISC_LIST_APPEND(dlist->elts, delt, link);
	}
	*target = dlist;
	return (ISC_R_SUCCESS);

 cleanup:
	ns_listenlist_detach(&dlist);
	return (result);
}

/*
 * Create a listen list from the corresponding configuration
 * data structure.
 */
static isc_result_t
ns_listenelt_fromconfig(const cfg_obj_t *listener, const cfg_obj_t *config,
			cfg_aclconfctx_t *actx, isc_mem_t *mctx,
			isc_uint16_t family, ns_listenelt_t **target)
{
	isc_result_t result;
	const cfg_obj_t *portobj, *dscpobj;
	in_port_t port;
	isc_dscp_t dscp = -1;
	ns_listenelt_t *delt = NULL;
	REQUIRE(target != NULL && *target == NULL);

	portobj = cfg_tuple_get(listener, "port");
	if (!cfg_obj_isuint32(portobj)) {
		if (named_g_port != 0) {
			port = named_g_port;
		} else {
			result = named_config_getport(config, &port);
			if (result != ISC_R_SUCCESS)
				return (result);
		}
	} else {
		if (cfg_obj_asuint32(portobj) >= ISC_UINT16_MAX) {
			cfg_obj_log(portobj, named_g_lctx, ISC_LOG_ERROR,
				    "port value '%u' is out of range",
				    cfg_obj_asuint32(portobj));
			return (ISC_R_RANGE);
		}
		port = (in_port_t)cfg_obj_asuint32(portobj);
	}

	dscpobj = cfg_tuple_get(listener, "dscp");
	if (!cfg_obj_isuint32(dscpobj))
		dscp = named_g_dscp;
	else {
		if (cfg_obj_asuint32(dscpobj) > 63) {
			cfg_obj_log(dscpobj, named_g_lctx, ISC_LOG_ERROR,
				    "dscp value '%u' is out of range",
				    cfg_obj_asuint32(dscpobj));
			return (ISC_R_RANGE);
		}
		dscp = (isc_dscp_t)cfg_obj_asuint32(dscpobj);
	}

	result = ns_listenelt_create(mctx, port, dscp, NULL, &delt);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = cfg_acl_fromconfig2(cfg_tuple_get(listener, "acl"),
				     config, named_g_lctx, actx, mctx, 0,
				     family, &delt->acl);
	if (result != ISC_R_SUCCESS) {
		ns_listenelt_destroy(delt);
		return (result);
	}
	*target = delt;
	return (ISC_R_SUCCESS);
}

isc_result_t
named_server_dumpstats(named_server_t *server) {
	isc_result_t result;
	FILE *fp = NULL;

	CHECKMF(isc_stdio_open(server->statsfile, "a", &fp),
		"could not open statistics dump file", server->statsfile);

	result = named_stats_dump(server, fp);

 cleanup:
	if (fp != NULL)
		(void)isc_stdio_close(fp);
	if (result == ISC_R_SUCCESS)
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "dumpstats complete");
	else
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "dumpstats failed: %s",
			      dns_result_totext(result));
	return (result);
}

static isc_result_t
add_zone_tolist(dns_zone_t *zone, void *uap) {
	struct dumpcontext *dctx = uap;
	struct zonelistentry *zle;

	zle = isc_mem_get(dctx->mctx, sizeof *zle);
	if (zle ==  NULL)
		return (ISC_R_NOMEMORY);
	zle->zone = NULL;
	dns_zone_attach(zone, &zle->zone);
	ISC_LINK_INIT(zle, link);
	ISC_LIST_APPEND(ISC_LIST_TAIL(dctx->viewlist)->zonelist, zle, link);
	return (ISC_R_SUCCESS);
}

static isc_result_t
add_view_tolist(struct dumpcontext *dctx, dns_view_t *view) {
	struct viewlistentry *vle;
	isc_result_t result = ISC_R_SUCCESS;

	/*
	 * Prevent duplicate views.
	 */
	for (vle = ISC_LIST_HEAD(dctx->viewlist);
	     vle != NULL;
	     vle = ISC_LIST_NEXT(vle, link))
		if (vle->view == view)
			return (ISC_R_SUCCESS);

	vle = isc_mem_get(dctx->mctx, sizeof *vle);
	if (vle == NULL)
		return (ISC_R_NOMEMORY);
	vle->view = NULL;
	dns_view_attach(view, &vle->view);
	ISC_LINK_INIT(vle, link);
	ISC_LIST_INIT(vle->zonelist);
	ISC_LIST_APPEND(dctx->viewlist, vle, link);
	if (dctx->dumpzones)
		result = dns_zt_apply(view->zonetable, ISC_TRUE,
				      add_zone_tolist, dctx);
	return (result);
}

static void
dumpcontext_destroy(struct dumpcontext *dctx) {
	struct viewlistentry *vle;
	struct zonelistentry *zle;

	vle = ISC_LIST_HEAD(dctx->viewlist);
	while (vle != NULL) {
		ISC_LIST_UNLINK(dctx->viewlist, vle, link);
		zle = ISC_LIST_HEAD(vle->zonelist);
		while (zle != NULL) {
			ISC_LIST_UNLINK(vle->zonelist, zle, link);
			dns_zone_detach(&zle->zone);
			isc_mem_put(dctx->mctx, zle, sizeof *zle);
			zle = ISC_LIST_HEAD(vle->zonelist);
		}
		dns_view_detach(&vle->view);
		isc_mem_put(dctx->mctx, vle, sizeof *vle);
		vle = ISC_LIST_HEAD(dctx->viewlist);
	}
	if (dctx->version != NULL)
		dns_db_closeversion(dctx->db, &dctx->version, ISC_FALSE);
	if (dctx->db != NULL)
		dns_db_detach(&dctx->db);
	if (dctx->cache != NULL)
		dns_db_detach(&dctx->cache);
	if (dctx->task != NULL)
		isc_task_detach(&dctx->task);
	if (dctx->fp != NULL)
		(void)isc_stdio_close(dctx->fp);
	if (dctx->mdctx != NULL)
		dns_dumpctx_detach(&dctx->mdctx);
	isc_mem_put(dctx->mctx, dctx, sizeof *dctx);
}

static void
dumpdone(void *arg, isc_result_t result) {
	struct dumpcontext *dctx = arg;
	char buf[1024+32];
	const dns_master_style_t *style;

	if (result != ISC_R_SUCCESS)
		goto cleanup;
	if (dctx->mdctx != NULL)
		dns_dumpctx_detach(&dctx->mdctx);
	if (dctx->view == NULL) {
		dctx->view = ISC_LIST_HEAD(dctx->viewlist);
		if (dctx->view == NULL)
			goto done;
		INSIST(dctx->zone == NULL);
	} else
		goto resume;
 nextview:
	fprintf(dctx->fp, ";\n; Start view %s\n;\n", dctx->view->view->name);
 resume:
	if (dctx->dumpcache && dns_view_iscacheshared(dctx->view->view)) {
		fprintf(dctx->fp,
			";\n; Cache of view '%s' is shared as '%s'\n",
			dctx->view->view->name,
			dns_cache_getname(dctx->view->view->cache));
	} else if (dctx->zone == NULL && dctx->cache == NULL &&
		   dctx->dumpcache)
	{
		style = &dns_master_style_cache;
		/* start cache dump */
		if (dctx->view->view->cachedb != NULL)
			dns_db_attach(dctx->view->view->cachedb, &dctx->cache);
		if (dctx->cache != NULL) {
			fprintf(dctx->fp,
				";\n; Cache dump of view '%s' (cache %s)\n;\n",
				dctx->view->view->name,
				dns_cache_getname(dctx->view->view->cache));
			result = dns_master_dumptostreaminc(dctx->mctx,
							    dctx->cache, NULL,
							    style, dctx->fp,
							    dctx->task,
							    dumpdone, dctx,
							    &dctx->mdctx);
			if (result == DNS_R_CONTINUE)
				return;
			if (result == ISC_R_NOTIMPLEMENTED)
				fprintf(dctx->fp, "; %s\n",
					dns_result_totext(result));
			else if (result != ISC_R_SUCCESS)
				goto cleanup;
		}
	}

	if ((dctx->dumpadb || dctx->dumpbad || dctx->dumpfail) &&
	    dctx->cache == NULL && dctx->view->view->cachedb != NULL)
		dns_db_attach(dctx->view->view->cachedb, &dctx->cache);

	if (dctx->cache != NULL) {
		if (dctx->dumpadb)
			dns_adb_dump(dctx->view->view->adb, dctx->fp);
		if (dctx->dumpbad)
			dns_resolver_printbadcache(dctx->view->view->resolver,
						   dctx->fp);
		if (dctx->dumpfail)
			dns_badcache_print(dctx->view->view->failcache,
					   "SERVFAIL cache", dctx->fp);
		dns_db_detach(&dctx->cache);
	}
	if (dctx->dumpzones) {
		style = &dns_master_style_full;
 nextzone:
		if (dctx->version != NULL)
			dns_db_closeversion(dctx->db, &dctx->version,
					    ISC_FALSE);
		if (dctx->db != NULL)
			dns_db_detach(&dctx->db);
		if (dctx->zone == NULL)
			dctx->zone = ISC_LIST_HEAD(dctx->view->zonelist);
		else
			dctx->zone = ISC_LIST_NEXT(dctx->zone, link);
		if (dctx->zone != NULL) {
			/* start zone dump */
			dns_zone_name(dctx->zone->zone, buf, sizeof(buf));
			fprintf(dctx->fp, ";\n; Zone dump of '%s'\n;\n", buf);
			result = dns_zone_getdb(dctx->zone->zone, &dctx->db);
			if (result != ISC_R_SUCCESS) {
				fprintf(dctx->fp, "; %s\n",
					dns_result_totext(result));
				goto nextzone;
			}
			dns_db_currentversion(dctx->db, &dctx->version);
			result = dns_master_dumptostreaminc(dctx->mctx,
							    dctx->db,
							    dctx->version,
							    style, dctx->fp,
							    dctx->task,
							    dumpdone, dctx,
							    &dctx->mdctx);
			if (result == DNS_R_CONTINUE)
				return;
			if (result == ISC_R_NOTIMPLEMENTED) {
				fprintf(dctx->fp, "; %s\n",
					dns_result_totext(result));
				result = ISC_R_SUCCESS;
				POST(result);
				goto nextzone;
			}
			if (result != ISC_R_SUCCESS)
				goto cleanup;
		}
	}
	if (dctx->view != NULL)
		dctx->view = ISC_LIST_NEXT(dctx->view, link);
	if (dctx->view != NULL)
		goto nextview;
 done:
	fprintf(dctx->fp, "; Dump complete\n");
	result = isc_stdio_flush(dctx->fp);
	if (result == ISC_R_SUCCESS)
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "dumpdb complete");
 cleanup:
	if (result != ISC_R_SUCCESS)
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "dumpdb failed: %s", dns_result_totext(result));
	dumpcontext_destroy(dctx);
}

isc_result_t
named_server_dumpdb(named_server_t *server, isc_lex_t *lex,
		    isc_buffer_t **text)
{
	struct dumpcontext *dctx = NULL;
	dns_view_t *view;
	isc_result_t result;
	char *ptr;
	const char *sep;
	isc_boolean_t found;

	/* Skip the command name. */
	ptr = next_token(lex, NULL);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	dctx = isc_mem_get(server->mctx, sizeof(*dctx));
	if (dctx == NULL)
		return (ISC_R_NOMEMORY);

	dctx->mctx = server->mctx;
	dctx->dumpcache = ISC_TRUE;
	dctx->dumpadb = ISC_TRUE;
	dctx->dumpbad = ISC_TRUE;
	dctx->dumpfail = ISC_TRUE;
	dctx->dumpzones = ISC_FALSE;
	dctx->fp = NULL;
	ISC_LIST_INIT(dctx->viewlist);
	dctx->view = NULL;
	dctx->zone = NULL;
	dctx->cache = NULL;
	dctx->mdctx = NULL;
	dctx->db = NULL;
	dctx->cache = NULL;
	dctx->task = NULL;
	dctx->version = NULL;
	isc_task_attach(server->task, &dctx->task);

	CHECKMF(isc_stdio_open(server->dumpfile, "w", &dctx->fp),
		"could not open dump file", server->dumpfile);

	ptr = next_token(lex, NULL);
	sep = (ptr == NULL) ? "" : ": ";
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "dumpdb started%s%s", sep, (ptr != NULL) ? ptr : "");

	if (ptr != NULL && strcmp(ptr, "-all") == 0) {
		/* also dump zones */
		dctx->dumpzones = ISC_TRUE;
		ptr = next_token(lex, NULL);
	} else if (ptr != NULL && strcmp(ptr, "-cache") == 0) {
		/* this is the default */
		ptr = next_token(lex, NULL);
	} else if (ptr != NULL && strcmp(ptr, "-zones") == 0) {
		/* only dump zones, suppress caches */
		dctx->dumpadb = ISC_FALSE;
		dctx->dumpbad = ISC_FALSE;
		dctx->dumpcache = ISC_FALSE;
		dctx->dumpfail = ISC_FALSE;
		dctx->dumpzones = ISC_TRUE;
		ptr = next_token(lex, NULL);
	} else if (ptr != NULL && strcmp(ptr, "-adb") == 0) {
		/* only dump adb, suppress other caches */
		dctx->dumpbad = ISC_FALSE;
		dctx->dumpcache = ISC_FALSE;
		dctx->dumpfail = ISC_FALSE;
		ptr = next_token(lex, NULL);
	} else if (ptr != NULL && strcmp(ptr, "-bad") == 0) {
		/* only dump badcache, suppress other caches */
		dctx->dumpadb = ISC_FALSE;
		dctx->dumpcache = ISC_FALSE;
		dctx->dumpfail = ISC_FALSE;
		ptr = next_token(lex, NULL);
	} else if (ptr != NULL && strcmp(ptr, "-fail") == 0) {
		/* only dump servfail cache, suppress other caches */
		dctx->dumpadb = ISC_FALSE;
		dctx->dumpbad = ISC_FALSE;
		dctx->dumpcache = ISC_FALSE;
		ptr = next_token(lex, NULL);
	}

 nextview:
	found = ISC_FALSE;
	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		if (ptr != NULL && strcmp(view->name, ptr) != 0)
			continue;
		found = ISC_TRUE;
		CHECK(add_view_tolist(dctx, view));
	}
	if (ptr != NULL) {
		if (!found) {
			putstr(text, "view '");
			putstr(text, ptr);
			putstr(text, "' not found");
			putnull(text);
			result = ISC_R_NOTFOUND;
			dumpdone(dctx, result);
			return (result);
		}
		ptr = next_token(lex, NULL);
		if (ptr != NULL)
			goto nextview;
	}
	dumpdone(dctx, ISC_R_SUCCESS);
	return (ISC_R_SUCCESS);

 cleanup:
	if (dctx != NULL)
		dumpcontext_destroy(dctx);
	return (result);
}

isc_result_t
named_server_dumpsecroots(named_server_t *server, isc_lex_t *lex,
			  isc_buffer_t **text)
{
	dns_view_t *view;
	dns_keytable_t *secroots = NULL;
	dns_ntatable_t *ntatable = NULL;
	isc_result_t result;
	char *ptr;
	FILE *fp = NULL;
	isc_time_t now;
	char tbuf[64];

	/* Skip the command name. */
	ptr = next_token(lex, text);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	/* "-" here means print the output instead of dumping to file */
	ptr = next_token(lex, text);
	if (ptr != NULL && strcmp(ptr, "-") == 0)
		ptr = next_token(lex, text);
	else {
		result = isc_stdio_open(server->secrootsfile, "w", &fp);
		if (result != ISC_R_SUCCESS) {
			(void) putstr(text, "could not open ");
			(void) putstr(text, server->secrootsfile);
			CHECKMF(result, "could not open secroots dump file",
				server->secrootsfile);
		}
	}

	TIME_NOW(&now);
	isc_time_formattimestamp(&now, tbuf, sizeof(tbuf));
	CHECK(putstr(text, "secure roots as of "));
	CHECK(putstr(text, tbuf));
	CHECK(putstr(text, ":\n"));

	do {
		for (view = ISC_LIST_HEAD(server->viewlist);
		     view != NULL;
		     view = ISC_LIST_NEXT(view, link))
		{
			if (ptr != NULL && strcmp(view->name, ptr) != 0)
				continue;
			if (secroots != NULL)
				dns_keytable_detach(&secroots);
			result = dns_view_getsecroots(view, &secroots);
			if (result == ISC_R_NOTFOUND) {
				result = ISC_R_SUCCESS;
				continue;
			}
			CHECK(putstr(text, "\n Start view "));
			CHECK(putstr(text, view->name));
			CHECK(putstr(text, "\n   Secure roots:\n\n"));
			CHECK(dns_keytable_totext(secroots, text));

			if (ntatable != NULL)
				dns_ntatable_detach(&ntatable);
			result = dns_view_getntatable(view, &ntatable);
			if (result == ISC_R_NOTFOUND) {
				result = ISC_R_SUCCESS;
				continue;
			}
			CHECK(putstr(text, "\n   Negative trust anchors:\n\n"));
			CHECK(dns_ntatable_totext(ntatable, text));
		}
		if (ptr != NULL)
			ptr = next_token(lex, text);
	} while (ptr != NULL);

 cleanup:
	if (isc_buffer_usedlength(*text) > 0) {
		if (fp != NULL)
			(void)putstr(text, "\n");
		else
			(void)putnull(text);
	}
	if (secroots != NULL)
		dns_keytable_detach(&secroots);
	if (ntatable != NULL)
		dns_ntatable_detach(&ntatable);
	if (fp != NULL) {
		fprintf(fp, "%.*s", (int) isc_buffer_usedlength(*text),
			(char *) isc_buffer_base(*text));
		isc_buffer_clear(*text);
		(void)isc_stdio_close(fp);
	}
	if (result == ISC_R_SUCCESS)
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "dumpsecroots complete");
	else
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "dumpsecroots failed: %s",
			      dns_result_totext(result));
	return (result);
}

isc_result_t
named_server_dumprecursing(named_server_t *server) {
	FILE *fp = NULL;
	dns_view_t *view;
	isc_result_t result;

	CHECKMF(isc_stdio_open(server->recfile, "w", &fp),
		"could not open dump file", server->recfile);
	fprintf(fp, ";\n; Recursing Queries\n;\n");
	ns_interfacemgr_dumprecursing(fp, server->interfacemgr);

	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		fprintf(fp, ";\n; Active fetch domains [view: %s]\n;\n",
			view->name);
		dns_resolver_dumpfetches(view->resolver,
					 isc_statsformat_file, fp);
	}

	fprintf(fp, "; Dump complete\n");

 cleanup:
	if (fp != NULL)
		result = isc_stdio_close(fp);
	if (result == ISC_R_SUCCESS)
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "dumprecursing complete");
	else
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "dumprecursing failed: %s",
			      dns_result_totext(result));
	return (result);
}

isc_result_t
named_server_setdebuglevel(named_server_t *server, isc_lex_t *lex) {
	char *ptr;
	char *endp;
	long newlevel;

	UNUSED(server);

	/* Skip the command name. */
	ptr = next_token(lex, NULL);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	/* Look for the new level name. */
	ptr = next_token(lex, NULL);
	if (ptr == NULL) {
		if (named_g_debuglevel < 99)
			named_g_debuglevel++;
	} else {
		newlevel = strtol(ptr, &endp, 10);
		if (*endp != '\0' || newlevel < 0 || newlevel > 99)
			return (ISC_R_RANGE);
		named_g_debuglevel = (unsigned int)newlevel;
	}
	isc_log_setdebuglevel(named_g_lctx, named_g_debuglevel);
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "debug level is now %u", named_g_debuglevel);
	return (ISC_R_SUCCESS);
}

isc_result_t
named_server_validation(named_server_t *server, isc_lex_t *lex,
			isc_buffer_t **text)
{
	char *ptr;
	dns_view_t *view;
	isc_boolean_t changed = ISC_FALSE;
	isc_result_t result;
	isc_boolean_t enable = ISC_TRUE, set = ISC_TRUE, first = ISC_TRUE;

	/* Skip the command name. */
	ptr = next_token(lex, text);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	/* Find out what we are to do. */
	ptr = next_token(lex, text);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	if (!strcasecmp(ptr, "on") || !strcasecmp(ptr, "yes") ||
	    !strcasecmp(ptr, "enable") || !strcasecmp(ptr, "true")) {
		enable = ISC_TRUE;
	} else if (!strcasecmp(ptr, "off") || !strcasecmp(ptr, "no") ||
		   !strcasecmp(ptr, "disable") || !strcasecmp(ptr, "false")) {
		enable = ISC_FALSE;
	} else if (!strcasecmp(ptr, "check") || !strcasecmp(ptr, "status")) {
		set = ISC_FALSE;
	} else {
		return (DNS_R_SYNTAX);
	}

	/* Look for the view name. */
	ptr = next_token(lex, text);

	result = isc_task_beginexclusive(server->task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		if (ptr != NULL && strcasecmp(ptr, view->name) != 0)
			continue;
		CHECK(dns_view_flushcache(view));

		if (set) {
			view->enablevalidation = enable;
			changed = ISC_TRUE;
		} else {
			if (!first)
				CHECK(putstr(text, "\n"));
			CHECK(putstr(text, "DNSSEC validation is "));
			CHECK(putstr(text, view->enablevalidation
					    ? "enabled" : "disabled"));
			CHECK(putstr(text, " (view "));
			CHECK(putstr(text, view->name));
			CHECK(putstr(text, ")"));
			CHECK(putnull(text));
			first = ISC_FALSE;
		}
	}

	if (!set)
		result = ISC_R_SUCCESS;
	else if (changed)
		result = ISC_R_SUCCESS;
	else
		result = ISC_R_FAILURE;
 cleanup:
	isc_task_endexclusive(server->task);
	return (result);
}

isc_result_t
named_server_flushcache(named_server_t *server, isc_lex_t *lex) {
	char *ptr;
	dns_view_t *view;
	isc_boolean_t flushed;
	isc_boolean_t found;
	isc_result_t result;
	named_cache_t *nsc;

	/* Skip the command name. */
	ptr = next_token(lex, NULL);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	/* Look for the view name. */
	ptr = next_token(lex, NULL);

	result = isc_task_beginexclusive(server->task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	flushed = ISC_TRUE;
	found = ISC_FALSE;

	/*
	 * Flushing a cache is tricky when caches are shared by multiple views.
	 * We first identify which caches should be flushed in the local cache
	 * list, flush these caches, and then update other views that refer to
	 * the flushed cache DB.
	 */
	if (ptr != NULL) {
		/*
		 * Mark caches that need to be flushed.  This is an O(#view^2)
		 * operation in the very worst case, but should be normally
		 * much more lightweight because only a few (most typically just
		 * one) views will match.
		 */
		for (view = ISC_LIST_HEAD(server->viewlist);
		     view != NULL;
		     view = ISC_LIST_NEXT(view, link))
		{
			if (strcasecmp(ptr, view->name) != 0)
				continue;
			found = ISC_TRUE;
			for (nsc = ISC_LIST_HEAD(server->cachelist);
			     nsc != NULL;
			     nsc = ISC_LIST_NEXT(nsc, link)) {
				if (nsc->cache == view->cache)
					break;
			}
			INSIST(nsc != NULL);
			nsc->needflush = ISC_TRUE;
		}
	} else
		found = ISC_TRUE;

	/* Perform flush */
	for (nsc = ISC_LIST_HEAD(server->cachelist);
	     nsc != NULL;
	     nsc = ISC_LIST_NEXT(nsc, link)) {
		if (ptr != NULL && !nsc->needflush)
			continue;
		nsc->needflush = ISC_TRUE;
		result = dns_view_flushcache2(nsc->primaryview, ISC_FALSE);
		if (result != ISC_R_SUCCESS) {
			flushed = ISC_FALSE;
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "flushing cache in view '%s' failed: %s",
				      nsc->primaryview->name,
				      isc_result_totext(result));
		}
	}

	/*
	 * Fix up views that share a flushed cache: let the views update the
	 * cache DB they're referring to.  This could also be an expensive
	 * operation, but should typically be marginal: the inner loop is only
	 * necessary for views that share a cache, and if there are many such
	 * views the number of shared cache should normally be small.
	 * A worst case is that we have n views and n/2 caches, each shared by
	 * two views.  Then this will be a O(n^2/4) operation.
	 */
	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		if (!dns_view_iscacheshared(view))
			continue;
		for (nsc = ISC_LIST_HEAD(server->cachelist);
		     nsc != NULL;
		     nsc = ISC_LIST_NEXT(nsc, link)) {
			if (!nsc->needflush || nsc->cache != view->cache)
				continue;
			result = dns_view_flushcache2(view, ISC_TRUE);
			if (result != ISC_R_SUCCESS) {
				flushed = ISC_FALSE;
				isc_log_write(named_g_lctx,
					      NAMED_LOGCATEGORY_GENERAL,
					      NAMED_LOGMODULE_SERVER,
					      ISC_LOG_ERROR,
					      "fixing cache in view '%s' "
					      "failed: %s", view->name,
					      isc_result_totext(result));
			}
		}
	}

	/* Cleanup the cache list. */
	for (nsc = ISC_LIST_HEAD(server->cachelist);
	     nsc != NULL;
	     nsc = ISC_LIST_NEXT(nsc, link)) {
		nsc->needflush = ISC_FALSE;
	}

	if (flushed && found) {
		if (ptr != NULL)
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
				      "flushing cache in view '%s' succeeded",
				      ptr);
		else
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
				      "flushing caches in all views succeeded");
		result = ISC_R_SUCCESS;
	} else {
		if (!found) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "flushing cache in view '%s' failed: "
				      "view not found", ptr);
			result = ISC_R_NOTFOUND;
		} else
			result = ISC_R_FAILURE;
	}
	isc_task_endexclusive(server->task);
	return (result);
}

isc_result_t
named_server_flushnode(named_server_t *server, isc_lex_t *lex,
		       isc_boolean_t tree)
{
	char *ptr, *viewname;
	char target[DNS_NAME_FORMATSIZE];
	dns_view_t *view;
	isc_boolean_t flushed;
	isc_boolean_t found;
	isc_result_t result;
	isc_buffer_t b;
	dns_fixedname_t fixed;
	dns_name_t *name;

	/* Skip the command name. */
	ptr = next_token(lex, NULL);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	/* Find the domain name to flush. */
	ptr = next_token(lex, NULL);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	strlcpy(target, ptr, DNS_NAME_FORMATSIZE);
	isc_buffer_constinit(&b, target, strlen(target));
	isc_buffer_add(&b, strlen(target));
	name = dns_fixedname_initname(&fixed);
	result = dns_name_fromtext(name, &b, dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS)
		return (result);

	/* Look for the view name. */
	viewname = next_token(lex, NULL);

	result = isc_task_beginexclusive(server->task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	flushed = ISC_TRUE;
	found = ISC_FALSE;
	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		if (viewname != NULL && strcasecmp(viewname, view->name) != 0)
			continue;
		found = ISC_TRUE;
		/*
		 * It's a little inefficient to try flushing name for all views
		 * if some of the views share a single cache.  But since the
		 * operation is lightweight we prefer simplicity here.
		 */
		result = dns_view_flushnode(view, name, tree);
		if (result != ISC_R_SUCCESS) {
			flushed = ISC_FALSE;
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "flushing %s '%s' in cache view '%s' "
				      "failed: %s",
				      tree ? "tree" : "name",
				      target, view->name,
				      isc_result_totext(result));
		}
	}
	if (flushed && found) {
		if (viewname != NULL)
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
				      "flushing %s '%s' in cache view '%s' "
				      "succeeded",
				      tree ? "tree" : "name",
				      target, viewname);
		else
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
				      "flushing %s '%s' in all cache views "
				      "succeeded",
				      tree ? "tree" : "name",
				      target);
		result = ISC_R_SUCCESS;
	} else {
		if (!found)
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "flushing %s '%s' in cache view '%s' "
				      "failed: view not found",
				      tree ? "tree" : "name",
				      target, viewname);
		result = ISC_R_FAILURE;
	}
	isc_task_endexclusive(server->task);
	return (result);
}

isc_result_t
named_server_status(named_server_t *server, isc_buffer_t **text) {
	isc_result_t result;
	unsigned int zonecount, xferrunning, xferdeferred, soaqueries;
	unsigned int automatic;
	const char *ob = "", *cb = "", *alt = "";
	char boottime[ISC_FORMATHTTPTIMESTAMP_SIZE];
	char configtime[ISC_FORMATHTTPTIMESTAMP_SIZE];
	char line[1024], hostname[256];

	if (named_g_server->version_set) {
		ob = " (";
		cb = ")";
		if (named_g_server->version == NULL)
			alt = "version.bind/txt/ch disabled";
		else
			alt = named_g_server->version;
	}
	zonecount = dns_zonemgr_getcount(server->zonemgr, DNS_ZONESTATE_ANY);
	xferrunning = dns_zonemgr_getcount(server->zonemgr,
					   DNS_ZONESTATE_XFERRUNNING);
	xferdeferred = dns_zonemgr_getcount(server->zonemgr,
					    DNS_ZONESTATE_XFERDEFERRED);
	soaqueries = dns_zonemgr_getcount(server->zonemgr,
					  DNS_ZONESTATE_SOAQUERY);
	automatic = dns_zonemgr_getcount(server->zonemgr,
					 DNS_ZONESTATE_AUTOMATIC);

	isc_time_formathttptimestamp(&named_g_boottime, boottime,
				     sizeof(boottime));
	isc_time_formathttptimestamp(&named_g_configtime, configtime,
				     sizeof(configtime));

	snprintf(line, sizeof(line), "version: %s %s%s%s <id:%s>%s%s%s\n",
		 named_g_product, named_g_version,
		 (*named_g_description != '\0') ? " " : "",
		 named_g_description, named_g_srcid, ob, alt, cb);
	CHECK(putstr(text, line));

	result = named_os_gethostname(hostname, sizeof(hostname));
	if (result != ISC_R_SUCCESS)
		strlcpy(hostname, "localhost", sizeof(hostname));
	snprintf(line, sizeof(line), "running on %s: %s\n",
		 hostname, named_os_uname());
	CHECK(putstr(text, line));

	snprintf(line, sizeof(line), "boot time: %s\n", boottime);
	CHECK(putstr(text, line));

	snprintf(line, sizeof(line), "last configured: %s\n", configtime);
	CHECK(putstr(text, line));

	if (named_g_chrootdir != NULL) {
		snprintf(line, sizeof(line), "configuration file: %s (%s%s)\n",
			 named_g_conffile, named_g_chrootdir, named_g_conffile);
	} else {
		snprintf(line, sizeof(line), "configuration file: %s\n",
			 named_g_conffile);
	}
	CHECK(putstr(text, line));

#ifdef ISC_PLATFORM_USETHREADS
	snprintf(line, sizeof(line), "CPUs found: %u\n", named_g_cpus_detected);
	CHECK(putstr(text, line));

	snprintf(line, sizeof(line), "worker threads: %u\n", named_g_cpus);
	CHECK(putstr(text, line));

	snprintf(line, sizeof(line), "UDP listeners per interface: %u\n",
		 named_g_udpdisp);
	CHECK(putstr(text, line));
#else
	snprintf(line, sizeof(line), "CPUs found: N/A (threads disabled)\n");
	CHECK(putstr(text, line));
#endif

	snprintf(line, sizeof(line), "number of zones: %u (%u automatic)\n",
		     zonecount, automatic);
	CHECK(putstr(text, line));

	snprintf(line, sizeof(line), "debug level: %u\n", named_g_debuglevel);
	CHECK(putstr(text, line));

	snprintf(line, sizeof(line), "xfers running: %u\n", xferrunning);
	CHECK(putstr(text, line));

	snprintf(line, sizeof(line), "xfers deferred: %u\n", xferdeferred);
	CHECK(putstr(text, line));

	snprintf(line, sizeof(line), "soa queries in progress: %u\n",
		     soaqueries);
	CHECK(putstr(text, line));

	snprintf(line, sizeof(line), "query logging is %s\n",
		 ns_server_getoption(server->sctx, NS_SERVER_LOGQUERIES)
		   ? "ON" : "OFF");
	CHECK(putstr(text, line));

	snprintf(line, sizeof(line), "recursive clients: %d/%d/%d\n",
		     server->sctx->recursionquota.used,
		     server->sctx->recursionquota.soft,
		     server->sctx->recursionquota.max);
	CHECK(putstr(text, line));

	snprintf(line, sizeof(line), "tcp clients: %d/%d\n",
		     server->sctx->tcpquota.used, server->sctx->tcpquota.max);
	CHECK(putstr(text, line));

	CHECK(putstr(text, "server is up and running"));
	CHECK(putnull(text));

	return (ISC_R_SUCCESS);
 cleanup:
	return (result);
}

isc_result_t
named_server_testgen(isc_lex_t *lex, isc_buffer_t **text) {
	isc_result_t result;
	char *ptr;
	unsigned long count;
	unsigned long i;
	const unsigned char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";

	/* Skip the command name. */
	ptr = next_token(lex, text);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	ptr = next_token(lex, text);
	if (ptr == NULL)
		count = 26;
	else
		count = strtoul(ptr, NULL, 10);

	CHECK(isc_buffer_reserve(text, count));
	for (i = 0; i < count; i++)
		CHECK(putuint8(text, chars[i % (sizeof(chars) - 1)]));

	CHECK(putnull(text));

 cleanup:
	return (result);
}

static isc_result_t
delete_keynames(dns_tsig_keyring_t *ring, char *target,
		unsigned int *foundkeys)
{
	char namestr[DNS_NAME_FORMATSIZE];
	isc_result_t result;
	dns_rbtnodechain_t chain;
	dns_name_t foundname;
	dns_fixedname_t fixedorigin;
	dns_name_t *origin;
	dns_rbtnode_t *node;
	dns_tsigkey_t *tkey;

	dns_name_init(&foundname, NULL);
	origin = dns_fixedname_initname(&fixedorigin);

 again:
	dns_rbtnodechain_init(&chain, ring->mctx);
	result = dns_rbtnodechain_first(&chain, ring->keys, &foundname,
					origin);
	if (result == ISC_R_NOTFOUND) {
		dns_rbtnodechain_invalidate(&chain);
		return (ISC_R_SUCCESS);
	}
	if (result != ISC_R_SUCCESS && result != DNS_R_NEWORIGIN) {
		dns_rbtnodechain_invalidate(&chain);
		return (result);
	}

	for (;;) {
		node = NULL;
		dns_rbtnodechain_current(&chain, &foundname, origin, &node);
		tkey = node->data;

		if (tkey != NULL) {
			if (!tkey->generated)
				goto nextkey;

			dns_name_format(&tkey->name, namestr, sizeof(namestr));
			if (strcmp(namestr, target) == 0) {
				(*foundkeys)++;
				dns_rbtnodechain_invalidate(&chain);
				(void)dns_rbt_deletename(ring->keys,
							 &tkey->name,
							 ISC_FALSE);
				goto again;
			}
		}

	nextkey:
		result = dns_rbtnodechain_next(&chain, &foundname, origin);
		if (result == ISC_R_NOMORE)
			break;
		if (result != ISC_R_SUCCESS && result != DNS_R_NEWORIGIN) {
			dns_rbtnodechain_invalidate(&chain);
			return (result);
		}
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
named_server_tsigdelete(named_server_t *server, isc_lex_t *lex,
			isc_buffer_t **text)
{
	isc_result_t result;
	dns_view_t *view;
	unsigned int foundkeys = 0;
	char *ptr, *viewname;
	char target[DNS_NAME_FORMATSIZE];
	char fbuf[16];

	(void)next_token(lex, text);  /* skip command name */

	ptr = next_token(lex, text);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);
	strlcpy(target, ptr, DNS_NAME_FORMATSIZE);

	viewname = next_token(lex, text);

	result = isc_task_beginexclusive(server->task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link)) {
		if (viewname == NULL || strcmp(view->name, viewname) == 0) {
			RWLOCK(&view->dynamickeys->lock, isc_rwlocktype_write);
			result = delete_keynames(view->dynamickeys, target,
						 &foundkeys);
			RWUNLOCK(&view->dynamickeys->lock,
				 isc_rwlocktype_write);
			if (result != ISC_R_SUCCESS) {
				isc_task_endexclusive(server->task);
				return (result);
			}
		}
	}
	isc_task_endexclusive(server->task);

	snprintf(fbuf, sizeof(fbuf), "%u", foundkeys);

	CHECK(putstr(text, fbuf));
	CHECK(putstr(text, " tsig keys deleted."));
	CHECK(putnull(text));

 cleanup:
	return (result);
}

static isc_result_t
list_keynames(dns_view_t *view, dns_tsig_keyring_t *ring, isc_buffer_t **text,
	      unsigned int *foundkeys)
{
	char namestr[DNS_NAME_FORMATSIZE];
	char creatorstr[DNS_NAME_FORMATSIZE];
	isc_result_t result;
	dns_rbtnodechain_t chain;
	dns_name_t foundname;
	dns_fixedname_t fixedorigin;
	dns_name_t *origin;
	dns_rbtnode_t *node;
	dns_tsigkey_t *tkey;
	const char *viewname;

	if (view != NULL)
		viewname = view->name;
	else
		viewname = "(global)";

	dns_name_init(&foundname, NULL);
	origin = dns_fixedname_initname(&fixedorigin);
	dns_rbtnodechain_init(&chain, ring->mctx);
	result = dns_rbtnodechain_first(&chain, ring->keys, &foundname,
					origin);
	if (result == ISC_R_NOTFOUND) {
		dns_rbtnodechain_invalidate(&chain);
		return (ISC_R_SUCCESS);
	}
	if (result != ISC_R_SUCCESS && result != DNS_R_NEWORIGIN) {
		dns_rbtnodechain_invalidate(&chain);
		return (result);
	}

	for (;;) {
		node = NULL;
		dns_rbtnodechain_current(&chain, &foundname, origin, &node);
		tkey = node->data;

		if (tkey != NULL) {
			dns_name_format(&tkey->name, namestr, sizeof(namestr));
			if (tkey->generated) {
				dns_name_format(tkey->creator, creatorstr,
						sizeof(creatorstr));
				if (*foundkeys != 0)
					CHECK(putstr(text, "\n"));
				CHECK(putstr(text, "view \""));
				CHECK(putstr(text, viewname));
				CHECK(putstr(text,
					     "\"; type \"dynamic\"; key \""));
				CHECK(putstr(text, namestr));
				CHECK(putstr(text, "\"; creator \""));
				CHECK(putstr(text, creatorstr));
				CHECK(putstr(text, "\";"));
			} else {
				if (*foundkeys != 0)
					CHECK(putstr(text, "\n"));
				CHECK(putstr(text, "view \""));
				CHECK(putstr(text, viewname));
				CHECK(putstr(text,
					     "\"; type \"static\"; key \""));
				CHECK(putstr(text, namestr));
				CHECK(putstr(text, "\";"));
			}
			(*foundkeys)++;
		}
		result = dns_rbtnodechain_next(&chain, &foundname, origin);
		if (result == ISC_R_NOMORE || result == DNS_R_NEWORIGIN)
			break;
	}

	return (ISC_R_SUCCESS);
 cleanup:
	dns_rbtnodechain_invalidate(&chain);
	return (result);
}

isc_result_t
named_server_tsiglist(named_server_t *server, isc_buffer_t **text) {
	isc_result_t result;
	dns_view_t *view;
	unsigned int foundkeys = 0;

	result = isc_task_beginexclusive(server->task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link)) {
		RWLOCK(&view->statickeys->lock, isc_rwlocktype_read);
		result = list_keynames(view, view->statickeys, text,
				       &foundkeys);
		RWUNLOCK(&view->statickeys->lock, isc_rwlocktype_read);
		if (result != ISC_R_SUCCESS) {
			isc_task_endexclusive(server->task);
			return (result);
		}
		RWLOCK(&view->dynamickeys->lock, isc_rwlocktype_read);
		result = list_keynames(view, view->dynamickeys, text,
				       &foundkeys);
		RWUNLOCK(&view->dynamickeys->lock, isc_rwlocktype_read);
		if (result != ISC_R_SUCCESS) {
			isc_task_endexclusive(server->task);
			return (result);
		}
	}
	isc_task_endexclusive(server->task);

	if (foundkeys == 0)
		CHECK(putstr(text, "no tsig keys found."));

	if (isc_buffer_usedlength(*text) > 0)
		CHECK(putnull(text));

 cleanup:
	return (result);
}

/*
 * Act on a "sign" or "loadkeys" command from the command channel.
 */
isc_result_t
named_server_rekey(named_server_t *server, isc_lex_t *lex,
		   isc_buffer_t **text)
{
	isc_result_t result;
	dns_zone_t *zone = NULL;
	dns_zonetype_t type;
	isc_uint16_t keyopts;
	isc_boolean_t fullsign = ISC_FALSE;
	char *ptr;

	ptr = next_token(lex, text);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	if (strcasecmp(ptr, NAMED_COMMAND_SIGN) == 0)
		fullsign = ISC_TRUE;

	result = zone_from_args(server, lex, NULL, &zone, NULL,
				text, ISC_FALSE);
	if (result != ISC_R_SUCCESS)
		return (result);
	if (zone == NULL)
		return (ISC_R_UNEXPECTEDEND);   /* XXX: or do all zones? */

	type = dns_zone_gettype(zone);
	if (type != dns_zone_master) {
		dns_zone_detach(&zone);
		return (DNS_R_NOTMASTER);
	}

	keyopts = dns_zone_getkeyopts(zone);

	/* "rndc loadkeys" requires "auto-dnssec maintain". */
	if ((keyopts & DNS_ZONEKEY_ALLOW) == 0)
		result = ISC_R_NOPERM;
	else if ((keyopts & DNS_ZONEKEY_MAINTAIN) == 0 && !fullsign)
		result = ISC_R_NOPERM;
	else
		dns_zone_rekey(zone, fullsign);

	dns_zone_detach(&zone);
	return (result);
}

/*
 * Act on a "sync" command from the command channel.
*/
static isc_result_t
synczone(dns_zone_t *zone, void *uap) {
	isc_boolean_t cleanup = *(isc_boolean_t *)uap;
	isc_result_t result;
	dns_zone_t *raw = NULL;
	char *journal;

	dns_zone_getraw(zone, &raw);
	if (raw != NULL) {
		synczone(raw, uap);
		dns_zone_detach(&raw);
	}

	result = dns_zone_flush(zone);
	if (result != ISC_R_SUCCESS)
		cleanup = ISC_FALSE;
	if (cleanup) {
		journal = dns_zone_getjournal(zone);
		if (journal != NULL)
			(void)isc_file_remove(journal);
	}

	return (result);
}

isc_result_t
named_server_sync(named_server_t *server, isc_lex_t *lex, isc_buffer_t **text) {
	isc_result_t result, tresult;
	dns_view_t *view;
	dns_zone_t *zone = NULL;
	char classstr[DNS_RDATACLASS_FORMATSIZE];
	char zonename[DNS_NAME_FORMATSIZE];
	const char *vname, *sep, *arg;
	isc_boolean_t cleanup = ISC_FALSE;

	(void) next_token(lex, text);

	arg = next_token(lex, text);
	if (arg != NULL &&
	    (strcmp(arg, "-clean") == 0 || strcmp(arg, "-clear") == 0)) {
		cleanup = ISC_TRUE;
		arg = next_token(lex, text);
	}

	result = zone_from_args(server, lex, arg, &zone, NULL,
				text, ISC_FALSE);
	if (result != ISC_R_SUCCESS)
		return (result);

	if (zone == NULL) {
		result = isc_task_beginexclusive(server->task);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		tresult = ISC_R_SUCCESS;
		for (view = ISC_LIST_HEAD(server->viewlist);
		     view != NULL;
		     view = ISC_LIST_NEXT(view, link)) {
			result = dns_zt_apply(view->zonetable, ISC_FALSE,
					      synczone, &cleanup);
			if (result != ISC_R_SUCCESS &&
			    tresult == ISC_R_SUCCESS)
				tresult = result;
		}
		isc_task_endexclusive(server->task);
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "dumping all zones%s: %s",
			      cleanup ? ", removing journal files" : "",
			      isc_result_totext(result));
		return (tresult);
	}

	result = isc_task_beginexclusive(server->task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	result = synczone(zone, &cleanup);
	isc_task_endexclusive(server->task);

	view = dns_zone_getview(zone);
	if (strcmp(view->name, "_default") == 0 ||
	    strcmp(view->name, "_bind") == 0)
	{
		vname = "";
		sep = "";
	} else {
		vname = view->name;
		sep = " ";
	}
	dns_rdataclass_format(dns_zone_getclass(zone), classstr,
			      sizeof(classstr));
	dns_name_format(dns_zone_getorigin(zone),
			zonename, sizeof(zonename));
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "sync: dumping zone '%s/%s'%s%s%s: %s",
		      zonename, classstr, sep, vname,
		      cleanup ? ", removing journal file" : "",
		      isc_result_totext(result));
	dns_zone_detach(&zone);
	return (result);
}

/*
 * Act on a "freeze" or "thaw" command from the command channel.
 */
isc_result_t
named_server_freeze(named_server_t *server, isc_boolean_t freeze,
		    isc_lex_t *lex, isc_buffer_t **text)
{
	isc_result_t result, tresult;
	dns_zone_t *zone = NULL, *raw = NULL;
	dns_zonetype_t type;
	char classstr[DNS_RDATACLASS_FORMATSIZE];
	char zonename[DNS_NAME_FORMATSIZE];
	dns_view_t *view;
	const char *vname, *sep;
	isc_boolean_t frozen;
	const char *msg = NULL;

	result = zone_from_args(server, lex, NULL, &zone, NULL,
				text, ISC_TRUE);
	if (result != ISC_R_SUCCESS)
		return (result);
	if (zone == NULL) {
		result = isc_task_beginexclusive(server->task);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		tresult = ISC_R_SUCCESS;
		for (view = ISC_LIST_HEAD(server->viewlist);
		     view != NULL;
		     view = ISC_LIST_NEXT(view, link)) {
			result = dns_view_freezezones(view, freeze);
			if (result != ISC_R_SUCCESS &&
			    tresult == ISC_R_SUCCESS)
				tresult = result;
		}
		isc_task_endexclusive(server->task);
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "%s all zones: %s",
			      freeze ? "freezing" : "thawing",
			      isc_result_totext(tresult));
		return (tresult);
	}
	dns_zone_getraw(zone, &raw);
	if (raw != NULL) {
		dns_zone_detach(&zone);
		dns_zone_attach(raw, &zone);
		dns_zone_detach(&raw);
	}
	type = dns_zone_gettype(zone);
	if (type != dns_zone_master) {
		dns_zone_detach(&zone);
		return (DNS_R_NOTMASTER);
	}

	if (freeze && !dns_zone_isdynamic(zone, ISC_TRUE)) {
		dns_zone_detach(&zone);
		return (DNS_R_NOTDYNAMIC);
	}

	result = isc_task_beginexclusive(server->task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	frozen = dns_zone_getupdatedisabled(zone);
	if (freeze) {
		if (frozen) {
			msg = "WARNING: The zone was already frozen.\n"
			      "Someone else may be editing it or "
			      "it may still be re-loading.";
			result = DNS_R_FROZEN;
		}
		if (result == ISC_R_SUCCESS) {
			result = dns_zone_flush(zone);
			if (result != ISC_R_SUCCESS)
				msg = "Flushing the zone updates to "
				      "disk failed.";
		}
		if (result == ISC_R_SUCCESS)
			dns_zone_setupdatedisabled(zone, freeze);
	} else {
		if (frozen) {
			result = dns_zone_loadandthaw(zone);
			switch (result) {
			case ISC_R_SUCCESS:
			case DNS_R_UPTODATE:
				msg = "The zone reload and thaw was "
				      "successful.";
				result = ISC_R_SUCCESS;
				break;
			case DNS_R_CONTINUE:
				msg = "A zone reload and thaw was started.\n"
				      "Check the logs to see the result.";
				result = ISC_R_SUCCESS;
				break;
			}
		}
	}
	isc_task_endexclusive(server->task);

	if (msg != NULL) {
		(void) putstr(text, msg);
		(void) putnull(text);
	}

	view = dns_zone_getview(zone);
	if (strcmp(view->name, "_default") == 0 ||
	    strcmp(view->name, "_bind") == 0)
	{
		vname = "";
		sep = "";
	} else {
		vname = view->name;
		sep = " ";
	}
	dns_rdataclass_format(dns_zone_getclass(zone), classstr,
			      sizeof(classstr));
	dns_name_format(dns_zone_getorigin(zone),
			zonename, sizeof(zonename));
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "%s zone '%s/%s'%s%s: %s",
		      freeze ? "freezing" : "thawing",
		      zonename, classstr, sep, vname,
		      isc_result_totext(result));
	dns_zone_detach(&zone);
	return (result);
}

#ifdef HAVE_LIBSCF
/*
 * This function adds a message for rndc to echo if named
 * is managed by smf and is also running chroot.
 */
isc_result_t
named_smf_add_message(isc_buffer_t **text) {
	return (putstr(text, "use svcadm(1M) to manage named"));
}
#endif /* HAVE_LIBSCF */

#ifndef HAVE_LMDB

/*
 * Emit a comment at the top of the nzf file containing the viewname
 * Expects the fp to already be open for writing
 */
#define HEADER1 "# New zone file for view: "
#define HEADER2 "\n# This file contains configuration for zones added by\n" \
		"# the 'rndc addzone' command. DO NOT EDIT BY HAND.\n"
static isc_result_t
add_comment(FILE *fp, const char *viewname) {
	isc_result_t result;
	CHECK(isc_stdio_write(HEADER1, sizeof(HEADER1) - 1, 1, fp, NULL));
	CHECK(isc_stdio_write(viewname, strlen(viewname), 1, fp, NULL));
	CHECK(isc_stdio_write(HEADER2, sizeof(HEADER2) - 1, 1, fp, NULL));
 cleanup:
	return (result);
}

static void
dumpzone(void *arg, const char *buf, int len) {
	FILE *fp = arg;

	(void) isc_stdio_write(buf, len, 1, fp, NULL);
}

static isc_result_t
nzf_append(dns_view_t *view, const cfg_obj_t *zconfig) {
	isc_result_t result;
	off_t offset;
	FILE *fp = NULL;
	isc_boolean_t offsetok = ISC_FALSE;

	LOCK(&view->new_zone_lock);

	CHECK(isc_stdio_open(view->new_zone_file, "a", &fp));
	CHECK(isc_stdio_seek(fp, 0, SEEK_END));

	CHECK(isc_stdio_tell(fp, &offset));
	offsetok = ISC_TRUE;
	if (offset == 0)
		CHECK(add_comment(fp, view->name));

	CHECK(isc_stdio_write("zone ", 5, 1, fp, NULL));
	cfg_printx(zconfig, CFG_PRINTER_ONELINE, dumpzone, fp);
	CHECK(isc_stdio_write(";\n", 2, 1, fp, NULL));
	CHECK(isc_stdio_flush(fp));
	result = isc_stdio_close(fp);
	fp = NULL;

 cleanup:
	if (fp != NULL) {
		(void)isc_stdio_close(fp);
		if (offsetok) {
			isc_result_t result2;

			result2 = isc_file_truncate(view->new_zone_file,
						    offset);
			if (result2 != ISC_R_SUCCESS) {
				isc_log_write(named_g_lctx,
					      NAMED_LOGCATEGORY_GENERAL,
					      NAMED_LOGMODULE_SERVER,
					      ISC_LOG_ERROR,
					      "Error truncating NZF file '%s' "
					      "during rollback from append: "
					      "%s",
					      view->new_zone_file,
					      isc_result_totext(result2));
			}
		}
	}
	UNLOCK(&view->new_zone_lock);
	return (result);
}

static isc_result_t
nzf_writeconf(const cfg_obj_t *config, dns_view_t *view) {
	const cfg_obj_t *zl = NULL;
	cfg_list_t *list;
	const cfg_listelt_t *elt;

	FILE *fp = NULL;
	char tmp[1024];
	isc_result_t result;

	result = isc_file_template(view->new_zone_file, "nzf-XXXXXXXX",
				   tmp, sizeof(tmp));
	if (result == ISC_R_SUCCESS)
		result = isc_file_openunique(tmp, &fp);
	if (result != ISC_R_SUCCESS)
		return (result);

	cfg_map_get(config, "zone", &zl);
	if (!cfg_obj_islist(zl))
		CHECK(ISC_R_FAILURE);

	DE_CONST(&zl->value.list, list);

	CHECK(add_comment(fp, view->name));	/* force a comment */

	for (elt = ISC_LIST_HEAD(*list);
	     elt != NULL;
	     elt = ISC_LIST_NEXT(elt, link))
	{
		const cfg_obj_t *zconfig = cfg_listelt_value(elt);

		CHECK(isc_stdio_write("zone ", 5, 1, fp, NULL));
		cfg_printx(zconfig, CFG_PRINTER_ONELINE, dumpzone, fp);
		CHECK(isc_stdio_write(";\n", 2, 1, fp, NULL));
	}

	CHECK(isc_stdio_flush(fp));
	result = isc_stdio_close(fp);
	fp = NULL;
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	CHECK(isc_file_rename(tmp, view->new_zone_file));
	return (result);

 cleanup:
	if (fp != NULL)
		(void)isc_stdio_close(fp);
	(void)isc_file_remove(tmp);
	return (result);
}

#else /* HAVE_LMDB */

static void
nzd_setkey(MDB_val *key, dns_name_t *name, char *namebuf, size_t buflen) {
	dns_fixedname_t fixed;

	dns_fixedname_init(&fixed);
	dns_name_downcase(name, dns_fixedname_name(&fixed), NULL);
	dns_name_format(dns_fixedname_name(&fixed), namebuf, buflen);

	key->mv_data = namebuf;
	key->mv_size = strlen(namebuf);
}

static void
dumpzone(void *arg, const char *buf, int len) {
	isc_buffer_t **text = arg;

	putmem(text, buf, len);
}

static isc_result_t
nzd_save(MDB_txn **txnp, MDB_dbi dbi, dns_zone_t *zone,
	 const cfg_obj_t *zconfig)
{
	isc_result_t result;
	int status;
	dns_view_t *view;
	isc_boolean_t commit = ISC_FALSE;
	isc_buffer_t *text = NULL;
	char namebuf[1024];
	MDB_val key, data;

	view = dns_zone_getview(zone);

	nzd_setkey(&key, dns_zone_getorigin(zone), namebuf, sizeof(namebuf));

	LOCK(&view->new_zone_lock);

	if (zconfig == NULL) {
		/* We're deleting the zone from the database */
		status = mdb_del(*txnp, dbi, &key, NULL);
		if (status != MDB_SUCCESS && status != MDB_NOTFOUND) {
			isc_log_write(named_g_lctx,
				      NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER,
				      ISC_LOG_ERROR,
				      "Error deleting zone %s "
				      "from NZD database: %s",
				      namebuf, mdb_strerror(status));
			result = ISC_R_FAILURE;
			goto cleanup;
		} else if (status != MDB_NOTFOUND) {
			commit = ISC_TRUE;
		}
	} else {
		/* We're creating or overwriting the zone */
		const cfg_obj_t *zoptions;

		result = isc_buffer_allocate(view->mctx, &text, 256);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(named_g_lctx,
				      NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER,
				      ISC_LOG_ERROR,
				      "Unable to allocate buffer in "
				      "nzd_save(): %s",
				      isc_result_totext(result));
			goto cleanup;
		}

		zoptions = cfg_tuple_get(zconfig, "options");
		if (zoptions == NULL) {
			isc_log_write(named_g_lctx,
				      NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER,
				      ISC_LOG_ERROR,
				      "Unable to get options from config in "
				      "nzd_save()");
			result = ISC_R_FAILURE;
			goto cleanup;
		}

		cfg_printx(zoptions, CFG_PRINTER_ONELINE, dumpzone, &text);

		data.mv_data = isc_buffer_base(text);
		data.mv_size = isc_buffer_usedlength(text);

		status = mdb_put(*txnp, dbi, &key, &data, 0);
		if (status != MDB_SUCCESS) {
			isc_log_write(named_g_lctx,
				      NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER,
				      ISC_LOG_ERROR,
				      "Error inserting zone in "
				      "NZD database: %s",
				      mdb_strerror(status));
			result = ISC_R_FAILURE;
			goto cleanup;
		}

		commit = ISC_TRUE;
	}

	result = ISC_R_SUCCESS;

 cleanup:
	if (!commit || result != ISC_R_SUCCESS) {
		(void) mdb_txn_abort(*txnp);
	} else {
		status = mdb_txn_commit(*txnp);
		if (status != MDB_SUCCESS) {
			isc_log_write(named_g_lctx,
				      NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER,
				      ISC_LOG_ERROR,
				      "Error committing "
				      "NZD database: %s",
				      mdb_strerror(status));
			result = ISC_R_FAILURE;
		}
	}
	*txnp = NULL;

	UNLOCK(&view->new_zone_lock);

	if (text != NULL) {
		isc_buffer_free(&text);
	}

	return (result);
}

static isc_result_t
nzd_writable(dns_view_t *view) {
	isc_result_t result = ISC_R_SUCCESS;
	int status;
	MDB_dbi dbi;
	MDB_txn *txn = NULL;

	REQUIRE(view != NULL);

	status = mdb_txn_begin((MDB_env *) view->new_zone_dbenv, 0, 0, &txn);
	if (status != MDB_SUCCESS) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
			      "mdb_txn_begin: %s",
			      mdb_strerror(status));
		return (ISC_R_FAILURE);
	}

	status = mdb_dbi_open(txn, NULL, 0, &dbi);
	if (status != MDB_SUCCESS) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
			      "mdb_dbi_open: %s",
			      mdb_strerror(status));
		result = ISC_R_FAILURE;
	}

	mdb_txn_abort(txn);
	return (result);
}

static isc_result_t
nzd_open(dns_view_t *view, unsigned int flags, MDB_txn **txnp, MDB_dbi *dbi) {
	int status;
	MDB_txn *txn = NULL;

	REQUIRE(view != NULL);
	REQUIRE(txnp != NULL && *txnp == NULL);
	REQUIRE(dbi != NULL);

	status = mdb_txn_begin((MDB_env *) view->new_zone_dbenv, 0,
			       flags, &txn);
	if (status != MDB_SUCCESS) {
		isc_log_write(named_g_lctx,
			      NAMED_LOGCATEGORY_GENERAL, NAMED_LOGMODULE_SERVER,
			      ISC_LOG_WARNING, "mdb_txn_begin: %s",
			      mdb_strerror(status));
		goto cleanup;
	}

	status = mdb_dbi_open(txn, NULL, 0, dbi);
	if (status != MDB_SUCCESS) {
		isc_log_write(named_g_lctx,
		      NAMED_LOGCATEGORY_GENERAL, NAMED_LOGMODULE_SERVER,
			      ISC_LOG_WARNING, "mdb_dbi_open: %s",
			      mdb_strerror(status));
		goto cleanup;
	}

	*txnp = txn;

 cleanup:
	if (status != MDB_SUCCESS) {
		if (txn != NULL) {
			mdb_txn_abort(txn);
		}
		return (ISC_R_FAILURE);
	}

	return (ISC_R_SUCCESS);
}

/*
 * nzd_env_close() and nzd_env_reopen are a kluge to address the
 * problem of an NZD file possibly being created before we drop
 * root privileges.
 */
static void
nzd_env_close(dns_view_t *view) {
	const char *dbpath = NULL;
	char dbpath_copy[PATH_MAX];
	char lockpath[PATH_MAX];
	int status, ret;

	if (view->new_zone_dbenv == NULL) {
		return;
	}

	status = mdb_env_get_path(view->new_zone_dbenv, &dbpath);
	INSIST(status == MDB_SUCCESS);
	snprintf(lockpath, sizeof(lockpath), "%s-lock", dbpath);
	strlcpy(dbpath_copy, dbpath, sizeof(dbpath_copy));
	mdb_env_close((MDB_env *) view->new_zone_dbenv);

	/*
	 * Database files must be owned by the eventual user, not by root.
	 */
	ret = chown(dbpath_copy, ns_os_uid(), -1);
	UNUSED(ret);

	/*
	 * Some platforms need the lockfile not to exist when we reopen the
	 * environment.
	 */
	(void) isc_file_remove(lockpath);

	view->new_zone_dbenv = NULL;
}

static isc_result_t
nzd_env_reopen(dns_view_t *view) {
	isc_result_t result;
	MDB_env *env = NULL;
	int status;

	if (view->new_zone_db == NULL) {
		return (ISC_R_SUCCESS);
	}

	nzd_env_close(view);

	status = mdb_env_create(&env);
	if (status != MDB_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_OTHER, ISC_LOG_ERROR,
			      "mdb_env_create failed: %s",
			      mdb_strerror(status));
		CHECK(ISC_R_FAILURE);
	}

	if (view->new_zone_mapsize != 0ULL) {
		status = mdb_env_set_mapsize(env, view->new_zone_mapsize);
		if (status != MDB_SUCCESS) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_OTHER, ISC_LOG_ERROR,
				      "mdb_env_set_mapsize failed: %s",
				      mdb_strerror(status));
			CHECK(ISC_R_FAILURE);
		}
	}

	status = mdb_env_open(env, view->new_zone_db, DNS_LMDB_FLAGS, 0600);
	if (status != MDB_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_OTHER, ISC_LOG_ERROR,
			      "mdb_env_open of '%s' failed: %s",
			      view->new_zone_db, mdb_strerror(status));
		CHECK(ISC_R_FAILURE);
	}

	view->new_zone_dbenv = env;
	env = NULL;
	result = ISC_R_SUCCESS;

 cleanup:
	if (env != NULL) {
		mdb_env_close(env);
	}
	return (result);
}

static isc_result_t
nzd_close(MDB_txn **txnp, isc_boolean_t commit) {
	isc_result_t result = ISC_R_SUCCESS;
	int status;

	REQUIRE(txnp != NULL);

	if (*txnp != NULL) {
		if (commit) {
			status = mdb_txn_commit(*txnp);
			if (status != MDB_SUCCESS) {
				result = ISC_R_FAILURE;
			}
		} else {
			mdb_txn_abort(*txnp);
		}
		*txnp = NULL;
	}

	return (result);
}

static isc_result_t
nzd_count(dns_view_t *view, int *countp) {
	isc_result_t result;
	int status;
	MDB_txn *txn = NULL;
	MDB_dbi dbi;
	MDB_stat statbuf;

	REQUIRE(countp != NULL);

	result = nzd_open(view, MDB_RDONLY, &txn, &dbi);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	status = mdb_stat(txn, dbi, &statbuf);
	if (status != MDB_SUCCESS) {
		isc_log_write(named_g_lctx,
			      NAMED_LOGCATEGORY_GENERAL, NAMED_LOGMODULE_SERVER,
			      ISC_LOG_WARNING, "mdb_stat: %s",
			      mdb_strerror(status));
		result = ISC_R_FAILURE;
		goto cleanup;
	}

	*countp = statbuf.ms_entries;

 cleanup:
	(void) nzd_close(&txn, ISC_FALSE);

	return (result);
}

static isc_result_t
migrate_nzf(dns_view_t *view) {
	isc_result_t result;
	cfg_obj_t *nzf_config = NULL;
	int status, n;
	isc_buffer_t *text = NULL;
	isc_boolean_t commit = ISC_FALSE;
	const cfg_obj_t *zonelist;
	const cfg_listelt_t *element;
	char tempname[PATH_MAX];
	MDB_txn *txn = NULL;
	MDB_dbi dbi;
	MDB_val key, data;

	/*
	 * If NZF file doesn't exist, or NZD DB exists and already
	 * has data, return without attempting migration.
	 */
	if (!isc_file_exists(view->new_zone_file)) {
		result = ISC_R_SUCCESS;
		goto cleanup;
	}

	result = nzd_count(view, &n);
	if (result == ISC_R_SUCCESS && n > 0) {
		result = ISC_R_SUCCESS;
		goto cleanup;
	}

	isc_log_write(named_g_lctx,
		      NAMED_LOGCATEGORY_GENERAL, NAMED_LOGMODULE_SERVER,
		      ISC_LOG_INFO,
		      "Migrating zones from NZF file '%s' to "
		      "NZD database '%s'",
		      view->new_zone_file, view->new_zone_db);
	/*
	 * Instead of blindly copying lines, we parse the NZF file using
	 * the configuration parser, because it validates it against the
	 * config type, giving us a guarantee that valid configuration
	 * will be written to DB.
	 */
	cfg_parser_reset(named_g_addparser);
	result = cfg_parse_file(named_g_addparser, view->new_zone_file,
				&cfg_type_addzoneconf, &nzf_config);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(named_g_lctx,
			      NAMED_LOGCATEGORY_GENERAL, NAMED_LOGMODULE_SERVER,
			      ISC_LOG_ERROR,
			      "Error parsing NZF file '%s': %s",
			      view->new_zone_file,
			      isc_result_totext(result));
		goto cleanup;
	}

	zonelist = NULL;
	CHECK(cfg_map_get(nzf_config, "zone", &zonelist));
	if (!cfg_obj_islist(zonelist)) {
		CHECK(ISC_R_FAILURE);
	}

	CHECK(nzd_open(view, 0, &txn, &dbi));

	CHECK(isc_buffer_allocate(view->mctx, &text, 256));

	for (element = cfg_list_first(zonelist);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *zconfig;
		const cfg_obj_t *zoptions;
		char zname[DNS_NAME_FORMATSIZE];
		dns_fixedname_t fname;
		dns_name_t *name;
		const char *origin;
		isc_buffer_t b;

		zconfig = cfg_listelt_value(element);

		origin = cfg_obj_asstring(cfg_tuple_get(zconfig, "name"));
		if (origin == NULL) {
			result = ISC_R_FAILURE;
			goto cleanup;
		}

		/* Normalize zone name */
		isc_buffer_constinit(&b, origin, strlen(origin));
		isc_buffer_add(&b, strlen(origin));
		name = dns_fixedname_initname(&fname);
		CHECK(dns_name_fromtext(name, &b, dns_rootname,
					DNS_NAME_DOWNCASE, NULL));
		dns_name_format(name, zname, sizeof(zname));

		key.mv_data = zname;
		key.mv_size = strlen(zname);

		zoptions = cfg_tuple_get(zconfig, "options");
		if (zoptions == NULL) {
			result = ISC_R_FAILURE;
			goto cleanup;
		}

		isc_buffer_clear(text);
		cfg_printx(zoptions, CFG_PRINTER_ONELINE, dumpzone, &text);

		data.mv_data = isc_buffer_base(text);
		data.mv_size = isc_buffer_usedlength(text);

		status = mdb_put(txn, dbi, &key, &data, MDB_NOOVERWRITE);
		if (status != MDB_SUCCESS) {
			isc_log_write(named_g_lctx,
				      NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER,
				      ISC_LOG_ERROR,
				      "Error inserting zone in "
				      "NZD database: %s",
				      mdb_strerror(status));
			result = ISC_R_FAILURE;
			goto cleanup;
		}

		commit = ISC_TRUE;
	}

	result = ISC_R_SUCCESS;

	/*
	 * Leaving the NZF file in place is harmless as we won't use it
	 * if an NZD database is found for the view. But we rename NZF file
	 * to a backup name here.
	 */
	strlcpy(tempname, view->new_zone_file, sizeof(tempname));
	if (strlen(tempname) < sizeof(tempname) - 1) {
		strlcat(tempname, "~", sizeof(tempname));
		isc_file_rename(view->new_zone_file, tempname);
	}

 cleanup:
	if (result != ISC_R_SUCCESS) {
		(void) nzd_close(&txn, ISC_FALSE);
	} else {
		result = nzd_close(&txn, commit);
	}

	if (text != NULL) {
		isc_buffer_free(&text);
	}

	if (nzf_config != NULL) {
		cfg_obj_destroy(named_g_addparser, &nzf_config);
	}

	return (result);
}

#endif /* HAVE_LMDB */

static isc_result_t
newzone_parse(named_server_t *server, char *command, dns_view_t **viewp,
	      cfg_obj_t **zoneconfp, const cfg_obj_t **zoneobjp,
	      isc_boolean_t *redirectp, isc_buffer_t **text)
{
	isc_result_t result;
	isc_buffer_t argbuf;
	isc_boolean_t redirect = ISC_FALSE;
	cfg_obj_t *zoneconf = NULL;
	const cfg_obj_t *zlist = NULL;
	const cfg_obj_t *zoneobj = NULL;
	const cfg_obj_t *zoptions = NULL;
	const cfg_obj_t *obj = NULL;
	const char *viewname = NULL;
	dns_rdataclass_t rdclass;
	dns_view_t *view = NULL;
	const char *bn;

	REQUIRE(viewp != NULL && *viewp == NULL);
	REQUIRE(zoneobjp != NULL && *zoneobjp == NULL);
	REQUIRE(zoneconfp != NULL && *zoneconfp == NULL);
	REQUIRE(redirectp != NULL);

	/* Try to parse the argument string */
	isc_buffer_init(&argbuf, command, (unsigned int) strlen(command));
	isc_buffer_add(&argbuf, strlen(command));

	if (strncasecmp(command, "add", 3) == 0)
		bn = "addzone";
	else if (strncasecmp(command, "mod", 3) == 0)
		bn = "modzone";
	else
		INSIST(0);

	/*
	 * Convert the "addzone" or "modzone" to just "zone", for
	 * the benefit of the parser
	 */
	isc_buffer_forward(&argbuf, 3);

	cfg_parser_reset(named_g_addparser);
	CHECK(cfg_parse_buffer3(named_g_addparser, &argbuf, bn, 0,
				&cfg_type_addzoneconf, &zoneconf));
	CHECK(cfg_map_get(zoneconf, "zone", &zlist));
	if (!cfg_obj_islist(zlist))
		CHECK(ISC_R_FAILURE);

	/* For now we only support adding one zone at a time */
	zoneobj = cfg_listelt_value(cfg_list_first(zlist));

	/* Check the zone type for ones that are not supported by addzone. */
	zoptions = cfg_tuple_get(zoneobj, "options");

	obj = NULL;
	(void)cfg_map_get(zoptions, "type", &obj);
	if (obj == NULL) {
		(void) cfg_map_get(zoptions, "in-view", &obj);
		if (obj != NULL) {
			(void) putstr(text,
				      "'in-view' zones not supported by ");
			(void) putstr(text, bn);
		} else
			(void) putstr(text, "zone type not specified");
		CHECK(ISC_R_FAILURE);
	}

	if (strcasecmp(cfg_obj_asstring(obj), "hint") == 0 ||
	    strcasecmp(cfg_obj_asstring(obj), "forward") == 0 ||
	    strcasecmp(cfg_obj_asstring(obj), "delegation-only") == 0)
	{
		(void) putstr(text, "'");
		(void) putstr(text, cfg_obj_asstring(obj));
		(void) putstr(text, "' zones not supported by ");
		(void) putstr(text, bn);
		CHECK(ISC_R_FAILURE);
	}

	if (strcasecmp(cfg_obj_asstring(obj), "redirect") == 0)
		redirect = ISC_TRUE;

	/* Make sense of optional class argument */
	obj = cfg_tuple_get(zoneobj, "class");
	CHECK(named_config_getclass(obj, dns_rdataclass_in, &rdclass));

	/* Make sense of optional view argument */
	obj = cfg_tuple_get(zoneobj, "view");
	if (obj && cfg_obj_isstring(obj))
		viewname = cfg_obj_asstring(obj);
	if (viewname == NULL || *viewname == '\0')
		viewname = "_default";
	result = dns_viewlist_find(&server->viewlist, viewname, rdclass,
				   &view);
	if (result == ISC_R_NOTFOUND) {
		(void) putstr(text, "no matching view found for '");
		(void) putstr(text, viewname);
		(void) putstr(text, "'");
		goto cleanup;
	} else if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	*viewp = view;
	*zoneobjp = zoneobj;
	*zoneconfp = zoneconf;
	*redirectp = redirect;

	return (ISC_R_SUCCESS);

 cleanup:
	if (zoneconf != NULL)
		cfg_obj_destroy(named_g_addparser, &zoneconf);
	if (view != NULL)
		dns_view_detach(&view);

	return (result);
}

static isc_result_t
delete_zoneconf(dns_view_t *view, cfg_parser_t *pctx,
		const cfg_obj_t *config, const dns_name_t *zname,
		nzfwriter_t nzfwriter)
{
	isc_result_t result = ISC_R_NOTFOUND;
	const cfg_listelt_t *elt = NULL;
	const cfg_obj_t *zl = NULL;
	cfg_list_t *list;
	dns_fixedname_t myfixed;
	dns_name_t *myname;

	REQUIRE(view != NULL);
	REQUIRE(pctx != NULL);
	REQUIRE(config != NULL);
	REQUIRE(zname != NULL);

	LOCK(&view->new_zone_lock);

	cfg_map_get(config, "zone", &zl);

	if (!cfg_obj_islist(zl))
		CHECK(ISC_R_FAILURE);

	DE_CONST(&zl->value.list, list);

	myname = dns_fixedname_initname(&myfixed);

	for (elt = ISC_LIST_HEAD(*list);
	     elt != NULL;
	     elt = ISC_LIST_NEXT(elt, link))
	{
		const cfg_obj_t *zconf = cfg_listelt_value(elt);
		const char *zn;
		cfg_listelt_t *e;

		zn = cfg_obj_asstring(cfg_tuple_get(zconf, "name"));
		result = dns_name_fromstring(myname, zn, 0, NULL);
		if (result != ISC_R_SUCCESS ||
		    !dns_name_equal(zname, myname))
			continue;

		DE_CONST(elt, e);
		ISC_LIST_UNLINK(*list, e, link);
		cfg_obj_destroy(pctx, &e->obj);
		isc_mem_put(pctx->mctx, e, sizeof(*e));
		result = ISC_R_SUCCESS;
		break;
	}

	/*
	 * Write config to NZF file if appropriate
	 */
	if (nzfwriter != NULL && view->new_zone_file != NULL)
		result = nzfwriter(config, view);

 cleanup:
	UNLOCK(&view->new_zone_lock);
	return (result);
}

static isc_result_t
do_addzone(named_server_t *server, ns_cfgctx_t *cfg, dns_view_t *view,
	   dns_name_t *name, cfg_obj_t *zoneconf, const cfg_obj_t *zoneobj,
	   isc_boolean_t redirect, isc_buffer_t **text)
{
	isc_result_t result, tresult;
	dns_zone_t *zone = NULL;
#ifndef HAVE_LMDB
	FILE *fp = NULL;
	isc_boolean_t cleanup_config = ISC_FALSE;
#else /* HAVE_LMDB */
	MDB_txn *txn = NULL;
	MDB_dbi dbi;

	UNUSED(zoneconf);
#endif /* HAVE_LMDB */

	/* Zone shouldn't already exist */
	if (redirect) {
		result = (view->redirect != NULL) ? ISC_R_SUCCESS :
						    ISC_R_NOTFOUND;
	} else
		result = dns_zt_find(view->zonetable, name, 0, NULL, &zone);
	if (result == ISC_R_SUCCESS) {
		result = ISC_R_EXISTS;
		goto cleanup;
	} else if (result == DNS_R_PARTIALMATCH) {
		/* Create our sub-zone anyway */
		dns_zone_detach(&zone);
		zone = NULL;
	} else if (result != ISC_R_NOTFOUND)
		goto cleanup;

#ifndef HAVE_LMDB
	/*
	 * Make sure we can open the configuration save file
	 */
	result = isc_stdio_open(view->new_zone_file, "a", &fp);
	if (result != ISC_R_SUCCESS) {
		TCHECK(putstr(text, "unable to create '"));
		TCHECK(putstr(text, view->new_zone_file));
		TCHECK(putstr(text, "': "));
		TCHECK(putstr(text, isc_result_totext(result)));
		goto cleanup;
	}

	(void)isc_stdio_close(fp);
	fp = NULL;
#else /* HAVE_LMDB */
	/* Make sure we can open the NZD database */
	result = nzd_writable(view);
	if (result != ISC_R_SUCCESS) {
		TCHECK(putstr(text, "unable to open NZD database for '"));
		TCHECK(putstr(text, view->new_zone_db));
		TCHECK(putstr(text, "'"));
		result = ISC_R_FAILURE;
		goto cleanup;
	}
#endif /* HAVE_LMDB */

	result = isc_task_beginexclusive(server->task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	/* Mark view unfrozen and configure zone */
	dns_view_thaw(view);
	result = configure_zone(cfg->config, zoneobj, cfg->vconfig,
				server->mctx, view, &server->viewlist,
				cfg->actx, ISC_TRUE, ISC_FALSE, ISC_FALSE);
	dns_view_freeze(view);

	isc_task_endexclusive(server->task);

	if (result != ISC_R_SUCCESS) {
		TCHECK(putstr(text, "configure_zone failed: "));
		TCHECK(putstr(text, isc_result_totext(result)));
		goto cleanup;
	}

	/* Is it there yet? */
	if (redirect) {
		if (view->redirect == NULL)
			CHECK(ISC_R_NOTFOUND);
		dns_zone_attach(view->redirect, &zone);
	} else {
		result = dns_zt_find(view->zonetable, name, 0, NULL, &zone);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(named_g_lctx,
				      NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER,
				      ISC_LOG_ERROR,
				      "added new zone was not found: %s",
				      isc_result_totext(result));
			goto cleanup;
		}
	}

#ifndef HAVE_LMDB
	/*
	 * If there wasn't a previous newzone config, just save the one
	 * we've created. If there was a previous one, merge the new
	 * zone into it.
	 */
	if (cfg->nzf_config == NULL) {
		cfg_obj_attach(zoneconf, &cfg->nzf_config);
	} else {
		cfg_obj_t *z;
		DE_CONST(zoneobj, z);
		CHECK(cfg_parser_mapadd(cfg->add_parser,
					cfg->nzf_config, z, "zone"));
	}
	cleanup_config = ISC_TRUE;
#endif /* HAVE_LMDB */

	/*
	 * Load the zone from the master file.  If this fails, we'll
	 * need to undo the configuration we've done already.
	 */
	result = dns_zone_loadnew(zone);
	if (result != ISC_R_SUCCESS) {
		dns_db_t *dbp = NULL;

		TCHECK(putstr(text, "dns_zone_loadnew failed: "));
		TCHECK(putstr(text, isc_result_totext(result)));

		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "addzone failed; reverting.");

		/* If the zone loaded partially, unload it */
		if (dns_zone_getdb(zone, &dbp) == ISC_R_SUCCESS) {
			dns_db_detach(&dbp);
			dns_zone_unload(zone);
		}

		/* Remove the zone from the zone table */
		dns_zt_unmount(view->zonetable, zone);
		goto cleanup;
	}

	/* Flag the zone as having been added at runtime */
	dns_zone_setadded(zone, ISC_TRUE);

#ifdef HAVE_LMDB
	/* Save the new zone configuration into the NZD */
	CHECK(nzd_open(view, 0, &txn, &dbi));
	CHECK(nzd_save(&txn, dbi, zone, zoneobj));
#else
	/* Append the zone configuration to the NZF */
	result = nzf_append(view, zoneobj);
#endif /* HAVE_LMDB */

 cleanup:

#ifndef HAVE_LMDB
	if (fp != NULL)
		(void)isc_stdio_close(fp);
	if (result != ISC_R_SUCCESS && cleanup_config) {
		tresult = delete_zoneconf(view, cfg->add_parser,
					 cfg->nzf_config, name,
					 NULL);
		RUNTIME_CHECK(tresult == ISC_R_SUCCESS);
	}
#else /* HAVE_LMDB */
	if (txn != NULL)
		(void) nzd_close(&txn, ISC_FALSE);
#endif /* HAVE_LMDB */

	if (zone != NULL)
		dns_zone_detach(&zone);

	return (result);
}

static isc_result_t
do_modzone(named_server_t *server, ns_cfgctx_t *cfg, dns_view_t *view,
	   dns_name_t *name, const char *zname, const cfg_obj_t *zoneobj,
	   isc_boolean_t redirect, isc_buffer_t **text)
{
	isc_result_t result, tresult;
	dns_zone_t *zone = NULL;
	isc_boolean_t added;
	isc_boolean_t exclusive = ISC_FALSE;
#ifndef HAVE_LMDB
	FILE *fp = NULL;
	cfg_obj_t *z;
#else /* HAVE_LMDB */
	MDB_txn *txn = NULL;
	MDB_dbi dbi;
#endif /* HAVE_LMDB */

	/* Zone must already exist */
	if (redirect) {
		if (view->redirect != NULL) {
			dns_zone_attach(view->redirect, &zone);
			result = ISC_R_SUCCESS;
		} else
			result = ISC_R_NOTFOUND;
	} else
		result = dns_zt_find(view->zonetable, name, 0, NULL, &zone);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	added = dns_zone_getadded(zone);
	dns_zone_detach(&zone);

#ifndef HAVE_LMDB
	cfg = (ns_cfgctx_t *) view->new_zone_config;
	if (cfg == NULL) {
		TCHECK(putstr(text, "new zone config is not set"));
		CHECK(ISC_R_FAILURE);
	}
#endif

	result = isc_task_beginexclusive(server->task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	exclusive = ISC_TRUE;

#ifndef HAVE_LMDB
	/* Make sure we can open the configuration save file */
	result = isc_stdio_open(view->new_zone_file, "a", &fp);
	if (result != ISC_R_SUCCESS) {
		TCHECK(putstr(text, "unable to open '"));
		TCHECK(putstr(text, view->new_zone_file));
		TCHECK(putstr(text, "': "));
		TCHECK(putstr(text, isc_result_totext(result)));
		goto cleanup;
	}
	(void)isc_stdio_close(fp);
	fp = NULL;
#else /* HAVE_LMDB */
	/* Make sure we can open the NZD database */
	result = nzd_writable(view);
	if (result != ISC_R_SUCCESS) {
		TCHECK(putstr(text, "unable to open NZD database for '"));
		TCHECK(putstr(text, view->new_zone_db));
		TCHECK(putstr(text, "'"));
		result = ISC_R_FAILURE;
		goto cleanup;
	}
#endif /* HAVE_LMDB */

	/* Reconfigure the zone */
	dns_view_thaw(view);
	result = configure_zone(cfg->config, zoneobj, cfg->vconfig,
				server->mctx, view, &server->viewlist,
				cfg->actx, ISC_TRUE, ISC_FALSE, ISC_TRUE);
	dns_view_freeze(view);

	exclusive = ISC_FALSE;
	isc_task_endexclusive(server->task);

	if (result != ISC_R_SUCCESS) {
		TCHECK(putstr(text, "configure_zone failed: "));
		TCHECK(putstr(text, isc_result_totext(result)));
		goto cleanup;
	}

	/* Is it there yet? */
	if (redirect) {
		if (view->redirect == NULL)
			CHECK(ISC_R_NOTFOUND);
		dns_zone_attach(view->redirect, &zone);
	} else
		CHECK(dns_zt_find(view->zonetable, name, 0, NULL, &zone));



#ifndef HAVE_LMDB
	/* Remove old zone from configuration (and NZF file if applicable) */
	if (added) {
		result = delete_zoneconf(view, cfg->add_parser,
					 cfg->nzf_config,
					 dns_zone_getorigin(zone),
					 nzf_writeconf);
		if (result != ISC_R_SUCCESS) {
			TCHECK(putstr(text, "former zone configuration "
					    "not deleted: "));
			TCHECK(putstr(text, isc_result_totext(result)));
			goto cleanup;
		}
	}
#endif /* HAVE_LMDB */

	if (!added) {
		if (cfg->vconfig == NULL) {
			result = delete_zoneconf(view, cfg->conf_parser,
						 cfg->config,
						 dns_zone_getorigin(zone),
						 NULL);
		} else {
			const cfg_obj_t *voptions =
				cfg_tuple_get(cfg->vconfig, "options");
			result = delete_zoneconf(view, cfg->conf_parser,
						 voptions,
						 dns_zone_getorigin(zone),
						 NULL);
		}

		if (result != ISC_R_SUCCESS) {
			TCHECK(putstr(text, "former zone configuration "
					    "not deleted: "));
			TCHECK(putstr(text, isc_result_totext(result)));
			goto cleanup;
		}
	}

	/* Load the zone from the master file if it needs reloading. */
	result = dns_zone_loadnew(zone);

	/*
	 * Dynamic zones need no reloading, so we can pass this result.
	 */
	if (result == DNS_R_DYNAMIC)
		result = ISC_R_SUCCESS;

	if (result != ISC_R_SUCCESS) {
		dns_db_t *dbp = NULL;

		TCHECK(putstr(text, "failed to load zone '"));
		TCHECK(putstr(text, zname));
		TCHECK(putstr(text, "': "));
		TCHECK(putstr(text, isc_result_totext(result)));
		TCHECK(putstr(text, "\nThe zone is no longer being served. "));
		TCHECK(putstr(text, "Use 'rndc addzone' to correct\n"));
		TCHECK(putstr(text, "the problem and restore service."));

		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "modzone failed; removing zone.");

		/* If the zone loaded partially, unload it */
		if (dns_zone_getdb(zone, &dbp) == ISC_R_SUCCESS) {
			dns_db_detach(&dbp);
			dns_zone_unload(zone);
		}

		/* Remove the zone from the zone table */
		dns_zt_unmount(view->zonetable, zone);
		goto cleanup;
	}

#ifndef HAVE_LMDB
	/* Store the new zone configuration; also in NZF if applicable */
	DE_CONST(zoneobj, z);
	CHECK(cfg_parser_mapadd(cfg->add_parser, cfg->nzf_config, z, "zone"));
#endif /* HAVE_LMDB */

	if (added) {
#ifdef HAVE_LMDB
		CHECK(nzd_open(view, 0, &txn, &dbi));
		CHECK(nzd_save(&txn, dbi, zone, zoneobj));
#else
		result = nzf_append(view, zoneobj);
		if (result != ISC_R_SUCCESS) {
			TCHECK(putstr(text, "\nNew zone config not saved: "));
			TCHECK(putstr(text, isc_result_totext(result)));
			goto cleanup;
		}
#endif /* HAVE_LMDB */

		TCHECK(putstr(text, "zone '"));
		TCHECK(putstr(text, zname));
		TCHECK(putstr(text, "' reconfigured."));

	} else {
		TCHECK(putstr(text, "zone '"));
		TCHECK(putstr(text, zname));
		TCHECK(putstr(text, "' must also be reconfigured in\n"));
		TCHECK(putstr(text, "named.conf to make changes permanent."));
	}

 cleanup:
	if (exclusive)
		isc_task_endexclusive(server->task);

#ifndef HAVE_LMDB
	if (fp != NULL)
		(void)isc_stdio_close(fp);
#else /* HAVE_LMDB */
	if (txn != NULL)
		(void) nzd_close(&txn, ISC_FALSE);
#endif /* HAVE_LMDB */

	if (zone != NULL)
		dns_zone_detach(&zone);

	return (result);
}

/*
 * Act on an "addzone" or "modzone" command from the command channel.
 */
isc_result_t
named_server_changezone(named_server_t *server, char *command,
			isc_buffer_t **text)
{
	isc_result_t result;
	isc_boolean_t addzone;
	isc_boolean_t redirect = ISC_FALSE;
	ns_cfgctx_t *cfg = NULL;
	cfg_obj_t *zoneconf = NULL;
	const cfg_obj_t *zoneobj = NULL;
	const char *zonename;
	dns_view_t *view = NULL;
	isc_buffer_t buf;
	dns_fixedname_t fname;
	dns_name_t *dnsname;

	if (strncasecmp(command, "add", 3) == 0)
		addzone = ISC_TRUE;
	else {
		INSIST(strncasecmp(command, "mod", 3) == 0);
		addzone = ISC_FALSE;
	}

	CHECK(newzone_parse(server, command, &view, &zoneconf,
			    &zoneobj, &redirect, text));

	/* Are we accepting new zones in this view? */
#ifdef HAVE_LMDB
	if (view->new_zone_db == NULL)
#else
	if (view->new_zone_file == NULL)
#endif /* HAVE_LMDB */
	{
		(void) putstr(text, "Not allowing new zones in view '");
		(void) putstr(text, view->name);
		(void) putstr(text, "'");
		result = ISC_R_NOPERM;
		goto cleanup;
	}

	cfg = (ns_cfgctx_t *) view->new_zone_config;
	if (cfg == NULL) {
		result = ISC_R_FAILURE;
		goto cleanup;
	}

	zonename = cfg_obj_asstring(cfg_tuple_get(zoneobj, "name"));
	isc_buffer_constinit(&buf, zonename, strlen(zonename));
	isc_buffer_add(&buf, strlen(zonename));

	dnsname = dns_fixedname_initname(&fname);
	CHECK(dns_name_fromtext(dnsname, &buf, dns_rootname, 0, NULL));

	if (redirect) {
		if (!dns_name_equal(dnsname, dns_rootname)) {
			(void) putstr(text,
				      "redirect zones must be called \".\"");
			CHECK(ISC_R_FAILURE);
		}
	}

	if (addzone)
		CHECK(do_addzone(server, cfg, view, dnsname, zoneconf,
				 zoneobj, redirect, text));
	else
		CHECK(do_modzone(server, cfg, view, dnsname, zonename,
				 zoneobj, redirect, text));

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "%s zone %s in view %s via %s",
		      addzone ? "added" : "updated",
		      zonename, view->name,
		      addzone ? NAMED_COMMAND_ADDZONE : NAMED_COMMAND_MODZONE);

	/* Changing a zone counts as reconfiguration */
	CHECK(isc_time_now(&named_g_configtime));

 cleanup:
	if (isc_buffer_usedlength(*text) > 0)
		(void) putnull(text);
	if (zoneconf != NULL)
		cfg_obj_destroy(named_g_addparser, &zoneconf);
	if (view != NULL)
		dns_view_detach(&view);

	return (result);
}

static isc_boolean_t
inuse(const char* file, isc_boolean_t first, isc_buffer_t **text) {
	if (file != NULL && isc_file_exists(file)) {
		if (first)
			(void) putstr(text,
				      "The following files were in use "
				      "and may now be removed:\n");
		else
			(void) putstr(text, "\n");
		(void) putstr(text, file);
		(void) putnull(text);
		return (ISC_FALSE);
	}
	return (first);
}

typedef struct {
	dns_zone_t *zone;
	isc_boolean_t cleanup;
} ns_dzctx_t;

/*
 * Carry out a zone deletion scheduled by named_server_delzone().
 */
static void
rmzone(isc_task_t *task, isc_event_t *event) {
	ns_dzctx_t *dz = (ns_dzctx_t *)event->ev_arg;
	dns_zone_t *zone, *raw = NULL, *mayberaw;
	char zonename[DNS_NAME_FORMATSIZE];
	dns_view_t *view;
	ns_cfgctx_t *cfg;
	dns_db_t *dbp = NULL;
	isc_boolean_t added;
	isc_result_t result;
#ifdef HAVE_LMDB
	MDB_txn *txn = NULL;
	MDB_dbi dbi;
#endif

	REQUIRE(dz != NULL);

	isc_event_free(&event);

	/* Dig out configuration for this zone */
	zone = dz->zone;
	view = dns_zone_getview(zone);
	cfg = (ns_cfgctx_t *) view->new_zone_config;
	dns_name_format(dns_zone_getorigin(zone), zonename, sizeof(zonename));

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "deleting zone %s in view %s via delzone",
		      zonename, view->name);

	/* Remove the zone from configuration (and NZF file if applicable) */
	added = dns_zone_getadded(zone);

	if (added && cfg != NULL) {
#ifdef HAVE_LMDB
		/* Make sure we can open the NZD database */
		result = nzd_open(view, 0, &txn, &dbi);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER,
				      ISC_LOG_ERROR,
				      "unable to open NZD database for '%s'",
				      view->new_zone_db);
		} else {
			result = nzd_save(&txn, dbi, zone, NULL);
		}

		if (result != ISC_R_SUCCESS) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER,
				      ISC_LOG_ERROR, "unable to "
				      "delete zone configuration: %s",
				      isc_result_totext(result));
		}
#else
		result = delete_zoneconf(view, cfg->add_parser,
					 cfg->nzf_config,
					 dns_zone_getorigin(zone),
					 nzf_writeconf);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER,
				      ISC_LOG_ERROR, "unable to "
				      "delete zone configuration: %s",
				      isc_result_totext(result));
		}
#endif /* HAVE_LMDB */
	}

	if (!added && cfg != NULL) {
		if (cfg->vconfig != NULL) {
			const cfg_obj_t *voptions =
				cfg_tuple_get(cfg->vconfig, "options");
			result = delete_zoneconf(view, cfg->conf_parser,
						 voptions,
						 dns_zone_getorigin(zone),
						 NULL);
		} else {
			result = delete_zoneconf(view, cfg->conf_parser,
						 cfg->config,
						 dns_zone_getorigin(zone),
						 NULL);
		}
		if (result != ISC_R_SUCCESS){
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER,
				      ISC_LOG_ERROR, "unable to "
				      "delete zone configuration: %s",
				      isc_result_totext(result));
		}
	}

	/* Unload zone database */
	if (dns_zone_getdb(zone, &dbp) == ISC_R_SUCCESS) {
		dns_db_detach(&dbp);
		dns_zone_unload(zone);
	}

	/* Clean up stub/slave zone files if requested to do so */
	dns_zone_getraw(zone, &raw);
	mayberaw = (raw != NULL) ? raw : zone;

	if (added && dz->cleanup) {
		const char *file;

		file = dns_zone_getfile(mayberaw);
		result = isc_file_remove(file);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
				      "file %s not removed: %s",
				      file, isc_result_totext(result));
		}

		file = dns_zone_getjournal(mayberaw);
		result = isc_file_remove(file);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
				      "file %s not removed: %s",
				      file, isc_result_totext(result));
		}

		if (zone != mayberaw) {
			file = dns_zone_getfile(zone);
			result = isc_file_remove(file);
			if (result != ISC_R_SUCCESS) {
				isc_log_write(named_g_lctx,
					      NAMED_LOGCATEGORY_GENERAL,
					      NAMED_LOGMODULE_SERVER,
					      ISC_LOG_WARNING,
					      "file %s not removed: %s",
					      file, isc_result_totext(result));
			}

			file = dns_zone_getjournal(zone);
			result = isc_file_remove(file);
			if (result != ISC_R_SUCCESS) {
				isc_log_write(named_g_lctx,
					      NAMED_LOGCATEGORY_GENERAL,
					      NAMED_LOGMODULE_SERVER,
					      ISC_LOG_WARNING,
					      "file %s not removed: %s",
					      file, isc_result_totext(result));
			}
		}
	}

#ifdef HAVE_LMDB
	if (txn != NULL)
		(void) nzd_close(&txn, ISC_FALSE);
#endif
	if (raw != NULL)
		dns_zone_detach(&raw);
	dns_zone_detach(&zone);
	isc_mem_put(named_g_mctx, dz, sizeof(*dz));
	isc_task_detach(&task);
}

/*
 * Act on a "delzone" command from the command channel.
 */
isc_result_t
named_server_delzone(named_server_t *server, isc_lex_t *lex,
		     isc_buffer_t **text)
{
	isc_result_t result, tresult;
	dns_zone_t *zone = NULL;
	dns_zone_t *raw = NULL;
	dns_zone_t *mayberaw;
	dns_view_t *view = NULL;
	char zonename[DNS_NAME_FORMATSIZE];
	isc_boolean_t cleanup = ISC_FALSE;
	const char *ptr;
	isc_boolean_t added;
	ns_dzctx_t *dz = NULL;
	isc_event_t *dzevent = NULL;
	isc_task_t *task = NULL;

	/* Skip the command name. */
	ptr = next_token(lex, text);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	/* Find out what we are to do. */
	ptr = next_token(lex, text);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	if (strcmp(ptr, "-clean") == 0 || strcmp(ptr, "-clear") == 0) {
		cleanup = ISC_TRUE;
		ptr = next_token(lex, text);
	}

	CHECK(zone_from_args(server, lex, ptr, &zone, zonename,
			     text, ISC_FALSE));
	if (zone == NULL) {
		result = ISC_R_UNEXPECTEDEND;
		goto cleanup;
	}

	INSIST(zonename != NULL);

	/* Is this a policy zone? */
	if (dns_zone_get_rpz_num(zone) != DNS_RPZ_INVALID_NUM) {
		TCHECK(putstr(text, "zone '"));
		TCHECK(putstr(text, zonename));
		TCHECK(putstr(text,
			      "' cannot be deleted: response-policy zone."));
		result = ISC_R_FAILURE;
		goto cleanup;
	}

	view = dns_zone_getview(zone);
	if (dns_zone_gettype(zone) == dns_zone_redirect)
		dns_zone_detach(&view->redirect);
	else
		CHECK(dns_zt_unmount(view->zonetable, zone));

	/* Send cleanup event */
	dz = isc_mem_get(named_g_mctx, sizeof(*dz));
	if (dz == NULL)
		CHECK(ISC_R_NOMEMORY);

	dz->cleanup = cleanup;
	dz->zone = NULL;
	dns_zone_attach(zone, &dz->zone);
	dzevent = isc_event_allocate(named_g_mctx, server, NAMED_EVENT_DELZONE,
				     rmzone, dz, sizeof(isc_event_t));
	if (dzevent == NULL)
		CHECK(ISC_R_NOMEMORY);

	dns_zone_gettask(zone, &task);
	isc_task_send(task, &dzevent);
	dz = NULL;

	/* Inform user about cleaning up stub/slave zone files */
	dns_zone_getraw(zone, &raw);
	mayberaw = (raw != NULL) ? raw : zone;

	added = dns_zone_getadded(zone);
	if (!added) {
		TCHECK(putstr(text, "zone '"));
		TCHECK(putstr(text, zonename));
		TCHECK(putstr(text,
			      "' is no longer active and will be deleted.\n"));
		TCHECK(putstr(text, "To keep it from returning "));
		TCHECK(putstr(text, "when the server is restarted, it\n"));
		TCHECK(putstr(text, "must also be removed from named.conf."));
	} else if (cleanup) {
		TCHECK(putstr(text, "zone '"));
		TCHECK(putstr(text, zonename));
		TCHECK(putstr(text, "' and associated files will be deleted."));
	} else if (dns_zone_gettype(mayberaw) == dns_zone_slave ||
		   dns_zone_gettype(mayberaw) == dns_zone_stub)
	{
		isc_boolean_t first;
		const char *file;

		TCHECK(putstr(text, "zone '"));
		TCHECK(putstr(text, zonename));
		TCHECK(putstr(text, "' will be deleted."));

		file = dns_zone_getfile(mayberaw);
		first = inuse(file, ISC_TRUE, text);

		file = dns_zone_getjournal(mayberaw);
		first = inuse(file, first, text);

		if (zone != mayberaw) {
			file = dns_zone_getfile(zone);
			first = inuse(file, first, text);

			file = dns_zone_getjournal(zone);
			(void) inuse(file, first, text);
		}
	}

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "zone %s scheduled for removal via delzone", zonename);

	/* Removing a zone counts as reconfiguration */
	CHECK(isc_time_now(&named_g_configtime));

	result = ISC_R_SUCCESS;

 cleanup:
	if (isc_buffer_usedlength(*text) > 0)
		(void) putnull(text);
	if (raw != NULL)
		dns_zone_detach(&raw);
	if (zone != NULL)
		dns_zone_detach(&zone);
	if (dz != NULL) {
		dns_zone_detach(&dz->zone);
		isc_mem_put(named_g_mctx, dz, sizeof(*dz));
	}

	return (result);
}

static const cfg_obj_t *
find_name_in_list_from_map(const cfg_obj_t *config,
			   const char *map_key_for_list,
			   const char *name, isc_boolean_t redirect)
{
	const cfg_obj_t *list = NULL;
	const cfg_listelt_t *element;
	const cfg_obj_t *obj = NULL;
	dns_fixedname_t fixed1, fixed2;
	dns_name_t *name1 = NULL, *name2 = NULL;
	isc_result_t result;

	if (strcmp(map_key_for_list, "zone") == 0) {
		name1 = dns_fixedname_initname(&fixed1);
		name2 = dns_fixedname_initname(&fixed2);
		result = dns_name_fromstring(name1, name, 0, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}

	cfg_map_get(config, map_key_for_list, &list);
	for (element = cfg_list_first(list);
	     element != NULL;
	     element = cfg_list_next(element))
	{
		const char *vname;

		obj = cfg_listelt_value(element);
		INSIST(obj != NULL);
		vname = cfg_obj_asstring(cfg_tuple_get(obj, "name"));
		if (vname == NULL) {
			obj = NULL;
			continue;
		}

		if (name1 != NULL) {
			result = dns_name_fromstring(name2, vname, 0, NULL);
			if (result == ISC_R_SUCCESS &&
			    dns_name_equal(name1, name2)) {
				const cfg_obj_t *zoptions;
				const cfg_obj_t *typeobj = NULL;
				zoptions = cfg_tuple_get(obj, "options");

				if (zoptions != NULL)
					cfg_map_get(zoptions, "type", &typeobj);
				if (redirect && typeobj != NULL &&
				    strcasecmp(cfg_obj_asstring(typeobj),
					       "redirect") == 0)
					break;
				else if (!redirect)
					break;
			}
		} else if (strcasecmp(vname, name) == 0)
			break;

		obj = NULL;
	}

	return (obj);
}

static void
emitzone(void *arg, const char *buf, int len) {
	isc_buffer_t **tpp = arg;
	putmem(tpp, buf, len);
}

/*
 * Act on a "showzone" command from the command channel.
 */
isc_result_t
named_server_showzone(named_server_t *server, isc_lex_t *lex,
		      isc_buffer_t **text)
{
	isc_result_t result;
	const cfg_obj_t	*vconfig = NULL, *zconfig = NULL;
	char zonename[DNS_NAME_FORMATSIZE];
	const cfg_obj_t *map;
	dns_view_t *view = NULL;
	dns_zone_t *zone = NULL;
	ns_cfgctx_t *cfg = NULL;
	isc_boolean_t exclusive = ISC_FALSE;
#ifdef HAVE_LMDB
	cfg_obj_t *nzconfig = NULL;
#endif /* HAVE_LMDB */
	isc_boolean_t added, redirect;

	/* Parse parameters */
	CHECK(zone_from_args(server, lex, NULL, &zone, zonename,
			     text, ISC_TRUE));
	if (zone == NULL) {
		result = ISC_R_UNEXPECTEDEND;
		goto cleanup;
	}

	redirect = dns_zone_gettype(zone) == dns_zone_redirect;
	added = dns_zone_getadded(zone);
	view = dns_zone_getview(zone);
	dns_zone_detach(&zone);

	cfg = (ns_cfgctx_t *) view->new_zone_config;
	if (cfg == NULL) {
		result = ISC_R_FAILURE;
		goto cleanup;
	}

	result = isc_task_beginexclusive(server->task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	exclusive = ISC_TRUE;

	if (!added) {
		/* Find the view statement */
		vconfig = find_name_in_list_from_map(cfg->config, "view",
						     view->name, ISC_FALSE);

		/* Find the zone statement */
		if (vconfig != NULL)
			map = cfg_tuple_get(vconfig, "options");
		else
			map = cfg->config;

		zconfig = find_name_in_list_from_map(map, "zone", zonename,
						     redirect);
	}

#ifndef HAVE_LMDB
	if (zconfig == NULL && cfg->nzf_config != NULL)
		zconfig = find_name_in_list_from_map(cfg->nzf_config,
						     "zone", zonename,
						     redirect);
#else /* HAVE_LMDB */
	if (zconfig == NULL) {
		const cfg_obj_t *zlist = NULL;
		CHECK(get_newzone_config(view, zonename, &nzconfig));
		CHECK(cfg_map_get(nzconfig, "zone", &zlist));
		if (!cfg_obj_islist(zlist))
			CHECK(ISC_R_FAILURE);

		zconfig = cfg_listelt_value(cfg_list_first(zlist));
	}
#endif /* HAVE_LMDB */

	if (zconfig == NULL)
		CHECK(ISC_R_NOTFOUND);

	putstr(text, "zone ");
	cfg_printx(zconfig, CFG_PRINTER_ONELINE, emitzone, text);
	putstr(text, ";");

	result = ISC_R_SUCCESS;

 cleanup:
#ifdef HAVE_LMDB
	if (nzconfig != NULL)
		cfg_obj_destroy(named_g_addparser, &nzconfig);
#endif /* HAVE_LMDB */
	if (isc_buffer_usedlength(*text) > 0)
		(void) putnull(text);
	if (exclusive)
		isc_task_endexclusive(server->task);

	return (result);
}

static void
newzone_cfgctx_destroy(void **cfgp) {
	ns_cfgctx_t *cfg;

	REQUIRE(cfgp != NULL && *cfgp != NULL);

	cfg = *cfgp;

	if (cfg->conf_parser != NULL) {
		if (cfg->config != NULL)
			cfg_obj_destroy(cfg->conf_parser, &cfg->config);
		if (cfg->vconfig != NULL)
			cfg_obj_destroy(cfg->conf_parser, &cfg->vconfig);
		cfg_parser_destroy(&cfg->conf_parser);
	}
	if (cfg->add_parser != NULL) {
		if (cfg->nzf_config != NULL)
			cfg_obj_destroy(cfg->add_parser, &cfg->nzf_config);
		cfg_parser_destroy(&cfg->add_parser);
	}

	if (cfg->actx != NULL)
		cfg_aclconfctx_detach(&cfg->actx);

	isc_mem_putanddetach(&cfg->mctx, cfg, sizeof(*cfg));
	*cfgp = NULL;
}

static isc_result_t
generate_salt(unsigned char *salt, size_t saltlen) {
	unsigned char text[512 + 1];
	isc_region_t r;
	isc_buffer_t buf;
	isc_result_t result;

	if (saltlen > 256U)
		return (ISC_R_RANGE);

	isc_rng_randombytes(named_g_server->sctx->rngctx, salt, saltlen);

	r.base = salt;
	r.length = (unsigned int) saltlen;

	isc_buffer_init(&buf, text, sizeof(text));
	result = isc_hex_totext(&r, 2, "", &buf);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	text[saltlen * 2] = 0;

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
		      "generated salt: %s", text);

	return (ISC_R_SUCCESS);
}

isc_result_t
named_server_signing(named_server_t *server, isc_lex_t *lex,
		     isc_buffer_t **text)
{
	isc_result_t result = ISC_R_SUCCESS;
	dns_zone_t *zone = NULL;
	dns_name_t *origin;
	dns_db_t *db = NULL;
	dns_dbnode_t *node = NULL;
	dns_dbversion_t *version = NULL;
	dns_rdatatype_t privatetype;
	dns_rdataset_t privset;
	isc_boolean_t first = ISC_TRUE;
	isc_boolean_t list = ISC_FALSE, clear = ISC_FALSE;
	isc_boolean_t chain = ISC_FALSE;
	isc_boolean_t setserial = ISC_FALSE;
	isc_uint32_t serial = 0;
	char keystr[DNS_SECALG_FORMATSIZE + 7]; /* <5-digit keyid>/<alg> */
	unsigned short hash = 0, flags = 0, iter = 0, saltlen = 0;
	unsigned char salt[255];
	const char *ptr;
	size_t n;

	dns_rdataset_init(&privset);

	/* Skip the command name. */
	ptr = next_token(lex, text);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	/* Find out what we are to do. */
	ptr = next_token(lex, text);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	if (strcasecmp(ptr, "-list") == 0)
		list = ISC_TRUE;
	else if ((strcasecmp(ptr, "-clear") == 0)  ||
		 (strcasecmp(ptr, "-clean") == 0))
	{
		clear = ISC_TRUE;
		ptr = next_token(lex, text);
		if (ptr == NULL)
			return (ISC_R_UNEXPECTEDEND);
		strlcpy(keystr, ptr, sizeof(keystr));
	} else if (strcasecmp(ptr, "-nsec3param") == 0) {
		char hashbuf[64], flagbuf[64], iterbuf[64];
		char nbuf[256];

		chain = ISC_TRUE;
		ptr = next_token(lex, text);
		if (ptr == NULL)
			return (ISC_R_UNEXPECTEDEND);

		if (strcasecmp(ptr, "none") == 0)
			hash = 0;
		else {
			strlcpy(hashbuf, ptr, sizeof(hashbuf));

			ptr = next_token(lex, text);
			if (ptr == NULL)
				return (ISC_R_UNEXPECTEDEND);
			strlcpy(flagbuf, ptr, sizeof(flagbuf));

			ptr = next_token(lex, text);
			if (ptr == NULL)
				return (ISC_R_UNEXPECTEDEND);
			strlcpy(iterbuf, ptr, sizeof(iterbuf));

			n = snprintf(nbuf, sizeof(nbuf), "%s %s %s",
				     hashbuf, flagbuf, iterbuf);
			if (n == sizeof(nbuf))
				return (ISC_R_NOSPACE);
			n = sscanf(nbuf, "%hu %hu %hu", &hash, &flags, &iter);
			if (n != 3U)
				return (ISC_R_BADNUMBER);

			if (hash > 0xffU || flags > 0xffU)
				return (ISC_R_RANGE);

			ptr = next_token(lex, text);
			if (ptr == NULL) {
				return (ISC_R_UNEXPECTEDEND);
			} else if (strcasecmp(ptr, "auto") == 0) {
				/* Auto-generate a random salt.
				 * XXXMUKS: This currently uses the
				 * minimum recommended length by RFC
				 * 5155 (64 bits). It should be made
				 * configurable.
				 */
				saltlen = 8;
				CHECK(generate_salt(salt, saltlen));
			} else if (strcmp(ptr, "-") != 0) {
				isc_buffer_t buf;

				isc_buffer_init(&buf, salt, sizeof(salt));
				CHECK(isc_hex_decodestring(ptr, &buf));
				saltlen = isc_buffer_usedlength(&buf);
			}
		}
	} else if (strcasecmp(ptr, "-serial") == 0) {
		ptr = next_token(lex, text);
		if (ptr == NULL)
			return (ISC_R_UNEXPECTEDEND);
		CHECK(isc_parse_uint32(&serial, ptr, 10));
		setserial = ISC_TRUE;
	} else
		CHECK(DNS_R_SYNTAX);

	CHECK(zone_from_args(server, lex, NULL, &zone, NULL,
			     text, ISC_FALSE));
	if (zone == NULL)
		CHECK(ISC_R_UNEXPECTEDEND);

	if (clear) {
		CHECK(dns_zone_keydone(zone, keystr));
		(void) putstr(text, "request queued");
		(void) putnull(text);
	} else if (chain) {
		CHECK(dns_zone_setnsec3param(zone, (isc_uint8_t)hash,
					     (isc_uint8_t)flags, iter,
					     (isc_uint8_t)saltlen, salt,
					     ISC_TRUE));
		(void) putstr(text, "nsec3param request queued");
		(void) putnull(text);
	} else if (setserial) {
		CHECK(dns_zone_setserial(zone, serial));
		(void) putstr(text, "serial request queued");
		(void) putnull(text);
	} else if (list) {
		privatetype = dns_zone_getprivatetype(zone);
		origin = dns_zone_getorigin(zone);
		CHECK(dns_zone_getdb(zone, &db));
		CHECK(dns_db_findnode(db, origin, ISC_FALSE, &node));
		dns_db_currentversion(db, &version);

		result = dns_db_findrdataset(db, node, version, privatetype,
					     dns_rdatatype_none, 0,
					     &privset, NULL);
		if (result == ISC_R_NOTFOUND) {
			(void) putstr(text, "No signing records found");
			(void) putnull(text);
			result = ISC_R_SUCCESS;
			goto cleanup;
		}

		for (result = dns_rdataset_first(&privset);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(&privset))
		{
			dns_rdata_t priv = DNS_RDATA_INIT;
			char output[BUFSIZ];
			isc_buffer_t buf;

			dns_rdataset_current(&privset, &priv);

			isc_buffer_init(&buf, output, sizeof(output));
			CHECK(dns_private_totext(&priv, &buf));
			if (!first)
				CHECK(putstr(text, "\n"));
			CHECK(putstr(text, output));
			first = ISC_FALSE;
		}
		if (!first)
			CHECK(putnull(text));

		if (result == ISC_R_NOMORE)
			result = ISC_R_SUCCESS;
	}

 cleanup:
	if (dns_rdataset_isassociated(&privset))
		dns_rdataset_disassociate(&privset);
	if (node != NULL)
		dns_db_detachnode(db, &node);
	if (version != NULL)
		dns_db_closeversion(db, &version, ISC_FALSE);
	if (db != NULL)
		dns_db_detach(&db);
	if (zone != NULL)
		dns_zone_detach(&zone);

	return (result);
}

static isc_result_t
putmem(isc_buffer_t **b, const char *str, size_t len) {
	isc_result_t result;

	result = isc_buffer_reserve(b, (unsigned int)len);
	if (result != ISC_R_SUCCESS)
		return (ISC_R_NOSPACE);

	isc_buffer_putmem(*b, (const unsigned char *)str, (unsigned int)len);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
putstr(isc_buffer_t **b, const char *str) {
	return (putmem(b, str, strlen(str)));
}

static isc_result_t
putuint8(isc_buffer_t **b, isc_uint8_t val) {
	isc_result_t result;

	result = isc_buffer_reserve(b, 1);
	if (result != ISC_R_SUCCESS)
		return (ISC_R_NOSPACE);

	isc_buffer_putuint8(*b, val);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
putnull(isc_buffer_t **b) {
	return (putuint8(b, 0));
}

isc_result_t
named_server_zonestatus(named_server_t *server, isc_lex_t *lex,
			isc_buffer_t **text)
{
	isc_result_t result = ISC_R_SUCCESS;
	dns_zone_t *zone = NULL, *raw = NULL, *mayberaw = NULL;
	const char *type, *file;
	char zonename[DNS_NAME_FORMATSIZE];
	isc_uint32_t serial, signed_serial, nodes;
	char serbuf[16], sserbuf[16], nodebuf[16];
	char resignbuf[DNS_NAME_FORMATSIZE + DNS_RDATATYPE_FORMATSIZE + 2];
	char lbuf[ISC_FORMATHTTPTIMESTAMP_SIZE];
	char xbuf[ISC_FORMATHTTPTIMESTAMP_SIZE];
	char rbuf[ISC_FORMATHTTPTIMESTAMP_SIZE];
	char kbuf[ISC_FORMATHTTPTIMESTAMP_SIZE];
	char rtbuf[ISC_FORMATHTTPTIMESTAMP_SIZE];
	isc_time_t loadtime, expiretime, refreshtime;
	isc_time_t refreshkeytime, resigntime;
	dns_zonetype_t zonetype;
	isc_boolean_t dynamic = ISC_FALSE, frozen = ISC_FALSE;
	isc_boolean_t hasraw = ISC_FALSE;
	isc_boolean_t secure, maintain, allow;
	dns_db_t *db = NULL, *rawdb = NULL;
	char **incfiles = NULL;
	int nfiles = 0;

	isc_time_settoepoch(&loadtime);
	isc_time_settoepoch(&refreshtime);
	isc_time_settoepoch(&expiretime);
	isc_time_settoepoch(&refreshkeytime);
	isc_time_settoepoch(&resigntime);

	CHECK(zone_from_args(server, lex, NULL, &zone, zonename,
			     text, ISC_TRUE));
	if (zone == NULL) {
		result = ISC_R_UNEXPECTEDEND;
		goto cleanup;
	}

	/* Inline signing? */
	CHECK(dns_zone_getdb(zone, &db));
	dns_zone_getraw(zone, &raw);
	hasraw = ISC_TF(raw != NULL);
	if (hasraw) {
		mayberaw = raw;
		zonetype = dns_zone_gettype(raw);
		CHECK(dns_zone_getdb(raw, &rawdb));
	} else {
		mayberaw = zone;
		zonetype = dns_zone_gettype(zone);
	}

	switch (zonetype) {
	case dns_zone_master:
		type = "master";
		break;
	case dns_zone_slave:
		type = "slave";
		break;
	case dns_zone_stub:
		type = "stub";
		break;
	case dns_zone_staticstub:
		type = "staticstub";
		break;
	case dns_zone_redirect:
		type = "redirect";
		break;
	case dns_zone_key:
		type = "key";
		break;
	case dns_zone_dlz:
		type = "dlz";
		break;
	default:
		type = "unknown";
	}

	/* Serial number */
	serial = dns_zone_getserial(mayberaw);
	snprintf(serbuf, sizeof(serbuf), "%u", serial);
	if (hasraw) {
		signed_serial = dns_zone_getserial(zone);
		snprintf(sserbuf, sizeof(sserbuf), "%u", signed_serial);
	}

	/* Database node count */
	nodes = dns_db_nodecount(hasraw ? rawdb : db);
	snprintf(nodebuf, sizeof(nodebuf), "%u", nodes);

	/* Security */
	secure = dns_db_issecure(db);
	allow = ISC_TF((dns_zone_getkeyopts(zone) & DNS_ZONEKEY_ALLOW) != 0);
	maintain = ISC_TF((dns_zone_getkeyopts(zone) &
			   DNS_ZONEKEY_MAINTAIN) != 0);

	/* Master files */
	file = dns_zone_getfile(mayberaw);
	nfiles = dns_zone_getincludes(mayberaw, &incfiles);

	/* Load time */
	dns_zone_getloadtime(zone, &loadtime);
	isc_time_formathttptimestamp(&loadtime, lbuf, sizeof(lbuf));

	/* Refresh/expire times */
	if (zonetype == dns_zone_slave ||
	    zonetype == dns_zone_stub ||
	    zonetype == dns_zone_redirect)
	{
		dns_zone_getexpiretime(mayberaw, &expiretime);
		isc_time_formathttptimestamp(&expiretime, xbuf, sizeof(xbuf));
		dns_zone_getrefreshtime(mayberaw, &refreshtime);
		isc_time_formathttptimestamp(&refreshtime, rbuf, sizeof(rbuf));
	}

	/* Key refresh time */
	if (zonetype == dns_zone_master ||
	    (zonetype == dns_zone_slave && hasraw))
	{
		dns_zone_getrefreshkeytime(zone, &refreshkeytime);
		isc_time_formathttptimestamp(&refreshkeytime, kbuf,
					     sizeof(kbuf));
	}

	/* Dynamic? */
	if (zonetype == dns_zone_master) {
		dynamic = dns_zone_isdynamic(mayberaw, ISC_TRUE);
		frozen = dynamic && !dns_zone_isdynamic(mayberaw, ISC_FALSE);
	}

	/* Next resign event */
	if (secure && (zonetype == dns_zone_master ||
	     (zonetype == dns_zone_slave && hasraw)) &&
	    ((dns_zone_getkeyopts(zone) & DNS_ZONEKEY_NORESIGN) == 0))
	{
		dns_name_t *name;
		dns_fixedname_t fixed;
		dns_rdataset_t next;

		dns_rdataset_init(&next);
		name = dns_fixedname_initname(&fixed);

		result = dns_db_getsigningtime(db, &next, name);
		if (result == ISC_R_SUCCESS) {
			isc_stdtime_t timenow;
			char namebuf[DNS_NAME_FORMATSIZE];
			char typebuf[DNS_RDATATYPE_FORMATSIZE];

			isc_stdtime_get(&timenow);
			dns_name_format(name, namebuf, sizeof(namebuf));
			dns_rdatatype_format(next.covers,
					     typebuf, sizeof(typebuf));
			snprintf(resignbuf, sizeof(resignbuf),
				     "%s/%s", namebuf, typebuf);
			isc_time_set(&resigntime, next.resign -
				dns_zone_getsigresigninginterval(zone), 0);
			isc_time_formathttptimestamp(&resigntime, rtbuf,
						     sizeof(rtbuf));
			dns_rdataset_disassociate(&next);
		}
	}

	/* Create text */
	CHECK(putstr(text, "name: "));
	CHECK(putstr(text, zonename));

	CHECK(putstr(text, "\ntype: "));
	CHECK(putstr(text, type));

	if (file != NULL) {
		int i;
		CHECK(putstr(text, "\nfiles: "));
		CHECK(putstr(text, file));
		for (i = 0; i < nfiles; i++) {
			CHECK(putstr(text, ", "));
			if (incfiles[i] != NULL) {
				CHECK(putstr(text, incfiles[i]));
			}
		}
	}

	CHECK(putstr(text, "\nserial: "));
	CHECK(putstr(text, serbuf));
	if (hasraw) {
		CHECK(putstr(text, "\nsigned serial: "));
		CHECK(putstr(text, sserbuf));
	}

	CHECK(putstr(text, "\nnodes: "));
	CHECK(putstr(text, nodebuf));

	if (! isc_time_isepoch(&loadtime)) {
		CHECK(putstr(text, "\nlast loaded: "));
		CHECK(putstr(text, lbuf));
	}

	if (! isc_time_isepoch(&refreshtime)) {
		CHECK(putstr(text, "\nnext refresh: "));
		CHECK(putstr(text, rbuf));
	}

	if (! isc_time_isepoch(&expiretime)) {
		CHECK(putstr(text, "\nexpires: "));
		CHECK(putstr(text, xbuf));
	}

	if (secure) {
		CHECK(putstr(text, "\nsecure: yes"));
		if (hasraw) {
			CHECK(putstr(text, "\ninline signing: yes"));
		} else {
			CHECK(putstr(text, "\ninline signing: no"));
		}
	} else {
		CHECK(putstr(text, "\nsecure: no"));
	}

	if (maintain) {
		CHECK(putstr(text, "\nkey maintenance: automatic"));
		if (! isc_time_isepoch(&refreshkeytime)) {
			CHECK(putstr(text, "\nnext key event: "));
			CHECK(putstr(text, kbuf));
		}
	} else if (allow) {
		CHECK(putstr(text, "\nkey maintenance: on command"));
	} else if (secure || hasraw) {
		CHECK(putstr(text, "\nkey maintenance: none"));
	}

	if (!isc_time_isepoch(&resigntime)) {
		CHECK(putstr(text, "\nnext resign node: "));
		CHECK(putstr(text, resignbuf));
		CHECK(putstr(text, "\nnext resign time: "));
		CHECK(putstr(text, rtbuf));
	}

	if (dynamic) {
		CHECK(putstr(text, "\ndynamic: yes"));
		if (frozen) {
			CHECK(putstr(text, "\nfrozen: yes"));
		} else {
			CHECK(putstr(text, "\nfrozen: no"));
		}
	} else {
		CHECK(putstr(text, "\ndynamic: no"));
	}

	CHECK(putstr(text, "\nreconfigurable via modzone: "));
	CHECK(putstr(text, dns_zone_getadded(zone) ? "yes" : "no"));

 cleanup:
	/* Indicate truncated output if possible. */
	if (result == ISC_R_NOSPACE) {
		(void) putstr(text, "\n...");
	}
	if ((result == ISC_R_SUCCESS || result == ISC_R_NOSPACE)) {
		(void) putnull(text);
	}

	if (db != NULL) {
		dns_db_detach(&db);
	}
	if (rawdb != NULL) {
		dns_db_detach(&rawdb);
	}
	if (incfiles != NULL && mayberaw != NULL) {
		int i;
		isc_mem_t *mctx = dns_zone_getmctx(mayberaw);

		for (i = 0; i < nfiles; i++) {
			if (incfiles[i] != NULL) {
				isc_mem_free(mctx, incfiles[i]);
			}
		}
		isc_mem_free(mctx, incfiles);
	}
	if (raw != NULL) {
		dns_zone_detach(&raw);
	}
	if (zone != NULL) {
		dns_zone_detach(&zone);
	}
	return (result);
}

static inline isc_boolean_t
argcheck(char *cmd, const char *full) {
	size_t l;

	if (cmd == NULL || cmd[0] != '-')
		return (ISC_FALSE);

	cmd++;
	l = strlen(cmd);
	if (l > strlen(full) || strncasecmp(cmd, full, l) != 0)
		return (ISC_FALSE);

	return (ISC_TRUE);
}

isc_result_t
named_server_nta(named_server_t *server, isc_lex_t *lex,
		 isc_boolean_t readonly, isc_buffer_t **text)
{
	dns_view_t *view;
	dns_ntatable_t *ntatable = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	char *ptr, *nametext = NULL, *viewname;
	char namebuf[DNS_NAME_FORMATSIZE];
	isc_stdtime_t now, when;
	isc_time_t t;
	char tbuf[64];
	const char *msg = NULL;
	isc_boolean_t dump = ISC_FALSE, force = ISC_FALSE;
	dns_fixedname_t fn;
	const dns_name_t *ntaname;
	dns_name_t *fname;
	dns_ttl_t ntattl;
	isc_boolean_t ttlset = ISC_FALSE, excl = ISC_FALSE;

	UNUSED(force);

	fname = dns_fixedname_initname(&fn);

	/* Skip the command name. */
	ptr = next_token(lex, text);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	for (;;) {
		/* Check for options */
		ptr = next_token(lex, text);
		if (ptr == NULL)
			return (ISC_R_UNEXPECTEDEND);

		if (argcheck(ptr, "dump"))
			dump = ISC_TRUE;
		else if (argcheck(ptr, "remove")) {
			ntattl = 0;
			ttlset = ISC_TRUE;
		} else if (argcheck(ptr, "force")) {
			force = ISC_TRUE;
			continue;
		} else if (argcheck(ptr, "lifetime")) {
			isc_textregion_t tr;

			ptr = next_token(lex, text);
			if (ptr == NULL) {
				msg = "No lifetime specified";
				CHECK(ISC_R_UNEXPECTEDEND);
			}

			tr.base = ptr;
			tr.length = strlen(ptr);
			result = dns_ttl_fromtext(&tr, &ntattl);
			if (result != ISC_R_SUCCESS) {
				msg = "could not parse NTA lifetime";
				CHECK(result);
			}

			if (ntattl > 604800) {
				msg = "NTA lifetime cannot exceed one week";
				CHECK(ISC_R_RANGE);
			}

			ttlset = ISC_TRUE;
			continue;
		} else
			nametext = ptr;

		break;
	}

	/*
	 * If -dump was specified, list NTA's and return
	 */
	if (dump) {
		for (view = ISC_LIST_HEAD(server->viewlist);
		     view != NULL;
		     view = ISC_LIST_NEXT(view, link))
		{
			if (ntatable != NULL)
				dns_ntatable_detach(&ntatable);
			result = dns_view_getntatable(view, &ntatable);
			if (result == ISC_R_NOTFOUND)
				continue;
			CHECK(dns_ntatable_totext(ntatable, text));
		}
		CHECK(putnull(text));

		goto cleanup;
	}

	if (readonly) {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_CONTROL, ISC_LOG_INFO,
			      "rejecting restricted control channel "
			      "NTA command");
		CHECK(ISC_R_FAILURE);
	}

	/* Get the NTA name. */
	if (nametext == NULL)
		nametext = next_token(lex, text);
	if (nametext == NULL)
		return (ISC_R_UNEXPECTEDEND);

	/* Copy nametext as it'll be overwritten by next_token() */
	strlcpy(namebuf, nametext, DNS_NAME_FORMATSIZE);

	if (strcmp(namebuf, ".") == 0)
		ntaname = dns_rootname;
	else {
		isc_buffer_t b;
		isc_buffer_init(&b, namebuf, strlen(namebuf));
		isc_buffer_add(&b, strlen(namebuf));
		CHECK(dns_name_fromtext(fname, &b, dns_rootname, 0, NULL));
		ntaname = fname;
	}

	/* Look for the view name. */
	viewname = next_token(lex, text);

	isc_stdtime_get(&now);

	result = isc_task_beginexclusive(server->task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	excl = ISC_TRUE;
	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		if (viewname != NULL &&
		    strcmp(view->name, viewname) != 0)
			continue;

		if (view->nta_lifetime == 0)
			continue;

		if (!ttlset)
			ntattl = view->nta_lifetime;

		if (ntatable != NULL)
			dns_ntatable_detach(&ntatable);

		result = dns_view_getntatable(view, &ntatable);
		if (result == ISC_R_NOTFOUND) {
			result = ISC_R_SUCCESS;
			continue;
		}

		result = dns_view_flushnode(view, ntaname, ISC_TRUE);
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
			      "flush tree '%s' in cache view '%s': %s",
			      namebuf, view->name,
			      isc_result_totext(result));

		if (ntattl != 0) {
			CHECK(dns_ntatable_add(ntatable, ntaname,
					       force, now, ntattl));

			when = now + ntattl;
			isc_time_set(&t, when, 0);
			isc_time_formattimestamp(&t, tbuf, sizeof(tbuf));

			CHECK(putstr(text, "Negative trust anchor added: "));
			CHECK(putstr(text, namebuf));
			CHECK(putstr(text, "/"));
			CHECK(putstr(text, view->name));
			CHECK(putstr(text, ", expires "));
			CHECK(putstr(text, tbuf));

			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
				      "added NTA '%s' (%d sec) in view '%s'",
				      namebuf, ntattl, view->name);
		} else {
			CHECK(dns_ntatable_delete(ntatable, ntaname));

			CHECK(putstr(text, "Negative trust anchor removed: "));
			CHECK(putstr(text, namebuf));
			CHECK(putstr(text, "/"));
			CHECK(putstr(text, view->name));

			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_INFO,
				      "removed NTA '%s' in view %s",
				      namebuf, view->name);
		}

		result = dns_view_saventa(view);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "error writing NTA file "
				      "for view '%s': %s",
				      view->name, isc_result_totext(result));
		}

		CHECK(putnull(text));

	}

 cleanup:
	if (msg != NULL) {
		(void) putstr(text, msg);
		(void) putnull(text);
	}
	if (excl)
		isc_task_endexclusive(server->task);
	if (ntatable != NULL)
		dns_ntatable_detach(&ntatable);
	return (result);
}

isc_result_t
named_server_saventa(named_server_t *server) {
	dns_view_t *view;

	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		isc_result_t result = dns_view_saventa(view);

		if (result != ISC_R_SUCCESS) {
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "error writing NTA file "
				      "for view '%s': %s",
				      view->name, isc_result_totext(result));
		}
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
named_server_loadnta(named_server_t *server) {
	dns_view_t *view;

	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		isc_result_t result = dns_view_loadnta(view);

		if ((result != ISC_R_SUCCESS) &&
		    (result != ISC_R_FILENOTFOUND) &&
		    (result != ISC_R_NOTFOUND))
		{
			isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
				      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
				      "error loading NTA file "
				      "for view '%s': %s",
				      view->name, isc_result_totext(result));
		}
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
mkey_refresh(dns_view_t *view, isc_buffer_t **text) {
	isc_result_t result;
	char msg[DNS_NAME_FORMATSIZE + 500] = "";

	snprintf(msg, sizeof(msg),
		 "refreshing managed keys for '%s'", view->name);
	CHECK(putstr(text, msg));
	CHECK(dns_zone_synckeyzone(view->managed_keys));

 cleanup:
	return (result);
}

static isc_result_t
mkey_destroy(named_server_t *server, dns_view_t *view, isc_buffer_t **text) {
	isc_result_t result;
	char msg[DNS_NAME_FORMATSIZE + 500] = "";
	isc_boolean_t exclusive = ISC_FALSE;
	const char *file = NULL;
	dns_db_t *dbp = NULL;
	dns_zone_t *mkzone = NULL;
	isc_boolean_t removed_a_file = ISC_FALSE;

	if (view->managed_keys == NULL) {
		CHECK(ISC_R_NOTFOUND);
	}

	snprintf(msg, sizeof(msg),
		 "destroying managed-keys database for '%s'", view->name);
	CHECK(putstr(text, msg));

	result = isc_task_beginexclusive(server->task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	exclusive = ISC_TRUE;

	/* Remove and clean up managed keys zone from view */
	mkzone = view->managed_keys;
	view->managed_keys = NULL;
	(void)dns_zone_flush(mkzone);

	/* Unload zone database */
	if (dns_zone_getdb(mkzone, &dbp) == ISC_R_SUCCESS) {
		dns_db_detach(&dbp);
		dns_zone_unload(mkzone);
	}

	/* Delete files */
	file = dns_zone_getfile(mkzone);
	result = isc_file_remove(file);
	if (result == ISC_R_SUCCESS) {
		removed_a_file = ISC_TRUE;
	} else {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
			      "file %s not removed: %s",
			      file, isc_result_totext(result));
	}

	file = dns_zone_getjournal(mkzone);
	result = isc_file_remove(file);
	if (result == ISC_R_SUCCESS) {
		removed_a_file = ISC_TRUE;
	} else {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
			      "file %s not removed: %s",
			      file, isc_result_totext(result));
	}

	if (!removed_a_file) {
		CHECK(putstr(text, "error: no files could be removed"));
		CHECK(ISC_R_FAILURE);
	}

	dns_zone_detach(&mkzone);
	result = ISC_R_SUCCESS;

 cleanup:
	if (exclusive) {
		isc_task_endexclusive(server->task);
	}
	return (result);
}


static isc_result_t
mkey_dumpzone(dns_view_t *view, isc_buffer_t **text) {
	isc_result_t result;
	dns_db_t *db = NULL;
	dns_dbversion_t *ver = NULL;
	dns_rriterator_t rrit;
	isc_stdtime_t now;
	dns_name_t *prevname = NULL;

	isc_stdtime_get(&now);

	CHECK(dns_zone_getdb(view->managed_keys, &db));
	dns_db_currentversion(db, &ver);
	dns_rriterator_init(&rrit, db, ver, 0);
	for (result = dns_rriterator_first(&rrit);
	     result == ISC_R_SUCCESS;
	     result = dns_rriterator_nextrrset(&rrit))
	{
		char buf[DNS_NAME_FORMATSIZE + 500];
		dns_name_t *name = NULL;
		dns_rdataset_t *kdset = NULL;
		dns_rdata_t rdata = DNS_RDATA_INIT;
		dns_rdata_keydata_t kd;
		isc_uint32_t ttl;

		dns_rriterator_current(&rrit, &name, &ttl, &kdset, NULL);
		if (kdset == NULL || kdset->type != dns_rdatatype_keydata ||
		    !dns_rdataset_isassociated(kdset))
			continue;

		if (name != prevname) {
			char nbuf[DNS_NAME_FORMATSIZE];
			dns_name_format(name, nbuf, sizeof(nbuf));
			snprintf(buf, sizeof(buf), "\n\n    name: %s", nbuf);
			CHECK(putstr(text, buf));
		}


		for (result = dns_rdataset_first(kdset);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(kdset))
		{
			char alg[DNS_SECALG_FORMATSIZE];
			char tbuf[ISC_FORMATHTTPTIMESTAMP_SIZE];
			dns_keytag_t keyid;
			isc_region_t r;
			isc_time_t t;
			isc_boolean_t revoked;

			dns_rdata_reset(&rdata);
			dns_rdataset_current(kdset, &rdata);
			result = dns_rdata_tostruct(&rdata, &kd, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);

			dns_rdata_toregion(&rdata, &r);
			isc_region_consume(&r, 12);
			keyid = dst_region_computeid(&r, kd.algorithm);

			snprintf(buf, sizeof(buf), "\n    keyid: %u", keyid);
			CHECK(putstr(text, buf));

			dns_secalg_format(kd.algorithm, alg, sizeof(alg));
			snprintf(buf, sizeof(buf), "\n\talgorithm: %s", alg);
			CHECK(putstr(text, buf));

			revoked = ISC_TF((kd.flags & DNS_KEYFLAG_REVOKE) != 0);
			snprintf(buf, sizeof(buf), "\n\tflags:%s%s%s",
				 revoked ? " REVOKE" : "",
				 ((kd.flags & DNS_KEYFLAG_KSK) != 0)
				   ? " SEP" : "",
				 (kd.flags == 0) ? " (none)" : "");
			CHECK(putstr(text, buf));

			isc_time_set(&t, kd.refresh, 0);
			isc_time_formathttptimestamp(&t, tbuf, sizeof(tbuf));
			snprintf(buf, sizeof(buf),
				 "\n\tnext refresh: %s", tbuf);
			CHECK(putstr(text, buf));

			if (kd.removehd != 0) {
				isc_time_set(&t, kd.removehd, 0);
				isc_time_formathttptimestamp(&t, tbuf,
							     sizeof(tbuf));
				snprintf(buf, sizeof(buf),
					 "\n\tremove at: %s", tbuf);
				CHECK(putstr(text, buf));
			}

			isc_time_set(&t, kd.addhd, 0);
			isc_time_formathttptimestamp(&t, tbuf, sizeof(tbuf));
			if (kd.addhd == 0)
				snprintf(buf, sizeof(buf), "\n\tno trust");
			else if (revoked)
				snprintf(buf, sizeof(buf),
					 "\n\ttrust revoked");
			else if (kd.addhd <= now)
				snprintf(buf, sizeof(buf),
					 "\n\ttrusted since: %s", tbuf);
			else if (kd.addhd > now)
				snprintf(buf, sizeof(buf),
					 "\n\ttrust pending: %s", tbuf);
			CHECK(putstr(text, buf));
		}
	}

	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;

 cleanup:
	if (ver != NULL) {
		dns_rriterator_destroy(&rrit);
		dns_db_closeversion(db, &ver, ISC_FALSE);
	}
	if (db != NULL)
		dns_db_detach(&db);

	return (result);
}

static isc_result_t
mkey_status(dns_view_t *view, isc_buffer_t **text) {
	isc_result_t result;
	char msg[ISC_FORMATHTTPTIMESTAMP_SIZE];
	isc_time_t t;

	CHECK(putstr(text, "view: "));
	CHECK(putstr(text, view->name));

	CHECK(putstr(text, "\nnext scheduled event: "));

	dns_zone_getrefreshkeytime(view->managed_keys, &t);
	if (isc_time_isepoch(&t)) {
		CHECK(putstr(text, "never"));
	} else {
		isc_time_formathttptimestamp(&t, msg, sizeof(msg));
		CHECK(putstr(text, msg));
	}

	CHECK(mkey_dumpzone(view, text));

 cleanup:
	return (result);
}

isc_result_t
named_server_mkeys(named_server_t *server, isc_lex_t *lex,
		   isc_buffer_t **text)
{
	char *cmd, *classtxt, *viewtxt = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	dns_view_t *view = NULL;
	dns_rdataclass_t rdclass;
	char msg[DNS_NAME_FORMATSIZE + 500] = "";
	enum { NONE, STATUS, REFRESH, SYNC, DESTROY } opt = NONE;
	isc_boolean_t found = ISC_FALSE;
	isc_boolean_t first = ISC_TRUE;

	/* Skip rndc command name */
	cmd = next_token(lex, text);
	if (cmd == NULL) {
		return (ISC_R_UNEXPECTEDEND);
	}

	/* Get managed-keys subcommand */
	cmd = next_token(lex, text);
	if (cmd == NULL) {
		return (ISC_R_UNEXPECTEDEND);
	}

	if (strcasecmp(cmd, "status") == 0) {
		opt = STATUS;
	} else if (strcasecmp(cmd, "refresh") == 0) {
		opt = REFRESH;
	} else if (strcasecmp(cmd, "sync") == 0) {
		opt = SYNC;
	} else if (strcasecmp(cmd, "destroy") == 0) {
		opt = DESTROY;
	} else {
		snprintf(msg, sizeof(msg), "unknown command '%s'", cmd);
		(void) putstr(text, msg);
		result = ISC_R_UNEXPECTED;
		goto cleanup;
	}

	/* Look for the optional class name. */
	classtxt = next_token(lex, text);
	if (classtxt != NULL) {
		/* Look for the optional view name. */
		viewtxt = next_token(lex, text);
	}

	if (classtxt == NULL) {
		rdclass = dns_rdataclass_in;
	} else {
		isc_textregion_t r;
		r.base = classtxt;
		r.length = strlen(classtxt);
		result = dns_rdataclass_fromtext(&rdclass, &r);
		if (result != ISC_R_SUCCESS) {
			if (viewtxt == NULL) {
				rdclass = dns_rdataclass_in;
				viewtxt = classtxt;
				result = ISC_R_SUCCESS;
			} else {
				snprintf(msg, sizeof(msg),
					 "unknown class '%s'", classtxt);
				(void) putstr(text, msg);
				goto cleanup;
			}
		}
	}

	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		if (viewtxt != NULL &&
		    (rdclass != view->rdclass ||
		     strcmp(view->name, viewtxt) != 0))
		{
			continue;
		}

		if (view->managed_keys == NULL) {
			if (viewtxt != NULL) {
				snprintf(msg, sizeof(msg),
					 "view '%s': no managed keys", viewtxt);
				CHECK(putstr(text, msg));
				goto cleanup;
			} else {
				continue;
			}
		}

		found = ISC_TRUE;

		switch (opt) {
		case REFRESH:
			CHECK(mkey_refresh(view, text));
			break;
		case STATUS:
			if (!first) {
				CHECK(putstr(text, "\n\n"));
			}
			CHECK(mkey_status(view, text));
			first = ISC_FALSE;
			break;
		case SYNC:
			CHECK(dns_zone_flush(view->managed_keys));
			break;
		case DESTROY:
			CHECK(mkey_destroy(server, view, text));
			break;
		default:
			INSIST(0);
		}

		if (viewtxt != NULL) {
			break;
		}
	}

	if (!found) {
		CHECK(putstr(text, "no views with managed keys"));
	}

 cleanup:
	if (isc_buffer_usedlength(*text) > 0) {
		(void) putnull(text);
	}

	return (result);
}

isc_result_t
named_server_dnstap(named_server_t *server, isc_lex_t *lex,
		    isc_buffer_t **text)
{
#ifdef HAVE_DNSTAP
	char *ptr;
	isc_result_t result;
	isc_boolean_t reopen = ISC_FALSE;
	int backups = 0;

	if (server->dtenv == NULL)
		return (ISC_R_NOTFOUND);

	/* Check the command name. */
	ptr = next_token(lex, text);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	/* "dnstap-reopen" was used in 9.11.0b1 */
	if (strcasecmp(ptr, "dnstap-reopen") == 0) {
		reopen = ISC_TRUE;
	} else {
		ptr = next_token(lex, text);
		if (ptr == NULL)
			return (ISC_R_UNEXPECTEDEND);
	}

	if (reopen || strcasecmp(ptr, "-reopen") == 0) {
		backups = ISC_LOG_ROLLNEVER;
	} else if ((strcasecmp(ptr, "-roll") == 0)) {
		unsigned int n;
		ptr = next_token(lex, text);
		if (ptr != NULL) {
			unsigned int u;
			n = sscanf(ptr, "%u", &u);
			if (n != 1U || u > INT_MAX)
				return (ISC_R_BADNUMBER);
			backups = u;
		} else {
			backups = ISC_LOG_ROLLINFINITE;
		}
	} else {
		return (DNS_R_SYNTAX);
	}

	result = dns_dt_reopen(server->dtenv, backups);
	return (result);
#else
	UNUSED(server);
	UNUSED(lex);
	UNUSED(text);
	return (ISC_R_NOTIMPLEMENTED);
#endif
}

isc_result_t
named_server_tcptimeouts(isc_lex_t *lex, isc_buffer_t **text) {
	char *ptr;
	isc_result_t result = ISC_R_SUCCESS;
	unsigned int initial, idle, keepalive, advertised;
	char msg[128];

	/* Skip the command name. */
	ptr = next_token(lex, text);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	ns_server_gettimeouts(named_g_server->sctx,
			      &initial, &idle, &keepalive, &advertised);

	/* Look for optional arguments. */
	ptr = next_token(lex, NULL);
	if (ptr != NULL) {
		CHECK(isc_parse_uint32(&initial, ptr, 10));
		if (initial > 1200)
			CHECK(ISC_R_RANGE);
		if (initial < 25)
			CHECK(ISC_R_RANGE);

		ptr = next_token(lex, text);
		if (ptr == NULL)
			return (ISC_R_UNEXPECTEDEND);
		CHECK(isc_parse_uint32(&idle, ptr, 10));
		if (idle > 1200)
			CHECK(ISC_R_RANGE);
		if (idle < 1)
			CHECK(ISC_R_RANGE);

		ptr = next_token(lex, text);
		if (ptr == NULL)
			return (ISC_R_UNEXPECTEDEND);
		CHECK(isc_parse_uint32(&keepalive, ptr, 10));
		if (keepalive > MAX_TCP_TIMEOUT)
			CHECK(ISC_R_RANGE);
		if (keepalive < 1)
			CHECK(ISC_R_RANGE);

		ptr = next_token(lex, text);
		if (ptr == NULL)
			return (ISC_R_UNEXPECTEDEND);
		CHECK(isc_parse_uint32(&advertised, ptr, 10));
		if (advertised > MAX_TCP_TIMEOUT)
			CHECK(ISC_R_RANGE);

		result = isc_task_beginexclusive(named_g_server->task);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);

		ns_server_settimeouts(named_g_server->sctx, initial, idle,
				      keepalive, advertised);

		isc_task_endexclusive(named_g_server->task);
	}

	snprintf(msg, sizeof(msg), "tcp-initial-timeout=%u\n", initial);
	CHECK(putstr(text, msg));
	snprintf(msg, sizeof(msg), "tcp-idle-timeout=%u\n", idle);
	CHECK(putstr(text, msg));
	snprintf(msg, sizeof(msg), "tcp-keepalive-timeout=%u\n", keepalive);
	CHECK(putstr(text, msg));
	snprintf(msg, sizeof(msg), "tcp-advertised-timeout=%u", advertised);
	CHECK(putstr(text, msg));

 cleanup:
	if (isc_buffer_usedlength(*text) > 0)
		(void) putnull(text);

	return (result);
}

isc_result_t
named_server_servestale(named_server_t *server, isc_lex_t *lex,
			isc_buffer_t **text)
{
	char *ptr, *classtxt, *viewtxt = NULL;
	char msg[128];
	dns_rdataclass_t rdclass = dns_rdataclass_in;
	dns_view_t *view;
	isc_boolean_t found = ISC_FALSE;
	dns_stale_answer_t staleanswersok = dns_stale_answer_conf;
	isc_boolean_t wantstatus = ISC_FALSE;
	isc_result_t result = ISC_R_SUCCESS;

	/* Skip the command name. */
	ptr = next_token(lex, text);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	ptr = next_token(lex, NULL);
	if (ptr == NULL)
		return (ISC_R_UNEXPECTEDEND);

	if (!strcasecmp(ptr, "on") || !strcasecmp(ptr, "yes") ||
	    !strcasecmp(ptr, "enable") || !strcasecmp(ptr, "true")) {
		staleanswersok = dns_stale_answer_yes;
	} else if (!strcasecmp(ptr, "off") || !strcasecmp(ptr, "no") ||
		   !strcasecmp(ptr, "disable") || !strcasecmp(ptr, "false")) {
		staleanswersok = dns_stale_answer_no;
	} else if (strcasecmp(ptr, "reset") == 0) {
		staleanswersok = dns_stale_answer_conf;
	} else if (!strcasecmp(ptr, "check") || !strcasecmp(ptr, "status")) {
		wantstatus = ISC_TRUE;
	} else
		return (DNS_R_SYNTAX);

	/* Look for the optional class name. */
	classtxt = next_token(lex, text);
	if (classtxt != NULL) {
		/* Look for the optional view name. */
		viewtxt = next_token(lex, text);
	}

	if (classtxt != NULL) {
		isc_textregion_t r;

		r.base = classtxt;
		r.length = strlen(classtxt);
		result = dns_rdataclass_fromtext(&rdclass, &r);
		if (result != ISC_R_SUCCESS) {
			if (viewtxt != NULL) {
				snprintf(msg, sizeof(msg),
					 "unknown class '%s'", classtxt);
				(void) putstr(text, msg);
				goto cleanup;
			}

			viewtxt = classtxt;
			classtxt = NULL;
		}
	}

	result = isc_task_beginexclusive(server->task);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		dns_ttl_t stale_ttl = 0;
		dns_db_t *db = NULL;

		if (classtxt != NULL && rdclass != view->rdclass)
			continue;

		if (viewtxt != NULL && strcmp(view->name, viewtxt) != 0)
			continue;

		if (!wantstatus) {
			view->staleanswersok = staleanswersok;
			found = ISC_TRUE;
			continue;
		}

		db = NULL;
		dns_db_attach(view->cachedb, &db);
		(void)dns_db_getservestalettl(db, &stale_ttl);
		dns_db_detach(&db);
		if (found)
			CHECK(putstr(text, "\n"));
		CHECK(putstr(text, view->name));
		CHECK(putstr(text, ": "));
		switch (view->staleanswersok) {
		case dns_stale_answer_yes:
			if (stale_ttl > 0)
				CHECK(putstr(text, "on (rndc)"));
			else
				CHECK(putstr(text, "off (not-cached)"));
			break;
		case dns_stale_answer_no:
			CHECK(putstr(text, "off (rndc)"));
			break;
		case dns_stale_answer_conf:
			if (view->staleanswersenable && stale_ttl > 0)
				CHECK(putstr(text, "on"));
			else if (view->staleanswersenable)
				CHECK(putstr(text, "off (not-cached)"));
			else
				CHECK(putstr(text, "off"));
			break;
		}
		if (stale_ttl > 0) {
			snprintf(msg, sizeof(msg),
				 " (stale-answer-ttl=%u max-stale-ttl=%u)",
				 view->staleanswerttl, stale_ttl);
			CHECK(putstr(text, msg));
		}
		found = ISC_TRUE;
	}
	isc_task_endexclusive(named_g_server->task);

	if (!found)
		result = ISC_R_NOTFOUND;

cleanup:
	if (isc_buffer_usedlength(*text) > 0)
		(void) putnull(text);

	return (result);
}
