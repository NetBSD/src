/*	$NetBSD: resolver.c,v 1.31.4.1 2018/04/16 01:57:56 pgoyette Exp $	*/

/*
 * Copyright (C) 2004-2018  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*! \file */

#include <config.h>
#include <ctype.h>

#include <isc/counter.h>
#include <isc/log.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/random.h>
#include <isc/socket.h>
#include <isc/stats.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#ifdef AES_SIT
#include <isc/aes.h>
#else
#include <isc/hmacsha.h>
#endif

#include <dns/acl.h>
#include <dns/adb.h>
#include <dns/cache.h>
#include <dns/db.h>
#include <dns/dispatch.h>
#include <dns/ds.h>
#include <dns/events.h>
#include <dns/forward.h>
#include <dns/keytable.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/ncache.h>
#include <dns/nsec.h>
#include <dns/nsec3.h>
#include <dns/opcode.h>
#include <dns/peer.h>
#include <dns/rbt.h>
#include <dns/rcode.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/resolver.h>
#include <dns/result.h>
#include <dns/rootns.h>
#include <dns/stats.h>
#include <dns/tsig.h>
#include <dns/validator.h>

#ifdef WANT_QUERYTRACE
#define RTRACE(m)       isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_RESOLVER, \
				      DNS_LOGMODULE_RESOLVER, \
				      ISC_LOG_DEBUG(3), \
				      "res %p: %s", res, (m))
#define RRTRACE(r, m)   isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_RESOLVER, \
				      DNS_LOGMODULE_RESOLVER, \
				      ISC_LOG_DEBUG(3), \
				      "res %p: %s", (r), (m))
#define FCTXTRACE(m) \
			isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_RESOLVER, \
				      DNS_LOGMODULE_RESOLVER, \
				      ISC_LOG_DEBUG(3), \
				      "fctx %p(%s): %s", \
				      fctx, fctx->info, (m))
#define FCTXTRACE2(m1, m2) \
			isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_RESOLVER, \
				      DNS_LOGMODULE_RESOLVER, \
				      ISC_LOG_DEBUG(3), \
				      "fctx %p(%s): %s %s", \
				      fctx, fctx->info, (m1), (m2))
#define FCTXTRACE3(m, res) \
			isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_RESOLVER, \
				      DNS_LOGMODULE_RESOLVER, \
				      ISC_LOG_DEBUG(3), \
				      "fctx %p(%s): [result: %s] %s", \
				      fctx, fctx->info, \
				      isc_result_totext(res), (m))
#define FCTXTRACE4(m1, m2, res) \
			isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_RESOLVER, \
				      DNS_LOGMODULE_RESOLVER, \
				      ISC_LOG_DEBUG(3), \
				      "fctx %p(%s): [result: %s] %s %s", \
				      fctx, fctx->info, \
				      isc_result_totext(res), (m1), (m2))
#define FCTXTRACE5(m1, m2, v) \
			isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_RESOLVER, \
				      DNS_LOGMODULE_RESOLVER, \
				      ISC_LOG_DEBUG(3), \
				      "fctx %p(%s): %s %s%u", \
				      fctx, fctx->info, (m1), (m2), (v))
#define FTRACE(m)       isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_RESOLVER, \
				      DNS_LOGMODULE_RESOLVER, \
				      ISC_LOG_DEBUG(3), \
				      "fetch %p (fctx %p(%s)): %s", \
				      fetch, fetch->private, \
				      fetch->private->info, (m))
#define QTRACE(m)       isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_RESOLVER, \
				      DNS_LOGMODULE_RESOLVER, \
				      ISC_LOG_DEBUG(3), \
				      "resquery %p (fctx %p(%s)): %s", \
				      query, query->fctx, \
				      query->fctx->info, (m))
#else
#define RTRACE(m) do { UNUSED(m); } while (0)
#define RRTRACE(r, m) do { UNUSED(r); UNUSED(m); } while (0)
#define FCTXTRACE(m) do { UNUSED(m); } while (0)
#define FCTXTRACE2(m1, m2) do { UNUSED(m1); UNUSED(m2); } while (0)
#define FCTXTRACE3(m1, res) do { UNUSED(m1); UNUSED(res); } while (0)
#define FCTXTRACE4(m1, m2, res) \
	do { UNUSED(m1); UNUSED(m2); UNUSED(res); } while (0)
#define FCTXTRACE5(m1, m2, v) \
	do { UNUSED(m1); UNUSED(m2); UNUSED(v); } while (0)
#define FTRACE(m) do { UNUSED(m); } while (0)
#define QTRACE(m) do { UNUSED(m); } while (0)
#endif /* WANT_QUERYTRACE */

#define US_PER_SEC 1000000U
/*
 * The maximum time we will wait for a single query.
 */
#define MAX_SINGLE_QUERY_TIMEOUT 9U
#define MAX_SINGLE_QUERY_TIMEOUT_US (MAX_SINGLE_QUERY_TIMEOUT*US_PER_SEC)

/*
 * We need to allow a individual query time to complete / timeout.
 */
#define MINIMUM_QUERY_TIMEOUT (MAX_SINGLE_QUERY_TIMEOUT + 1U)

/* The default time in seconds for the whole query to live. */
#ifndef DEFAULT_QUERY_TIMEOUT
#define DEFAULT_QUERY_TIMEOUT MINIMUM_QUERY_TIMEOUT
#endif

/* The maximum time in seconds for the whole query to live. */
#ifndef MAXIMUM_QUERY_TIMEOUT
#define MAXIMUM_QUERY_TIMEOUT 30
#endif

/* The default maximum number of recursions to follow before giving up. */
#ifndef DEFAULT_RECURSION_DEPTH
#define DEFAULT_RECURSION_DEPTH 7
#endif

/* The default maximum number of iterative queries to allow before giving up. */
#ifndef DEFAULT_MAX_QUERIES
#define DEFAULT_MAX_QUERIES 75
#endif

/* Number of hash buckets for zone counters */
#ifndef RES_DOMAIN_BUCKETS
#define RES_DOMAIN_BUCKETS	523
#endif
#define RES_NOBUCKET		0xffffffff

/*%
 * Maximum EDNS0 input packet size.
 */
#define RECV_BUFFER_SIZE                4096            /* XXXRTH  Constant. */

/*%
 * This defines the maximum number of timeouts we will permit before we
 * disable EDNS0 on the query.
 */
#define MAX_EDNS0_TIMEOUTS      3

typedef struct fetchctx fetchctx_t;

typedef struct query {
	/* Locked by task event serialization. */
	unsigned int			magic;
	fetchctx_t *			fctx;
	isc_mem_t *			mctx;
	dns_dispatchmgr_t *		dispatchmgr;
	dns_dispatch_t *		dispatch;
	isc_boolean_t			exclusivesocket;
	dns_adbaddrinfo_t *		addrinfo;
	isc_socket_t *			tcpsocket;
	isc_time_t			start;
	dns_messageid_t			id;
	dns_dispentry_t *		dispentry;
	ISC_LINK(struct query)		link;
	isc_buffer_t			buffer;
	isc_buffer_t			*tsig;
	dns_tsigkey_t			*tsigkey;
	isc_socketevent_t		sendevent;
	isc_dscp_t			dscp;
	int 				ednsversion;
	unsigned int			options;
	unsigned int			attributes;
	unsigned int			sends;
	unsigned int			connects;
	unsigned int			udpsize;
	unsigned char			data[512];
} resquery_t;

struct tried {
	isc_sockaddr_t			addr;
	unsigned int			count;
	ISC_LINK(struct tried)		link;
};

#define QUERY_MAGIC			ISC_MAGIC('Q', '!', '!', '!')
#define VALID_QUERY(query)		ISC_MAGIC_VALID(query, QUERY_MAGIC)

#define RESQUERY_ATTR_CANCELED          0x02

#define RESQUERY_CONNECTING(q)          ((q)->connects > 0)
#define RESQUERY_CANCELED(q)            (((q)->attributes & \
					  RESQUERY_ATTR_CANCELED) != 0)
#define RESQUERY_SENDING(q)             ((q)->sends > 0)

typedef enum {
	fetchstate_init = 0,            /*%< Start event has not run yet. */
	fetchstate_active,
	fetchstate_done                 /*%< FETCHDONE events posted. */
} fetchstate;

typedef enum {
	badns_unreachable = 0,
	badns_response,
	badns_validation
} badnstype_t;

struct fetchctx {
	/*% Not locked. */
	unsigned int			magic;
	dns_resolver_t *		res;
	dns_name_t			name;
	dns_rdatatype_t			type;
	unsigned int			options;
	unsigned int			bucketnum;
	unsigned int			dbucketnum;
	char *				info;
	isc_mem_t *			mctx;

	/*% Locked by appropriate bucket lock. */
	fetchstate			state;
	isc_boolean_t			want_shutdown;
	isc_boolean_t			cloned;
	isc_boolean_t			spilled;
	unsigned int			references;
	isc_event_t			control_event;
	ISC_LINK(struct fetchctx)       link;
	ISC_LIST(dns_fetchevent_t)      events;
	/*% Locked by task event serialization. */
	dns_name_t			domain;
	dns_rdataset_t			nameservers;
	unsigned int			attributes;
	isc_timer_t *			timer;
	isc_time_t			expires;
	isc_interval_t			interval;
	dns_message_t *			qmessage;
	dns_message_t *			rmessage;
	ISC_LIST(resquery_t)		queries;
	dns_adbfindlist_t		finds;
	dns_adbfind_t *			find;
	dns_adbfindlist_t		altfinds;
	dns_adbfind_t *			altfind;
	dns_adbaddrinfolist_t		forwaddrs;
	dns_adbaddrinfolist_t		altaddrs;
	dns_forwarderlist_t		forwarders;
	dns_fwdpolicy_t			fwdpolicy;
	isc_sockaddrlist_t		bad;
	ISC_LIST(struct tried)		edns;
	ISC_LIST(struct tried)		edns512;
	isc_sockaddrlist_t		bad_edns;
	dns_validator_t *		validator;
	ISC_LIST(dns_validator_t)       validators;
	dns_db_t *			cache;
	dns_adb_t *			adb;
	isc_boolean_t			ns_ttl_ok;
	isc_uint32_t			ns_ttl;
	isc_counter_t *			qc;

	/*%
	 * The number of events we're waiting for.
	 */
	unsigned int			pending;

	/*%
	 * The number of times we've "restarted" the current
	 * nameserver set.  This acts as a failsafe to prevent
	 * us from pounding constantly on a particular set of
	 * servers that, for whatever reason, are not giving
	 * us useful responses, but are responding in such a
	 * way that they are not marked "bad".
	 */
	unsigned int			restarts;

	/*%
	 * The number of timeouts that have occurred since we
	 * last successfully received a response packet.  This
	 * is used for EDNS0 black hole detection.
	 */
	unsigned int			timeouts;

	/*%
	 * Look aside state for DS lookups.
	 */
	dns_name_t 			nsname;
	dns_fetch_t *			nsfetch;
	dns_rdataset_t			nsrrset;

	/*%
	 * Number of queries that reference this context.
	 */
	unsigned int			nqueries;

	/*%
	 * The reason to print when logging a successful
	 * response to a query.
	 */
	const char *			reason;

	/*%
	 * Random numbers to use for mixing up server addresses.
	 */
	isc_uint32_t                    rand_buf;
	isc_uint32_t                    rand_bits;

	/*%
	 * Fetch-local statistics for detailed logging.
	 */
	isc_result_t			result; /*%< fetch result  */
	isc_result_t			vresult; /*%< validation result  */
	int				exitline;
	isc_time_t			start;
	isc_uint64_t			duration;
	isc_boolean_t			logged;
	unsigned int			querysent;
	unsigned int			referrals;
	unsigned int			lamecount;
	unsigned int			quotacount;
	unsigned int			neterr;
	unsigned int			badresp;
	unsigned int			adberr;
	unsigned int			findfail;
	unsigned int			valfail;
	isc_boolean_t			timeout;
	dns_adbaddrinfo_t 		*addrinfo;
	isc_sockaddr_t			*client;
	unsigned int			depth;
};

#define FCTX_MAGIC			ISC_MAGIC('F', '!', '!', '!')
#define VALID_FCTX(fctx)		ISC_MAGIC_VALID(fctx, FCTX_MAGIC)

#define FCTX_ATTR_HAVEANSWER            0x0001
#define FCTX_ATTR_GLUING                0x0002
#define FCTX_ATTR_ADDRWAIT              0x0004
#define FCTX_ATTR_SHUTTINGDOWN          0x0008
#define FCTX_ATTR_WANTCACHE             0x0010
#define FCTX_ATTR_WANTNCACHE            0x0020
#define FCTX_ATTR_NEEDEDNS0             0x0040
#define FCTX_ATTR_TRIEDFIND             0x0080
#define FCTX_ATTR_TRIEDALT              0x0100

#define HAVE_ANSWER(f)          (((f)->attributes & FCTX_ATTR_HAVEANSWER) != \
				 0)
#define GLUING(f)               (((f)->attributes & FCTX_ATTR_GLUING) != \
				 0)
#define ADDRWAIT(f)             (((f)->attributes & FCTX_ATTR_ADDRWAIT) != \
				 0)
#define SHUTTINGDOWN(f)         (((f)->attributes & FCTX_ATTR_SHUTTINGDOWN) \
				 != 0)
#define WANTCACHE(f)            (((f)->attributes & FCTX_ATTR_WANTCACHE) != 0)
#define WANTNCACHE(f)           (((f)->attributes & FCTX_ATTR_WANTNCACHE) != 0)
#define NEEDEDNS0(f)            (((f)->attributes & FCTX_ATTR_NEEDEDNS0) != 0)
#define TRIEDFIND(f)            (((f)->attributes & FCTX_ATTR_TRIEDFIND) != 0)
#define TRIEDALT(f)             (((f)->attributes & FCTX_ATTR_TRIEDALT) != 0)

typedef struct {
	dns_adbaddrinfo_t *		addrinfo;
	fetchctx_t *			fctx;
} dns_valarg_t;

struct dns_fetch {
	unsigned int			magic;
	isc_mem_t *			mctx;
	fetchctx_t *			private;
};

#define DNS_FETCH_MAGIC			ISC_MAGIC('F', 't', 'c', 'h')
#define DNS_FETCH_VALID(fetch)		ISC_MAGIC_VALID(fetch, DNS_FETCH_MAGIC)

typedef struct fctxbucket {
	isc_task_t *			task;
	isc_mutex_t			lock;
	ISC_LIST(fetchctx_t)		fctxs;
	isc_boolean_t			exiting;
	isc_mem_t *			mctx;
} fctxbucket_t;

#ifdef ENABLE_FETCHLIMIT
typedef struct fctxcount fctxcount_t;
struct fctxcount {
	dns_fixedname_t			fdname;
	dns_name_t			*domain;
	isc_uint32_t			count;
	isc_uint32_t			allowed;
	isc_uint32_t			dropped;
	isc_stdtime_t			logged;
	ISC_LINK(fctxcount_t)		link;
};

typedef struct zonebucket {
	isc_mutex_t			lock;
	isc_mem_t 			*mctx;
	ISC_LIST(fctxcount_t)		list;
} zonebucket_t;
#endif /* ENABLE_FETCHLIMIT */

typedef struct alternate {
	isc_boolean_t			isaddress;
	union   {
		isc_sockaddr_t		addr;
		struct {
			dns_name_t      name;
			in_port_t       port;
		} _n;
	} _u;
	ISC_LINK(struct alternate)      link;
} alternate_t;

typedef struct dns_badcache dns_badcache_t;
struct dns_badcache {
	dns_badcache_t *	next;
	dns_rdatatype_t 	type;
	isc_time_t		expire;
	unsigned int		hashval;
	dns_name_t		name;
};
#define DNS_BADCACHE_SIZE 1021
#define DNS_BADCACHE_TTL(fctx) \
	(((fctx)->res->lame_ttl > 30 ) ? (fctx)->res->lame_ttl : 30)

struct dns_resolver {
	/* Unlocked. */
	unsigned int			magic;
	isc_mem_t *			mctx;
	isc_mutex_t			lock;
	isc_mutex_t			nlock;
	isc_mutex_t			primelock;
	dns_rdataclass_t		rdclass;
	isc_socketmgr_t *		socketmgr;
	isc_timermgr_t *		timermgr;
	isc_taskmgr_t *			taskmgr;
	dns_view_t *			view;
	isc_boolean_t			frozen;
	unsigned int			options;
	dns_dispatchmgr_t *		dispatchmgr;
	dns_dispatchset_t *		dispatches4;
	isc_boolean_t			exclusivev4;
	dns_dispatchset_t *		dispatches6;
	isc_dscp_t			querydscp4;
	isc_dscp_t			querydscp6;
	isc_boolean_t			exclusivev6;
	unsigned int			nbuckets;
	fctxbucket_t *			buckets;
#ifdef ENABLE_FETCHLIMIT
	zonebucket_t *			dbuckets;
#endif /* ENABLE_FETCHLIMIT */
	isc_uint32_t			lame_ttl;
	ISC_LIST(alternate_t)		alternates;
	isc_uint16_t			udpsize;
#if USE_ALGLOCK
	isc_rwlock_t			alglock;
#endif
	dns_rbt_t *			algorithms;
	dns_rbt_t *			digests;
#if USE_MBSLOCK
	isc_rwlock_t			mbslock;
#endif
	dns_rbt_t *			mustbesecure;
	unsigned int			spillatmax;
	unsigned int			spillatmin;
	isc_timer_t *			spillattimer;
	isc_boolean_t			zero_no_soa_ttl;
	unsigned int			query_timeout;
	unsigned int			maxdepth;
	unsigned int			maxqueries;
	isc_result_t			quotaresp[2];

	/* Locked by lock. */
	unsigned int			references;
	isc_boolean_t			exiting;
	isc_eventlist_t			whenshutdown;
	unsigned int			activebuckets;
	isc_boolean_t			priming;
	unsigned int			spillat;	/* clients-per-query */
	unsigned int			zspill;		/* fetches-per-zone */

	/* Bad cache. */
	dns_badcache_t  ** 		badcache;
	unsigned int 			badcount;
	unsigned int 			badhash;
	unsigned int 			badsweep;

	/* Locked by primelock. */
	dns_fetch_t *			primefetch;
	/* Locked by nlock. */
	unsigned int			nfctx;
};

#define RES_MAGIC			ISC_MAGIC('R', 'e', 's', '!')
#define VALID_RESOLVER(res)		ISC_MAGIC_VALID(res, RES_MAGIC)

/*%
 * Private addrinfo flags.  These must not conflict with DNS_FETCHOPT_NOEDNS0
 * (0x008) which we also use as an addrinfo flag.
 */
#define FCTX_ADDRINFO_MARK              0x0001
#define FCTX_ADDRINFO_FORWARDER         0x1000
#define FCTX_ADDRINFO_EDNSOK            0x4000
#define FCTX_ADDRINFO_NOSIT             0x8000
#define FCTX_ADDRINFO_BADCOOKIE         0x10000

#define UNMARKED(a)                     (((a)->flags & FCTX_ADDRINFO_MARK) \
					 == 0)
#define ISFORWARDER(a)                  (((a)->flags & \
					 FCTX_ADDRINFO_FORWARDER) != 0)
#define NOSIT(a)                        (((a)->flags & \
					 FCTX_ADDRINFO_NOSIT) != 0)
#define EDNSOK(a)                       (((a)->flags & \
					 FCTX_ADDRINFO_EDNSOK) != 0)
#define BADCOOKIE(a)                       (((a)->flags & \
					 FCTX_ADDRINFO_BADCOOKIE) != 0)


#define NXDOMAIN(r) (((r)->attributes & DNS_RDATASETATTR_NXDOMAIN) != 0)
#define NEGATIVE(r) (((r)->attributes & DNS_RDATASETATTR_NEGATIVE) != 0)

static void destroy(dns_resolver_t *res);
static void empty_bucket(dns_resolver_t *res);
static isc_result_t resquery_send(resquery_t *query);
static void resquery_response(isc_task_t *task, isc_event_t *event);
static void resquery_connected(isc_task_t *task, isc_event_t *event);
static void fctx_try(fetchctx_t *fctx, isc_boolean_t retrying,
		     isc_boolean_t badcache);
static void fctx_destroy(fetchctx_t *fctx);
static isc_boolean_t fctx_unlink(fetchctx_t *fctx);
static isc_result_t ncache_adderesult(dns_message_t *message,
				      dns_db_t *cache, dns_dbnode_t *node,
				      dns_rdatatype_t covers,
				      isc_stdtime_t now, dns_ttl_t maxttl,
				      isc_boolean_t optout,
				      isc_boolean_t secure,
				      dns_rdataset_t *ardataset,
				      isc_result_t *eresultp);
static void validated(isc_task_t *task, isc_event_t *event);
static isc_boolean_t maybe_destroy(fetchctx_t *fctx, isc_boolean_t locked);
static void add_bad(fetchctx_t *fctx, dns_adbaddrinfo_t *addrinfo,
		    isc_result_t reason, badnstype_t badtype);
static inline isc_result_t findnoqname(fetchctx_t *fctx, dns_name_t *name,
				       dns_rdatatype_t type,
				       dns_name_t **noqname);
static void fctx_increference(fetchctx_t *fctx);
static isc_boolean_t fctx_decreference(fetchctx_t *fctx);

/*%
 * Increment resolver-related statistics counters.
 */
static inline void
inc_stats(dns_resolver_t *res, isc_statscounter_t counter) {
	if (res->view->resstats != NULL)
		isc_stats_increment(res->view->resstats, counter);
}

static inline void
dec_stats(dns_resolver_t *res, isc_statscounter_t counter) {
	if (res->view->resstats != NULL)
		isc_stats_decrement(res->view->resstats, counter);
}

static isc_result_t
valcreate(fetchctx_t *fctx, dns_adbaddrinfo_t *addrinfo, dns_name_t *name,
	  dns_rdatatype_t type, dns_rdataset_t *rdataset,
	  dns_rdataset_t *sigrdataset, unsigned int valoptions,
	  isc_task_t *task)
{
	dns_validator_t *validator = NULL;
	dns_valarg_t *valarg;
	isc_result_t result;

	valarg = isc_mem_get(fctx->mctx, sizeof(*valarg));
	if (valarg == NULL)
		return (ISC_R_NOMEMORY);

	valarg->fctx = fctx;
	valarg->addrinfo = addrinfo;

	if (!ISC_LIST_EMPTY(fctx->validators))
		valoptions |= DNS_VALIDATOR_DEFER;
	else
		valoptions &= ~DNS_VALIDATOR_DEFER;

	result = dns_validator_create(fctx->res->view, name, type, rdataset,
				      sigrdataset, fctx->rmessage,
				      valoptions, task, validated, valarg,
				      &validator);
	if (result == ISC_R_SUCCESS) {
		inc_stats(fctx->res, dns_resstatscounter_val);
		if ((valoptions & DNS_VALIDATOR_DEFER) == 0) {
			INSIST(fctx->validator == NULL);
			fctx->validator = validator;
		}
		ISC_LIST_APPEND(fctx->validators, validator, link);
	} else
		isc_mem_put(fctx->mctx, valarg, sizeof(*valarg));
	return (result);
}

static isc_boolean_t
rrsig_fromchildzone(fetchctx_t *fctx, dns_rdataset_t *rdataset) {
	dns_namereln_t namereln;
	dns_rdata_rrsig_t rrsig;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	int order;
	isc_result_t result;
	unsigned int labels;

	for (result = dns_rdataset_first(rdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(rdataset)) {
		dns_rdataset_current(rdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &rrsig, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		namereln = dns_name_fullcompare(&rrsig.signer, &fctx->domain,
						&order, &labels);
		if (namereln == dns_namereln_subdomain)
			return (ISC_TRUE);
		dns_rdata_reset(&rdata);
	}
	return (ISC_FALSE);
}

static isc_boolean_t
fix_mustbedelegationornxdomain(dns_message_t *message, fetchctx_t *fctx) {
	dns_name_t *name;
	dns_name_t *domain = &fctx->domain;
	dns_rdataset_t *rdataset;
	dns_rdatatype_t type;
	isc_result_t result;
	isc_boolean_t keep_auth = ISC_FALSE;

	if (message->rcode == dns_rcode_nxdomain)
		return (ISC_FALSE);

	/*
	 * A DS RRset can appear anywhere in a zone, even for a delegation-only
	 * zone.  So a response to an explicit query for this type should be
	 * excluded from delegation-only fixup.
	 *
	 * SOA, NS, and DNSKEY can only exist at a zone apex, so a postive
	 * response to a query for these types can never violate the
	 * delegation-only assumption: if the query name is below a
	 * zone cut, the response should normally be a referral, which should
	 * be accepted; if the query name is below a zone cut but the server
	 * happens to have authority for the zone of the query name, the
	 * response is a (non-referral) answer.  But this does not violate
	 * delegation-only because the query name must be in a different zone
	 * due to the "apex-only" nature of these types.  Note that if the
	 * remote server happens to have authority for a child zone of a
	 * delegation-only zone, we may still incorrectly "fix" the response
	 * with NXDOMAIN for queries for other types.  Unfortunately it's
	 * generally impossible to differentiate this case from violation of
	 * the delegation-only assumption.  Once the resolver learns the
	 * correct zone cut, possibly via a separate query for an "apex-only"
	 * type, queries for other types will be resolved correctly.
	 *
	 * A query for type ANY will be accepted if it hits an exceptional
	 * type above in the answer section as it should be from a child
	 * zone.
	 *
	 * Also accept answers with RRSIG records from the child zone.
	 * Direct queries for RRSIG records should not be answered from
	 * the parent zone.
	 */

	if (message->counts[DNS_SECTION_ANSWER] != 0 &&
	    (fctx->type == dns_rdatatype_ns ||
	     fctx->type == dns_rdatatype_ds ||
	     fctx->type == dns_rdatatype_soa ||
	     fctx->type == dns_rdatatype_any ||
	     fctx->type == dns_rdatatype_rrsig ||
	     fctx->type == dns_rdatatype_dnskey)) {
		result = dns_message_firstname(message, DNS_SECTION_ANSWER);
		while (result == ISC_R_SUCCESS) {
			name = NULL;
			dns_message_currentname(message, DNS_SECTION_ANSWER,
						&name);
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link)) {
				if (!dns_name_equal(name, &fctx->name))
					continue;
				type = rdataset->type;
				/*
				 * RRsig from child?
				 */
				if (type == dns_rdatatype_rrsig &&
				    rrsig_fromchildzone(fctx, rdataset))
					return (ISC_FALSE);
				/*
				 * Direct query for apex records or DS.
				 */
				if (fctx->type == type &&
				    (type == dns_rdatatype_ds ||
				     type == dns_rdatatype_ns ||
				     type == dns_rdatatype_soa ||
				     type == dns_rdatatype_dnskey))
					return (ISC_FALSE);
				/*
				 * Indirect query for apex records or DS.
				 */
				if (fctx->type == dns_rdatatype_any &&
				    (type == dns_rdatatype_ns ||
				     type == dns_rdatatype_ds ||
				     type == dns_rdatatype_soa ||
				     type == dns_rdatatype_dnskey))
					return (ISC_FALSE);
			}
			result = dns_message_nextname(message,
						      DNS_SECTION_ANSWER);
		}
	}

	/*
	 * A NODATA response to a DS query?
	 */
	if (fctx->type == dns_rdatatype_ds &&
	    message->counts[DNS_SECTION_ANSWER] == 0)
		return (ISC_FALSE);

	/* Look for referral or indication of answer from child zone? */
	if (message->counts[DNS_SECTION_AUTHORITY] == 0)
		goto munge;

	result = dns_message_firstname(message, DNS_SECTION_AUTHORITY);
	while (result == ISC_R_SUCCESS) {
		name = NULL;
		dns_message_currentname(message, DNS_SECTION_AUTHORITY, &name);
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link)) {
			type = rdataset->type;
			if (type == dns_rdatatype_soa &&
			    dns_name_equal(name, domain))
				keep_auth = ISC_TRUE;

			if (type != dns_rdatatype_ns &&
			    type != dns_rdatatype_soa &&
			    type != dns_rdatatype_rrsig)
				continue;

			if (type == dns_rdatatype_rrsig) {
				if (rrsig_fromchildzone(fctx, rdataset))
					return (ISC_FALSE);
				else
					continue;
			}

			/* NS or SOA records. */
			if (dns_name_equal(name, domain)) {
				/*
				 * If a query for ANY causes a negative
				 * response, we can be sure that this is
				 * an empty node.  For other type of queries
				 * we cannot differentiate an empty node
				 * from a node that just doesn't have that
				 * type of record.  We only accept the former
				 * case.
				 */
				if (message->counts[DNS_SECTION_ANSWER] == 0 &&
				    fctx->type == dns_rdatatype_any)
					return (ISC_FALSE);
			} else if (dns_name_issubdomain(name, domain)) {
				/* Referral or answer from child zone. */
				return (ISC_FALSE);
			}
		}
		result = dns_message_nextname(message, DNS_SECTION_AUTHORITY);
	}

 munge:
	message->rcode = dns_rcode_nxdomain;
	message->counts[DNS_SECTION_ANSWER] = 0;
	if (!keep_auth)
		message->counts[DNS_SECTION_AUTHORITY] = 0;
	message->counts[DNS_SECTION_ADDITIONAL] = 0;
	return (ISC_TRUE);
}

static inline isc_result_t
fctx_starttimer(fetchctx_t *fctx) {
	/*
	 * Start the lifetime timer for fctx.
	 *
	 * This is also used for stopping the idle timer; in that
	 * case we must purge events already posted to ensure that
	 * no further idle events are delivered.
	 */
	return (isc_timer_reset(fctx->timer, isc_timertype_once,
				&fctx->expires, NULL, ISC_TRUE));
}

static inline void
fctx_stoptimer(fetchctx_t *fctx) {
	isc_result_t result;

	/*
	 * We don't return a result if resetting the timer to inactive fails
	 * since there's nothing to be done about it.  Resetting to inactive
	 * should never fail anyway, since the code as currently written
	 * cannot fail in that case.
	 */
	result = isc_timer_reset(fctx->timer, isc_timertype_inactive,
				 NULL, NULL, ISC_TRUE);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_timer_reset(): %s",
				 isc_result_totext(result));
	}
}

static inline isc_result_t
fctx_startidletimer(fetchctx_t *fctx, isc_interval_t *interval) {
	/*
	 * Start the idle timer for fctx.  The lifetime timer continues
	 * to be in effect.
	 */
	return (isc_timer_reset(fctx->timer, isc_timertype_once,
				&fctx->expires, interval, ISC_FALSE));
}

/*
 * Stopping the idle timer is equivalent to calling fctx_starttimer(), but
 * we use fctx_stopidletimer for readability in the code below.
 */
#define fctx_stopidletimer      fctx_starttimer

static inline void
resquery_destroy(resquery_t **queryp) {
	dns_resolver_t *res;
	isc_boolean_t empty;
	resquery_t *query;
	fetchctx_t *fctx;
	unsigned int bucket;

	REQUIRE(queryp != NULL);
	query = *queryp;
	REQUIRE(!ISC_LINK_LINKED(query, link));

	INSIST(query->tcpsocket == NULL);

	fctx = query->fctx;
	res = fctx->res;
	bucket = fctx->bucketnum;

	fctx->nqueries--;

	LOCK(&res->buckets[bucket].lock);
	empty = fctx_decreference(query->fctx);
	UNLOCK(&res->buckets[bucket].lock);

	query->magic = 0;
	isc_mem_put(query->mctx, query, sizeof(*query));
	*queryp = NULL;

	if (empty)
		empty_bucket(res);
}

static void
fctx_cancelquery(resquery_t **queryp, dns_dispatchevent_t **deventp,
		 isc_time_t *finish, isc_boolean_t no_response,
		 isc_boolean_t age_untried)
{
	fetchctx_t *fctx;
	resquery_t *query;
	unsigned int rtt, rttms;
	unsigned int factor;
	dns_adbfind_t *find;
	dns_adbaddrinfo_t *addrinfo;
	isc_socket_t *sock;
	isc_stdtime_t now;

	query = *queryp;
	fctx = query->fctx;

	FCTXTRACE("cancelquery");

	REQUIRE(!RESQUERY_CANCELED(query));

	query->attributes |= RESQUERY_ATTR_CANCELED;

	/*
	 * Should we update the RTT?
	 */
	if (finish != NULL || no_response) {
		if (finish != NULL) {
			/*
			 * We have both the start and finish times for this
			 * packet, so we can compute a real RTT.
			 */
			rtt = (unsigned int)isc_time_microdiff(finish,
							       &query->start);
			factor = DNS_ADB_RTTADJDEFAULT;

			rttms = rtt / 1000;
			if (rttms < DNS_RESOLVER_QRYRTTCLASS0) {
				inc_stats(fctx->res,
					  dns_resstatscounter_queryrtt0);
			} else if (rttms < DNS_RESOLVER_QRYRTTCLASS1) {
				inc_stats(fctx->res,
					  dns_resstatscounter_queryrtt1);
			} else if (rttms < DNS_RESOLVER_QRYRTTCLASS2) {
				inc_stats(fctx->res,
					  dns_resstatscounter_queryrtt2);
			} else if (rttms < DNS_RESOLVER_QRYRTTCLASS3) {
				inc_stats(fctx->res,
					  dns_resstatscounter_queryrtt3);
			} else if (rttms < DNS_RESOLVER_QRYRTTCLASS4) {
				inc_stats(fctx->res,
					  dns_resstatscounter_queryrtt4);
			} else {
				inc_stats(fctx->res,
					  dns_resstatscounter_queryrtt5);
			}
		} else {
			isc_uint32_t value;
			isc_uint32_t mask;

			if ((query->options & DNS_FETCHOPT_NOEDNS0) == 0)
				dns_adb_ednsto(fctx->adb, query->addrinfo,
					       query->udpsize);
			else
				dns_adb_timeout(fctx->adb, query->addrinfo);

			/*
			 * We don't have an RTT for this query.  Maybe the
			 * packet was lost, or maybe this server is very
			 * slow.  We don't know.  Increase the RTT.
			 */
			INSIST(no_response);
			isc_random_get(&value);
			if (query->addrinfo->srtt > 800000)
				mask = 0x3fff;
			else if (query->addrinfo->srtt > 400000)
				mask = 0x7fff;
			else if (query->addrinfo->srtt > 200000)
				mask = 0xffff;
			else if (query->addrinfo->srtt > 100000)
				mask = 0x1ffff;
			else if (query->addrinfo->srtt > 50000)
				mask = 0x3ffff;
			else if (query->addrinfo->srtt > 25000)
				mask = 0x7ffff;
			else
				mask = 0xfffff;

			/*
			 * Don't adjust timeout on EDNS queries unless we have
			 * seen a EDNS response.
			 */
			if ((query->options & DNS_FETCHOPT_NOEDNS0) == 0 &&
			    !EDNSOK(query->addrinfo)) {
				mask >>= 2;
			}

			rtt = query->addrinfo->srtt + (value & mask);
			if (rtt > MAX_SINGLE_QUERY_TIMEOUT_US)
				rtt = MAX_SINGLE_QUERY_TIMEOUT_US;

			/*
			 * Replace the current RTT with our value.
			 */
			factor = DNS_ADB_RTTADJREPLACE;
		}

		dns_adb_adjustsrtt(fctx->adb, query->addrinfo, rtt, factor);
	}

#ifdef ENABLE_FETCHLIMIT
	dns_adb_endudpfetch(fctx->adb, query->addrinfo);
#endif /* ENABLE_FETCHLIMIT */

	/*
	 * Age RTTs of servers not tried.
	 */
	isc_stdtime_get(&now);
	if (finish != NULL || age_untried)
		for (addrinfo = ISC_LIST_HEAD(fctx->forwaddrs);
		     addrinfo != NULL;
		     addrinfo = ISC_LIST_NEXT(addrinfo, publink))
			if (UNMARKED(addrinfo))
				dns_adb_agesrtt(fctx->adb, addrinfo, now);

	if ((finish != NULL || age_untried) && TRIEDFIND(fctx))
		for (find = ISC_LIST_HEAD(fctx->finds);
		     find != NULL;
		     find = ISC_LIST_NEXT(find, publink))
			for (addrinfo = ISC_LIST_HEAD(find->list);
			     addrinfo != NULL;
			     addrinfo = ISC_LIST_NEXT(addrinfo, publink))
				if (UNMARKED(addrinfo))
					dns_adb_agesrtt(fctx->adb, addrinfo,
							now);

	if ((finish != NULL || age_untried) && TRIEDALT(fctx)) {
		for (addrinfo = ISC_LIST_HEAD(fctx->altaddrs);
		     addrinfo != NULL;
		     addrinfo = ISC_LIST_NEXT(addrinfo, publink))
			if (UNMARKED(addrinfo))
				dns_adb_agesrtt(fctx->adb, addrinfo, now);
		for (find = ISC_LIST_HEAD(fctx->altfinds);
		     find != NULL;
		     find = ISC_LIST_NEXT(find, publink))
			for (addrinfo = ISC_LIST_HEAD(find->list);
			     addrinfo != NULL;
			     addrinfo = ISC_LIST_NEXT(addrinfo, publink))
				if (UNMARKED(addrinfo))
					dns_adb_agesrtt(fctx->adb, addrinfo,
							now);
	}

	/*
	 * Check for any outstanding socket events.  If they exist, cancel
	 * them and let the event handlers finish the cleanup.  The resolver
	 * only needs to worry about managing the connect and send events;
	 * the dispatcher manages the recv events.
	 */
	if (RESQUERY_CONNECTING(query)) {
		/*
		 * Cancel the connect.
		 */
		if (query->tcpsocket != NULL) {
			isc_socket_cancel(query->tcpsocket, NULL,
					  ISC_SOCKCANCEL_CONNECT);
		} else if (query->dispentry != NULL) {
			INSIST(query->exclusivesocket);
			sock = dns_dispatch_getentrysocket(query->dispentry);
			if (sock != NULL)
				isc_socket_cancel(sock, NULL,
						  ISC_SOCKCANCEL_CONNECT);
		}
	} else if (RESQUERY_SENDING(query)) {
		/*
		 * Cancel the pending send.
		 */
		if (query->exclusivesocket && query->dispentry != NULL)
			sock = dns_dispatch_getentrysocket(query->dispentry);
		else
			sock = dns_dispatch_getsocket(query->dispatch);
		if (sock != NULL)
			isc_socket_cancel(sock, NULL, ISC_SOCKCANCEL_SEND);
	}

	if (query->dispentry != NULL)
		dns_dispatch_removeresponse(&query->dispentry, deventp);

	ISC_LIST_UNLINK(fctx->queries, query, link);

	if (query->tsig != NULL)
		isc_buffer_free(&query->tsig);

	if (query->tsigkey != NULL)
		dns_tsigkey_detach(&query->tsigkey);

	if (query->dispatch != NULL)
		dns_dispatch_detach(&query->dispatch);

	if (! (RESQUERY_CONNECTING(query) || RESQUERY_SENDING(query)))
		/*
		 * It's safe to destroy the query now.
		 */
		resquery_destroy(&query);
}

static void
fctx_cancelqueries(fetchctx_t *fctx, isc_boolean_t no_response,
		   isc_boolean_t age_untried)
{
	resquery_t *query, *next_query;

	FCTXTRACE("cancelqueries");

	for (query = ISC_LIST_HEAD(fctx->queries);
	     query != NULL;
	     query = next_query) {
		next_query = ISC_LIST_NEXT(query, link);
		fctx_cancelquery(&query, NULL, NULL, no_response,
				 age_untried);
	}
}

static void
fctx_cleanupfinds(fetchctx_t *fctx) {
	dns_adbfind_t *find, *next_find;

	REQUIRE(ISC_LIST_EMPTY(fctx->queries));

	for (find = ISC_LIST_HEAD(fctx->finds);
	     find != NULL;
	     find = next_find)
	{
		next_find = ISC_LIST_NEXT(find, publink);
		ISC_LIST_UNLINK(fctx->finds, find, publink);
		dns_adb_destroyfind(&find);
	}
	fctx->find = NULL;
}

static void
fctx_cleanupaltfinds(fetchctx_t *fctx) {
	dns_adbfind_t *find, *next_find;

	REQUIRE(ISC_LIST_EMPTY(fctx->queries));

	for (find = ISC_LIST_HEAD(fctx->altfinds);
	     find != NULL;
	     find = next_find)
	{
		next_find = ISC_LIST_NEXT(find, publink);
		ISC_LIST_UNLINK(fctx->altfinds, find, publink);
		dns_adb_destroyfind(&find);
	}
	fctx->altfind = NULL;
}

static void
fctx_cleanupforwaddrs(fetchctx_t *fctx) {
	dns_adbaddrinfo_t *addr, *next_addr;

	REQUIRE(ISC_LIST_EMPTY(fctx->queries));

	for (addr = ISC_LIST_HEAD(fctx->forwaddrs);
	     addr != NULL;
	     addr = next_addr)
	{
		next_addr = ISC_LIST_NEXT(addr, publink);
		ISC_LIST_UNLINK(fctx->forwaddrs, addr, publink);
		dns_adb_freeaddrinfo(fctx->adb, &addr);
	}
}

static void
fctx_cleanupaltaddrs(fetchctx_t *fctx) {
	dns_adbaddrinfo_t *addr, *next_addr;

	REQUIRE(ISC_LIST_EMPTY(fctx->queries));

	for (addr = ISC_LIST_HEAD(fctx->altaddrs);
	     addr != NULL;
	     addr = next_addr)
	{
		next_addr = ISC_LIST_NEXT(addr, publink);
		ISC_LIST_UNLINK(fctx->altaddrs, addr, publink);
		dns_adb_freeaddrinfo(fctx->adb, &addr);
	}
}

static inline void
fctx_stopqueries(fetchctx_t *fctx, isc_boolean_t no_response,
		 isc_boolean_t age_untried)
{
	FCTXTRACE("stopqueries");
	fctx_cancelqueries(fctx, no_response, age_untried);
	fctx_stoptimer(fctx);
}

static inline void
fctx_cleanupall(fetchctx_t *fctx) {
	fctx_cleanupfinds(fctx);
	fctx_cleanupaltfinds(fctx);
	fctx_cleanupforwaddrs(fctx);
	fctx_cleanupaltaddrs(fctx);
}

#ifdef ENABLE_FETCHLIMIT
static void
fcount_logspill(fetchctx_t *fctx, fctxcount_t *counter) {
	char dbuf[DNS_NAME_FORMATSIZE];
	isc_stdtime_t now;

	if (! isc_log_wouldlog(dns_lctx, ISC_LOG_INFO))
		return;

	isc_stdtime_get(&now);
	if (counter->logged > now - 60)
		return;

	dns_name_format(&fctx->domain, dbuf, sizeof(dbuf));

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_SPILL,
		      DNS_LOGMODULE_RESOLVER, ISC_LOG_INFO,
		      "too many simultaneous fetches for %s "
		      "(allowed %d spilled %d)",
		      dbuf, counter->allowed, counter->dropped);

	counter->logged = now;
}

static isc_result_t
fcount_incr(fetchctx_t *fctx, isc_boolean_t force) {
	isc_result_t result = ISC_R_SUCCESS;
	zonebucket_t *dbucket;
	fctxcount_t *counter;
	unsigned int bucketnum, spill;

	REQUIRE(fctx != NULL);
	REQUIRE(fctx->res != NULL);

	INSIST(fctx->dbucketnum == RES_NOBUCKET);
	bucketnum = dns_name_fullhash(&fctx->domain, ISC_FALSE)
			% RES_DOMAIN_BUCKETS;

	LOCK(&fctx->res->lock);
	spill = fctx->res->zspill;
	UNLOCK(&fctx->res->lock);

	dbucket = &fctx->res->dbuckets[bucketnum];

	LOCK(&dbucket->lock);
	for (counter = ISC_LIST_HEAD(dbucket->list);
	     counter != NULL;
	     counter = ISC_LIST_NEXT(counter, link))
	{
		if (dns_name_equal(counter->domain, &fctx->domain))
			break;
	}

	if (counter == NULL) {
		counter = isc_mem_get(dbucket->mctx, sizeof(fctxcount_t));
		if (counter == NULL)
			result = ISC_R_NOMEMORY;
		else {
			ISC_LINK_INIT(counter, link);
			counter->count = 1;
			counter->logged = 0;
			counter->allowed = 1;
			counter->dropped = 0;
			dns_fixedname_init(&counter->fdname);
			counter->domain = dns_fixedname_name(&counter->fdname);
			dns_name_copy(&fctx->domain, counter->domain, NULL);
			ISC_LIST_APPEND(dbucket->list, counter, link);
		}
	} else {
		if (!force && spill != 0 && counter->count >= spill) {
			counter->dropped++;
			fcount_logspill(fctx, counter);
			result = ISC_R_QUOTA;
		} else {
			counter->count++;
			counter->allowed++;
		}
	}
	UNLOCK(&dbucket->lock);

	if (result == ISC_R_SUCCESS)
		fctx->dbucketnum = bucketnum;

	return (result);
}

static void
fcount_decr(fetchctx_t *fctx) {
	zonebucket_t *dbucket;
	fctxcount_t *counter;

	REQUIRE(fctx != NULL);

	if (fctx->dbucketnum == RES_NOBUCKET)
		return;

	dbucket = &fctx->res->dbuckets[fctx->dbucketnum];

	LOCK(&dbucket->lock);
	for (counter = ISC_LIST_HEAD(dbucket->list);
	     counter != NULL;
	     counter = ISC_LIST_NEXT(counter, link))
	{
		if (dns_name_equal(counter->domain, &fctx->domain))
			break;
	}

	if (counter != NULL) {
		INSIST(counter->count != 0);
		counter->count--;
		fctx->dbucketnum = RES_NOBUCKET;

		if (counter->count == 0) {
			ISC_LIST_UNLINK(dbucket->list, counter, link);
			isc_mem_put(dbucket->mctx, counter, sizeof(*counter));
		}
	}

	UNLOCK(&dbucket->lock);
}
#endif /* ENABLE_FETCHLIMIT */

static inline void
fctx_sendevents(fetchctx_t *fctx, isc_result_t result, int line) {
	dns_fetchevent_t *event, *next_event;
	isc_task_t *task;
	unsigned int count = 0;
	isc_interval_t i;
	isc_boolean_t logit = ISC_FALSE;
	isc_time_t now;
	unsigned int old_spillat;
	unsigned int new_spillat = 0;	/* initialized to silence
					   compiler warnings */

	/*
	 * Caller must be holding the appropriate bucket lock.
	 */
	REQUIRE(fctx->state == fetchstate_done);

	FCTXTRACE("sendevents");

	/*
	 * Keep some record of fetch result for logging later (if required).
	 */
	fctx->result = result;
	fctx->exitline = line;
	TIME_NOW(&now);
	fctx->duration = isc_time_microdiff(&now, &fctx->start);

	for (event = ISC_LIST_HEAD(fctx->events);
	     event != NULL;
	     event = next_event) {
		next_event = ISC_LIST_NEXT(event, ev_link);
		ISC_LIST_UNLINK(fctx->events, event, ev_link);
		task = event->ev_sender;
		event->ev_sender = fctx;
		event->vresult = fctx->vresult;
		if (!HAVE_ANSWER(fctx))
			event->result = result;

		INSIST(event->result != ISC_R_SUCCESS ||
		       dns_rdataset_isassociated(event->rdataset) ||
		       fctx->type == dns_rdatatype_any ||
		       fctx->type == dns_rdatatype_rrsig ||
		       fctx->type == dns_rdatatype_sig);

		/*
		 * Negative results must be indicated in event->result.
		 */
		if (dns_rdataset_isassociated(event->rdataset) &&
		    NEGATIVE(event->rdataset)) {
			INSIST(event->result == DNS_R_NCACHENXDOMAIN ||
			       event->result == DNS_R_NCACHENXRRSET);
		}

		isc_task_sendanddetach(&task, ISC_EVENT_PTR(&event));
		count++;
	}

	if ((fctx->attributes & FCTX_ATTR_HAVEANSWER) != 0 &&
	    fctx->spilled &&
	    (count < fctx->res->spillatmax || fctx->res->spillatmax == 0)) {
		LOCK(&fctx->res->lock);
		if (count == fctx->res->spillat && !fctx->res->exiting) {
			old_spillat = fctx->res->spillat;
			fctx->res->spillat += 5;
			if (fctx->res->spillat > fctx->res->spillatmax &&
			    fctx->res->spillatmax != 0)
				fctx->res->spillat = fctx->res->spillatmax;
			new_spillat = fctx->res->spillat;
			if (new_spillat != old_spillat) {
				logit = ISC_TRUE;
			}
			isc_interval_set(&i, 20 * 60, 0);
			result = isc_timer_reset(fctx->res->spillattimer,
						 isc_timertype_ticker, NULL,
						 &i, ISC_TRUE);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
		}
		UNLOCK(&fctx->res->lock);
		if (logit)
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
				      DNS_LOGMODULE_RESOLVER, ISC_LOG_NOTICE,
				      "clients-per-query increased to %u",
				      new_spillat);
	}
}

static inline void
log_edns(fetchctx_t *fctx) {
	char domainbuf[DNS_NAME_FORMATSIZE];

	if (fctx->reason == NULL)
		return;

	/*
	 * We do not know if fctx->domain is the actual domain the record
	 * lives in or a parent domain so we have a '?' after it.
	 */
	dns_name_format(&fctx->domain, domainbuf, sizeof(domainbuf));
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_EDNS_DISABLED,
		      DNS_LOGMODULE_RESOLVER, ISC_LOG_INFO,
		      "success resolving '%s' (in '%s'?) after %s",
		      fctx->info, domainbuf, fctx->reason);

	fctx->reason = NULL;
}

static void
fctx_done(fetchctx_t *fctx, isc_result_t result, int line) {
	dns_resolver_t *res;
	isc_boolean_t no_response = ISC_FALSE;
	isc_boolean_t age_untried = ISC_FALSE;

	REQUIRE(line >= 0);

	FCTXTRACE("done");

	res = fctx->res;

	if (result == ISC_R_SUCCESS) {
		/*%
		 * Log any deferred EDNS timeout messages.
		 */
		log_edns(fctx);
		no_response = ISC_TRUE;
	} else if (result == ISC_R_TIMEDOUT)
		age_untried = ISC_TRUE;

	fctx->reason = NULL;

	fctx_stopqueries(fctx, no_response, age_untried);

	LOCK(&res->buckets[fctx->bucketnum].lock);

	fctx->state = fetchstate_done;
	fctx->attributes &= ~FCTX_ATTR_ADDRWAIT;
	fctx_sendevents(fctx, result, line);

	UNLOCK(&res->buckets[fctx->bucketnum].lock);
}

static void
process_sendevent(resquery_t *query, isc_event_t *event) {
	isc_socketevent_t *sevent = (isc_socketevent_t *)event;
	isc_boolean_t destroy_query = ISC_FALSE;
	isc_boolean_t retry = ISC_FALSE;
	isc_result_t result;
	fetchctx_t *fctx;

	fctx = query->fctx;

	if (RESQUERY_CANCELED(query)) {
		if (query->sends == 0 && query->connects == 0) {
			/*
			 * This query was canceled while the
			 * isc_socket_sendto/connect() was in progress.
			 */
			if (query->tcpsocket != NULL)
				isc_socket_detach(&query->tcpsocket);
			destroy_query = ISC_TRUE;
		}
	} else {
		switch (sevent->result) {
		case ISC_R_SUCCESS:
			break;

		case ISC_R_HOSTUNREACH:
		case ISC_R_NETUNREACH:
		case ISC_R_NOPERM:
		case ISC_R_ADDRNOTAVAIL:
		case ISC_R_CONNREFUSED:
			FCTXTRACE3("query canceled in sendevent(): "
				   "no route to host; no response",
				   sevent->result);

			/*
			 * No route to remote.
			 */
			add_bad(fctx, query->addrinfo, sevent->result,
				badns_unreachable);
			fctx_cancelquery(&query, NULL, NULL, ISC_TRUE,
					 ISC_FALSE);
			retry = ISC_TRUE;
			break;

		default:
			FCTXTRACE3("query canceled in sendevent() due to "
				   "unexpected event result; responding",
				   sevent->result);

			fctx_cancelquery(&query, NULL, NULL, ISC_FALSE,
					 ISC_FALSE);
			break;
		}
	}

	if (event->ev_type == ISC_SOCKEVENT_CONNECT)
		isc_event_free(&event);

	if (retry) {
		/*
		 * Behave as if the idle timer has expired.  For TCP
		 * this may not actually reflect the latest timer.
		 */
		fctx->attributes &= ~FCTX_ATTR_ADDRWAIT;
		result = fctx_stopidletimer(fctx);
		if (result != ISC_R_SUCCESS)
			fctx_done(fctx, result, __LINE__);
		else
			fctx_try(fctx, ISC_TRUE, ISC_FALSE);
	}

	if (destroy_query)
		resquery_destroy(&query);
}

static void
resquery_udpconnected(isc_task_t *task, isc_event_t *event) {
	resquery_t *query = event->ev_arg;

	REQUIRE(event->ev_type == ISC_SOCKEVENT_CONNECT);

	QTRACE("udpconnected");

	UNUSED(task);

	INSIST(RESQUERY_CONNECTING(query));

	query->connects--;

	process_sendevent(query, event);
}

static void
resquery_senddone(isc_task_t *task, isc_event_t *event) {
	resquery_t *query = event->ev_arg;

	REQUIRE(event->ev_type == ISC_SOCKEVENT_SENDDONE);

	QTRACE("senddone");

	/*
	 * XXXRTH
	 *
	 * Currently we don't wait for the senddone event before retrying
	 * a query.  This means that if we get really behind, we may end
	 * up doing extra work!
	 */

	UNUSED(task);

	INSIST(RESQUERY_SENDING(query));

	query->sends--;

	process_sendevent(query, event);
}

static inline isc_result_t
fctx_addopt(dns_message_t *message, unsigned int version,
	    isc_uint16_t udpsize, dns_ednsopt_t *ednsopts, size_t count)
{
	dns_rdataset_t *rdataset = NULL;
	isc_result_t result;

	result = dns_message_buildopt(message, &rdataset, version, udpsize,
				      DNS_MESSAGEEXTFLAG_DO, ednsopts, count);
	if (result != ISC_R_SUCCESS)
		return (result);
	return (dns_message_setopt(message, rdataset));
}

static inline void
fctx_setretryinterval(fetchctx_t *fctx, unsigned int rtt) {
	unsigned int seconds;
	unsigned int us;

	/*
	 * We retry every .8 seconds the first two times through the address
	 * list, and then we do exponential back-off.
	 */
	if (fctx->restarts < 3)
		us = 800000;
	else
		us = (800000 << (fctx->restarts - 2));

	/*
	 * Add a fudge factor to the expected rtt based on the current
	 * estimate.
	 */
	if (rtt < 50000)
		rtt += 50000;
	else if (rtt < 100000)
		rtt += 100000;
	else
		rtt += 200000;

	/*
	 * Always wait for at least the expected rtt.
	 */
	if (us < rtt)
		us = rtt;

	/*
	 * But don't ever wait for more than 10 seconds.
	 */
	if (us > MAX_SINGLE_QUERY_TIMEOUT_US)
		us = MAX_SINGLE_QUERY_TIMEOUT_US;

	seconds = us / US_PER_SEC;
	us -= seconds * US_PER_SEC;
	isc_interval_set(&fctx->interval, seconds, us * 1000);
}

static isc_result_t
fctx_query(fetchctx_t *fctx, dns_adbaddrinfo_t *addrinfo,
	   unsigned int options)
{
	dns_resolver_t *res;
	isc_task_t *task;
	isc_result_t result;
	resquery_t *query;
	isc_sockaddr_t addr;
	isc_boolean_t have_addr = ISC_FALSE;
	unsigned int srtt;
	isc_dscp_t dscp = -1;

	FCTXTRACE("query");

	res = fctx->res;
	task = res->buckets[fctx->bucketnum].task;

	srtt = addrinfo->srtt;

	/*
	 * A forwarder needs to make multiple queries. Give it at least
	 * a second to do these in.
	 */
	if (ISFORWARDER(addrinfo) && srtt < 1000000)
		srtt = 1000000;

	fctx_setretryinterval(fctx, srtt);
	result = fctx_startidletimer(fctx, &fctx->interval);
	if (result != ISC_R_SUCCESS)
		return (result);

	INSIST(ISC_LIST_EMPTY(fctx->validators));

	dns_message_reset(fctx->rmessage, DNS_MESSAGE_INTENTPARSE);

	query = isc_mem_get(fctx->mctx, sizeof(*query));
	if (query == NULL) {
		result = ISC_R_NOMEMORY;
		goto stop_idle_timer;
	}
	query->mctx = fctx->mctx;
	query->options = options;
	query->attributes = 0;
	query->sends = 0;
	query->connects = 0;
	query->dscp = addrinfo->dscp;
	query->udpsize = 0;
	/*
	 * Note that the caller MUST guarantee that 'addrinfo' will remain
	 * valid until this query is canceled.
	 */
	query->addrinfo = addrinfo;
	TIME_NOW(&query->start);

	/*
	 * If this is a TCP query, then we need to make a socket and
	 * a dispatch for it here.  Otherwise we use the resolver's
	 * shared dispatch.
	 */
	query->dispatchmgr = res->dispatchmgr;
	query->dispatch = NULL;
	query->exclusivesocket = ISC_FALSE;
	query->tcpsocket = NULL;
	if (res->view->peers != NULL) {
		dns_peer_t *peer = NULL;
		isc_netaddr_t dstip;
		isc_boolean_t usetcp = ISC_FALSE;
		isc_netaddr_fromsockaddr(&dstip, &addrinfo->sockaddr);
		result = dns_peerlist_peerbyaddr(res->view->peers,
						 &dstip, &peer);
		if (result == ISC_R_SUCCESS) {
			result = dns_peer_getquerysource(peer, &addr);
			if (result == ISC_R_SUCCESS)
				have_addr = ISC_TRUE;
			result = dns_peer_getquerydscp(peer, &dscp);
			if (result == ISC_R_SUCCESS)
				query->dscp = dscp;
			result = dns_peer_getforcetcp(peer, &usetcp);
			if (result == ISC_R_SUCCESS && usetcp)
				query->options |= DNS_FETCHOPT_TCP;
		}
	}

	dscp = -1;
	if ((query->options & DNS_FETCHOPT_TCP) != 0) {
		int pf;

		pf = isc_sockaddr_pf(&addrinfo->sockaddr);
		if (!have_addr) {
			switch (pf) {
			case PF_INET:
				result = dns_dispatch_getlocaladdress(
					      res->dispatches4->dispatches[0],
					      &addr);
				dscp = dns_resolver_getquerydscp4(fctx->res);
				break;
			case PF_INET6:
				result = dns_dispatch_getlocaladdress(
					      res->dispatches6->dispatches[0],
					      &addr);
				dscp = dns_resolver_getquerydscp6(fctx->res);
				break;
			default:
				result = ISC_R_NOTIMPLEMENTED;
				break;
			}
			if (result != ISC_R_SUCCESS)
				goto cleanup_query;
		}
		isc_sockaddr_setport(&addr, 0);
		if (query->dscp == -1)
			query->dscp = dscp;

		result = isc_socket_create(res->socketmgr, pf,
					   isc_sockettype_tcp,
					   &query->tcpsocket);
		if (result != ISC_R_SUCCESS)
			goto cleanup_query;

#ifndef BROKEN_TCP_BIND_BEFORE_CONNECT
		result = isc_socket_bind(query->tcpsocket, &addr, 0);
		if (result != ISC_R_SUCCESS)
			goto cleanup_socket;
#endif
		/*
		 * A dispatch will be created once the connect succeeds.
		 */
	} else {
		if (have_addr) {
			unsigned int attrs, attrmask;
			attrs = DNS_DISPATCHATTR_UDP;
			switch (isc_sockaddr_pf(&addr)) {
			case AF_INET:
				attrs |= DNS_DISPATCHATTR_IPV4;
				dscp = dns_resolver_getquerydscp4(fctx->res);
				break;
			case AF_INET6:
				attrs |= DNS_DISPATCHATTR_IPV6;
				dscp = dns_resolver_getquerydscp6(fctx->res);
				break;
			default:
				result = ISC_R_NOTIMPLEMENTED;
				goto cleanup_query;
			}
			attrmask = DNS_DISPATCHATTR_UDP;
			attrmask |= DNS_DISPATCHATTR_TCP;
			attrmask |= DNS_DISPATCHATTR_IPV4;
			attrmask |= DNS_DISPATCHATTR_IPV6;
			result = dns_dispatch_getudp(res->dispatchmgr,
						     res->socketmgr,
						     res->taskmgr, &addr,
						     4096, 20000, 32768, 16411,
						     16433, attrs, attrmask,
						     &query->dispatch);
			if (result != ISC_R_SUCCESS)
				goto cleanup_query;
		} else {
			switch (isc_sockaddr_pf(&addrinfo->sockaddr)) {
			case PF_INET:
				dns_dispatch_attach(
				    dns_resolver_dispatchv4(res),
				    &query->dispatch);
				query->exclusivesocket = res->exclusivev4;
				dscp = dns_resolver_getquerydscp4(fctx->res);
				break;
			case PF_INET6:
				dns_dispatch_attach(
				    dns_resolver_dispatchv6(res),
				    &query->dispatch);
				query->exclusivesocket = res->exclusivev6;
				dscp = dns_resolver_getquerydscp6(fctx->res);
				break;
			default:
				result = ISC_R_NOTIMPLEMENTED;
				goto cleanup_query;
			}
		}

		if (query->dscp == -1)
			query->dscp = dscp;
		/*
		 * We should always have a valid dispatcher here.  If we
		 * don't support a protocol family, then its dispatcher
		 * will be NULL, but we shouldn't be finding addresses for
		 * protocol types we don't support, so the dispatcher
		 * we found should never be NULL.
		 */
		INSIST(query->dispatch != NULL);
	}

	query->dispentry = NULL;
	query->fctx = fctx;	/* reference added by caller */
	query->tsig = NULL;
	query->tsigkey = NULL;
	ISC_LINK_INIT(query, link);
	query->magic = QUERY_MAGIC;

	if ((query->options & DNS_FETCHOPT_TCP) != 0) {
		/*
		 * Connect to the remote server.
		 *
		 * XXXRTH  Should we attach to the socket?
		 */
		if (query->dscp != -1)
			isc_socket_dscp(query->tcpsocket, query->dscp);
		result = isc_socket_connect(query->tcpsocket,
					    &addrinfo->sockaddr, task,
					    resquery_connected, query);
		if (result != ISC_R_SUCCESS)
			goto cleanup_socket;
		query->connects++;
		QTRACE("connecting via TCP");
	} else {
#ifdef ENABLE_FETCHLIMIT
		if (dns_adbentry_overquota(addrinfo->entry))
			goto cleanup_dispatch;

		/* Inform the ADB that we're starting a fetch */
		dns_adb_beginudpfetch(fctx->adb, addrinfo);
#endif /* ENABLE_FETCHLIMIT */

		result = resquery_send(query);
		if (result != ISC_R_SUCCESS)
			goto cleanup_dispatch;
	}

	fctx->querysent++;

	ISC_LIST_APPEND(fctx->queries, query, link);
	query->fctx->nqueries++;
	if (isc_sockaddr_pf(&addrinfo->sockaddr) == PF_INET)
		inc_stats(res, dns_resstatscounter_queryv4);
	else
		inc_stats(res, dns_resstatscounter_queryv6);
	if (res->view->resquerystats != NULL)
		dns_rdatatypestats_increment(res->view->resquerystats,
					     fctx->type);

	return (ISC_R_SUCCESS);

 cleanup_socket:
	isc_socket_detach(&query->tcpsocket);

 cleanup_dispatch:
	if (query->dispatch != NULL)
		dns_dispatch_detach(&query->dispatch);

 cleanup_query:
	if (query->connects == 0) {
		query->magic = 0;
		isc_mem_put(fctx->mctx, query, sizeof(*query));
	}

 stop_idle_timer:
	RUNTIME_CHECK(fctx_stopidletimer(fctx) == ISC_R_SUCCESS);

	return (result);
}

static isc_boolean_t
bad_edns(fetchctx_t *fctx, isc_sockaddr_t *address) {
	isc_sockaddr_t *sa;

	for (sa = ISC_LIST_HEAD(fctx->bad_edns);
	     sa != NULL;
	     sa = ISC_LIST_NEXT(sa, link)) {
		if (isc_sockaddr_equal(sa, address))
			return (ISC_TRUE);
	}

	return (ISC_FALSE);
}

static void
add_bad_edns(fetchctx_t *fctx, isc_sockaddr_t *address) {
	isc_sockaddr_t *sa;

	if (bad_edns(fctx, address))
		return;

	sa = isc_mem_get(fctx->mctx, sizeof(*sa));
	if (sa == NULL)
		return;

	*sa = *address;
	ISC_LIST_INITANDAPPEND(fctx->bad_edns, sa, link);
}

static struct tried *
triededns(fetchctx_t *fctx, isc_sockaddr_t *address) {
	struct tried *tried;

	for (tried = ISC_LIST_HEAD(fctx->edns);
	     tried != NULL;
	     tried = ISC_LIST_NEXT(tried, link)) {
		if (isc_sockaddr_equal(&tried->addr, address))
			return (tried);
	}

	return (NULL);
}

static void
add_triededns(fetchctx_t *fctx, isc_sockaddr_t *address) {
	struct tried *tried;

	tried = triededns(fctx, address);
	if (tried != NULL) {
		tried->count++;
		return;
	}

	tried = isc_mem_get(fctx->mctx, sizeof(*tried));
	if (tried == NULL)
		return;

	tried->addr = *address;
	tried->count = 1;
	ISC_LIST_INITANDAPPEND(fctx->edns, tried, link);
}

static struct tried *
triededns512(fetchctx_t *fctx, isc_sockaddr_t *address) {
	struct tried *tried;

	for (tried = ISC_LIST_HEAD(fctx->edns512);
	     tried != NULL;
	     tried = ISC_LIST_NEXT(tried, link)) {
		if (isc_sockaddr_equal(&tried->addr, address))
			return (tried);
	}

	return (NULL);
}

static void
add_triededns512(fetchctx_t *fctx, isc_sockaddr_t *address) {
	struct tried *tried;

	tried = triededns512(fctx, address);
	if (tried != NULL) {
		tried->count++;
		return;
	}

	tried = isc_mem_get(fctx->mctx, sizeof(*tried));
	if (tried == NULL)
		return;

	tried->addr = *address;
	tried->count = 1;
	ISC_LIST_INITANDAPPEND(fctx->edns512, tried, link);
}

#ifdef ISC_PLATFORM_USESIT
static void
compute_cc(resquery_t *query, unsigned char *cookie, size_t len) {
#ifdef AES_SIT
	unsigned char digest[ISC_AES_BLOCK_LENGTH];
	unsigned char input[16];
	isc_netaddr_t netaddr;
	unsigned int i;

	INSIST(len >= 8U);

	isc_netaddr_fromsockaddr(&netaddr, &query->addrinfo->sockaddr);
	switch (netaddr.family) {
	case AF_INET:
		memmove(input, (unsigned char *)&netaddr.type.in, 4);
		memset(input + 4, 0, 12);
		break;
	case AF_INET6:
		memmove(input, (unsigned char *)&netaddr.type.in6, 16);
		break;
	}
	isc_aes128_crypt(query->fctx->res->view->secret, input, digest);
	for (i = 0; i < 8; i++)
		digest[i] ^= digest[i + 8];
	memmove(cookie, digest, 8);
#endif
#ifdef HMAC_SHA1_SIT
	unsigned char digest[ISC_SHA1_DIGESTLENGTH];
	isc_netaddr_t netaddr;
	isc_hmacsha1_t hmacsha1;

	INSIST(len >= 8U);

	isc_hmacsha1_init(&hmacsha1, query->fctx->res->view->secret,
			  ISC_SHA1_DIGESTLENGTH);
	isc_netaddr_fromsockaddr(&netaddr, &query->addrinfo->sockaddr);
	switch (netaddr.family) {
	case AF_INET:
		isc_hmacsha1_update(&hmacsha1,
				    (unsigned char *)&netaddr.type.in, 4);
		break;
	case AF_INET6:
		isc_hmacsha1_update(&hmacsha1,
				    (unsigned char *)&netaddr.type.in6, 16);
		break;
	}
	isc_hmacsha1_sign(&hmacsha1, digest, sizeof(digest));
	memmove(cookie, digest, 8);
	isc_hmacsha1_invalidate(&hmacsha1);
#endif
#ifdef HMAC_SHA256_SIT
	unsigned char digest[ISC_SHA256_DIGESTLENGTH];
	isc_netaddr_t netaddr;
	isc_hmacsha256_t hmacsha256;

	INSIST(len >= 8U);

	isc_hmacsha256_init(&hmacsha256, query->fctx->res->view->secret,
			    ISC_SHA256_DIGESTLENGTH);
	isc_netaddr_fromsockaddr(&netaddr, &query->addrinfo->sockaddr);
	switch (netaddr.family) {
	case AF_INET:
		isc_hmacsha256_update(&hmacsha256,
				      (unsigned char *)&netaddr.type.in, 4);
		break;
	case AF_INET6:
		isc_hmacsha256_update(&hmacsha256,
				      (unsigned char *)&netaddr.type.in6, 16);
		break;
	}
	isc_hmacsha256_sign(&hmacsha256, digest, sizeof(digest));
	memmove(cookie, digest, 8);
	isc_hmacsha256_invalidate(&hmacsha256);
#endif
}
#endif

static isc_boolean_t
wouldvalidate(fetchctx_t *fctx) {
	isc_boolean_t secure_domain;
	isc_result_t result;

	if (!fctx->res->view->enablevalidation)
		return (ISC_FALSE);

	if (fctx->res->view->dlv != NULL)
		return (ISC_TRUE);

	result = dns_view_issecuredomain(fctx->res->view, &fctx->name,
					 &secure_domain);
	if (result != ISC_R_SUCCESS)
		return (ISC_FALSE);
	return (secure_domain);
}

static isc_result_t
resquery_send(resquery_t *query) {
	fetchctx_t *fctx;
	isc_result_t result;
	dns_name_t *qname = NULL;
	dns_rdataset_t *qrdataset = NULL;
	isc_region_t r;
	dns_resolver_t *res;
	isc_task_t *task;
	isc_socket_t *sock;
	isc_buffer_t tcpbuffer;
	isc_sockaddr_t *address;
	isc_buffer_t *buffer;
	isc_netaddr_t ipaddr;
	dns_tsigkey_t *tsigkey = NULL;
	dns_peer_t *peer = NULL;
	isc_boolean_t useedns;
	dns_compress_t cctx;
	isc_boolean_t cleanup_cctx = ISC_FALSE;
	isc_boolean_t secure_domain;
	isc_boolean_t connecting = ISC_FALSE;
	isc_boolean_t tcp = ISC_TF((query->options & DNS_FETCHOPT_TCP) != 0);
	dns_ednsopt_t ednsopts[DNS_EDNSOPTIONS];
	unsigned ednsopt = 0;
	isc_uint16_t hint = 0, udpsize = 0;	/* No EDNS */

	fctx = query->fctx;
	QTRACE("send");

	res = fctx->res;
	task = res->buckets[fctx->bucketnum].task;
	address = NULL;

	if (tcp) {
		/*
		 * Reserve space for the TCP message length.
		 */
		isc_buffer_init(&tcpbuffer, query->data, sizeof(query->data));
		isc_buffer_init(&query->buffer, query->data + 2,
				sizeof(query->data) - 2);
		buffer = &tcpbuffer;
	} else {
		isc_buffer_init(&query->buffer, query->data,
				sizeof(query->data));
		buffer = &query->buffer;
	}

	result = dns_message_gettempname(fctx->qmessage, &qname);
	if (result != ISC_R_SUCCESS)
		goto cleanup_temps;
	result = dns_message_gettemprdataset(fctx->qmessage, &qrdataset);
	if (result != ISC_R_SUCCESS)
		goto cleanup_temps;

	/*
	 * Get a query id from the dispatch.
	 */
	result = dns_dispatch_addresponse2(query->dispatch,
					   &query->addrinfo->sockaddr,
					   task,
					   resquery_response,
					   query,
					   &query->id,
					   &query->dispentry,
					   res->socketmgr);
	if (result != ISC_R_SUCCESS)
		goto cleanup_temps;

	fctx->qmessage->opcode = dns_opcode_query;

	/*
	 * Set up question.
	 */
	dns_name_init(qname, NULL);
	dns_name_clone(&fctx->name, qname);
	dns_rdataset_makequestion(qrdataset, res->rdclass, fctx->type);
	ISC_LIST_APPEND(qname->list, qrdataset, link);
	dns_message_addname(fctx->qmessage, qname, DNS_SECTION_QUESTION);
	qname = NULL;
	qrdataset = NULL;

	/*
	 * Set RD if the client has requested that we do a recursive query,
	 * or if we're sending to a forwarder.
	 */
	if ((query->options & DNS_FETCHOPT_RECURSIVE) != 0 ||
	    ISFORWARDER(query->addrinfo))
		fctx->qmessage->flags |= DNS_MESSAGEFLAG_RD;

	/*
	 * Set CD if the client says not to validate, or if the
	 * question is under a secure entry point and this is a
	 * recursive/forward query -- unless the client said not to.
	 */
	if ((query->options & DNS_FETCHOPT_NOCDFLAG) != 0)
		/* Do nothing */
		;
	else if ((query->options & DNS_FETCHOPT_NOVALIDATE) != 0)
		fctx->qmessage->flags |= DNS_MESSAGEFLAG_CD;
	else if (res->view->enablevalidation &&
		 ((fctx->qmessage->flags & DNS_MESSAGEFLAG_RD) != 0)) {
		result = dns_view_issecuredomain(res->view, &fctx->name,
						 &secure_domain);
		if (result != ISC_R_SUCCESS)
			secure_domain = ISC_FALSE;
		if (res->view->dlv != NULL)
			secure_domain = ISC_TRUE;
		if (secure_domain)
			fctx->qmessage->flags |= DNS_MESSAGEFLAG_CD;
	}

	/*
	 * We don't have to set opcode because it defaults to query.
	 */
	fctx->qmessage->id = query->id;

	/*
	 * Convert the question to wire format.
	 */
	result = dns_compress_init(&cctx, -1, fctx->res->mctx);
	if (result != ISC_R_SUCCESS)
		goto cleanup_message;
	cleanup_cctx = ISC_TRUE;

	result = dns_message_renderbegin(fctx->qmessage, &cctx,
					 &query->buffer);
	if (result != ISC_R_SUCCESS)
		goto cleanup_message;

	result = dns_message_rendersection(fctx->qmessage,
					   DNS_SECTION_QUESTION, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup_message;

	peer = NULL;
	isc_netaddr_fromsockaddr(&ipaddr, &query->addrinfo->sockaddr);
	(void) dns_peerlist_peerbyaddr(fctx->res->view->peers, &ipaddr, &peer);

	/*
	 * The ADB does not know about servers with "edns no".  Check this,
	 * and then inform the ADB for future use.
	 */
	if ((query->addrinfo->flags & DNS_FETCHOPT_NOEDNS0) == 0 &&
	    peer != NULL &&
	    dns_peer_getsupportedns(peer, &useedns) == ISC_R_SUCCESS &&
	    !useedns)
	{
		query->options |= DNS_FETCHOPT_NOEDNS0;
		dns_adb_changeflags(fctx->adb, query->addrinfo,
				    DNS_FETCHOPT_NOEDNS0,
				    DNS_FETCHOPT_NOEDNS0);
	}

	/* Sync NOEDNS0 flag in addrinfo->flags and options now. */
	if ((query->addrinfo->flags & DNS_FETCHOPT_NOEDNS0) != 0)
		query->options |= DNS_FETCHOPT_NOEDNS0;

	/* See if response history indicates that EDNS is not supported. */
	if ((query->options & DNS_FETCHOPT_NOEDNS0) == 0 &&
	    dns_adb_noedns(fctx->adb, query->addrinfo))
		query->options |= DNS_FETCHOPT_NOEDNS0;

	if (fctx->timeout && (query->options & DNS_FETCHOPT_NOEDNS0) == 0) {
		isc_sockaddr_t *sockaddr = &query->addrinfo->sockaddr;
		struct tried *tried;

		if (fctx->timeouts > (MAX_EDNS0_TIMEOUTS * 2) &&
		    (!EDNSOK(query->addrinfo) || !wouldvalidate(fctx))) {
			query->options |= DNS_FETCHOPT_NOEDNS0;
			fctx->reason = "disabling EDNS";
		} else if ((tried = triededns512(fctx, sockaddr)) != NULL &&
		    tried->count >= 2U &&
		    (!EDNSOK(query->addrinfo) || !wouldvalidate(fctx))) {
			query->options |= DNS_FETCHOPT_NOEDNS0;
			fctx->reason = "disabling EDNS";
		} else if ((tried = triededns(fctx, sockaddr)) != NULL) {
			if (tried->count == 1U) {
				hint = dns_adb_getudpsize(fctx->adb,
							  query->addrinfo);
			} else if (tried->count >= 2U) {
				query->options |= DNS_FETCHOPT_EDNS512;
				fctx->reason = "reducing the advertised EDNS "
					       "UDP packet size to 512 octets";
			}
		}
	}
	fctx->timeout = ISC_FALSE;

	/*
	 * Use EDNS0, unless the caller doesn't want it, or we know that
	 * the remote server doesn't like it.
	 */
	if ((query->options & DNS_FETCHOPT_NOEDNS0) == 0) {
		if ((query->addrinfo->flags & DNS_FETCHOPT_NOEDNS0) == 0) {
			unsigned int version = 0;       /* Default version. */
			unsigned int flags = query->addrinfo->flags;
			isc_boolean_t reqnsid = res->view->requestnsid;
#ifdef ISC_PLATFORM_USESIT
			isc_boolean_t reqsit = res->view->requestsit;
			unsigned char cookie[64];
#endif

			if ((flags & FCTX_ADDRINFO_EDNSOK) != 0 &&
			    (query->options & DNS_FETCHOPT_EDNS512) == 0) {
				udpsize = dns_adb_probesize2(fctx->adb,
							     query->addrinfo,
							     fctx->timeouts);
				if (udpsize > res->udpsize)
					udpsize = res->udpsize;
			}

			if (peer != NULL)
				(void)dns_peer_getudpsize(peer, &udpsize);

			if (udpsize == 0U && res->udpsize == 512U)
				udpsize = 512;

			/*
			 * Was the size forced to 512 in the configuration?
			 */
			if (udpsize == 512U)
			    query->options |= DNS_FETCHOPT_EDNS512;

			/*
			 * We have talked to this server before.
			 */
			if (hint != 0U)
				udpsize = hint;

			/*
			 * We know nothing about the peer's capabilities
			 * so start with minimal EDNS UDP size.
			 */
			if (udpsize == 0U)
				udpsize = 512;

			if ((flags & DNS_FETCHOPT_EDNSVERSIONSET) != 0) {
				version = flags & DNS_FETCHOPT_EDNSVERSIONMASK;
				version >>= DNS_FETCHOPT_EDNSVERSIONSHIFT;
			}

			/* Request NSID/COOKIE for current view or peer? */
			if (peer != NULL) {
				(void) dns_peer_getrequestnsid(peer, &reqnsid);
#ifdef ISC_PLATFORM_USESIT
				(void) dns_peer_getrequestsit(peer, &reqsit);
#endif
			}
#ifdef ISC_PLATFORM_USESIT
			if (NOSIT(query->addrinfo))
				reqsit = ISC_FALSE;
#endif
			if (reqnsid) {
				INSIST(ednsopt < DNS_EDNSOPTIONS);
				ednsopts[ednsopt].code = DNS_OPT_NSID;
				ednsopts[ednsopt].length = 0;
				ednsopts[ednsopt].value = NULL;
				ednsopt++;
			}
#ifdef ISC_PLATFORM_USESIT
			if (reqsit) {
				INSIST(ednsopt < DNS_EDNSOPTIONS);
				ednsopts[ednsopt].code = DNS_OPT_COOKIE;
				ednsopts[ednsopt].length = (isc_uint16_t)
					dns_adb_getsit(fctx->adb,
						       query->addrinfo,
						       cookie, sizeof(cookie));
				if (ednsopts[ednsopt].length != 0) {
					ednsopts[ednsopt].value = cookie;
					inc_stats(fctx->res,
						  dns_resstatscounter_sitout);
				} else {
					compute_cc(query, cookie, 8);
					ednsopts[ednsopt].value = cookie;
					ednsopts[ednsopt].length = 8;
					inc_stats(fctx->res,
						  dns_resstatscounter_sitcc);
				}
				ednsopt++;
			}
#endif
			query->ednsversion = version;
			result = fctx_addopt(fctx->qmessage, version,
					     udpsize, ednsopts, ednsopt);
			if (reqnsid && result == ISC_R_SUCCESS) {
				query->options |= DNS_FETCHOPT_WANTNSID;
			} else if (result != ISC_R_SUCCESS) {
				/*
				 * We couldn't add the OPT, but we'll press on.
				 * We're not using EDNS0, so set the NOEDNS0
				 * bit.
				 */
				query->options |= DNS_FETCHOPT_NOEDNS0;
				query->ednsversion = -1;
				udpsize = 0;
			}
		} else {
			/*
			 * We know this server doesn't like EDNS0, so we
			 * won't use it.  Set the NOEDNS0 bit since we're
			 * not using EDNS0.
			 */
			query->options |= DNS_FETCHOPT_NOEDNS0;
			query->ednsversion = -1;
		}
	} else
		query->ednsversion = -1;

	/*
	 * Record the UDP EDNS size choosen.
	 */
	query->udpsize = udpsize;

	/*
	 * If we need EDNS0 to do this query and aren't using it, we lose.
	 */
	if (NEEDEDNS0(fctx) && (query->options & DNS_FETCHOPT_NOEDNS0) != 0) {
		result = DNS_R_SERVFAIL;
		goto cleanup_message;
	}

	if (udpsize > 512U)
		add_triededns(fctx, &query->addrinfo->sockaddr);

	if (udpsize == 512U)
		add_triededns512(fctx, &query->addrinfo->sockaddr);

	/*
	 * Clear CD if EDNS is not in use.
	 */
	if ((query->options & DNS_FETCHOPT_NOEDNS0) != 0)
		fctx->qmessage->flags &= ~DNS_MESSAGEFLAG_CD;

	/*
	 * Add TSIG record tailored to the current recipient.
	 */
	result = dns_view_getpeertsig(fctx->res->view, &ipaddr, &tsigkey);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOTFOUND)
		goto cleanup_message;

	if (tsigkey != NULL) {
		result = dns_message_settsigkey(fctx->qmessage, tsigkey);
		dns_tsigkey_detach(&tsigkey);
		if (result != ISC_R_SUCCESS)
			goto cleanup_message;
	}

	result = dns_message_rendersection(fctx->qmessage,
					   DNS_SECTION_ADDITIONAL, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup_message;

	result = dns_message_renderend(fctx->qmessage);
	if (result != ISC_R_SUCCESS)
		goto cleanup_message;

	dns_compress_invalidate(&cctx);
	cleanup_cctx = ISC_FALSE;

	if (dns_message_gettsigkey(fctx->qmessage) != NULL) {
		dns_tsigkey_attach(dns_message_gettsigkey(fctx->qmessage),
				   &query->tsigkey);
		result = dns_message_getquerytsig(fctx->qmessage,
						  fctx->res->mctx,
						  &query->tsig);
		if (result != ISC_R_SUCCESS)
			goto cleanup_message;
	}

	/*
	 * If using TCP, write the length of the message at the beginning
	 * of the buffer.
	 */
	if (tcp) {
		isc_buffer_usedregion(&query->buffer, &r);
		isc_buffer_putuint16(&tcpbuffer, (isc_uint16_t)r.length);
		isc_buffer_add(&tcpbuffer, r.length);
	}

	/*
	 * We're now done with the query message.
	 */
	dns_message_reset(fctx->qmessage, DNS_MESSAGE_INTENTRENDER);

	if (query->exclusivesocket)
		sock = dns_dispatch_getentrysocket(query->dispentry);
	else
		sock = dns_dispatch_getsocket(query->dispatch);
	/*
	 * Send the query!
	 */
	if (!tcp) {
		address = &query->addrinfo->sockaddr;
		if (query->exclusivesocket) {
			result = isc_socket_connect(sock, address, task,
						    resquery_udpconnected,
						    query);
			if (result != ISC_R_SUCCESS)
				goto cleanup_message;
			connecting = ISC_TRUE;
			query->connects++;
		}
	}
	isc_buffer_usedregion(buffer, &r);

	/*
	 * XXXRTH  Make sure we don't send to ourselves!  We should probably
	 *		prune out these addresses when we get them from the ADB.
	 */
	memset(&query->sendevent, 0, sizeof(query->sendevent));
	ISC_EVENT_INIT(&query->sendevent, sizeof(query->sendevent), 0, NULL,
		       ISC_SOCKEVENT_SENDDONE, resquery_senddone, query,
		       NULL, NULL, NULL);

	if (query->dscp == -1) {
		query->sendevent.attributes &= ~ISC_SOCKEVENTATTR_DSCP;
		query->sendevent.dscp = 0;
	} else {
		query->sendevent.attributes |= ISC_SOCKEVENTATTR_DSCP;
		query->sendevent.dscp = query->dscp;
		if (tcp)
			isc_socket_dscp(sock, query->dscp);
	}

	result = isc_socket_sendto2(sock, &r, task, address, NULL,
				    &query->sendevent, 0);
	if (result != ISC_R_SUCCESS) {
		if (connecting) {
			/*
			 * This query is still connecting.
			 * Mark it as canceled so that it will just be
			 * cleaned up when the connected event is received.
			 * Keep fctx around until the event is processed.
			 */
			query->fctx->nqueries++;
			query->attributes |= RESQUERY_ATTR_CANCELED;
		}
		goto cleanup_message;
	}

	query->sends++;

	QTRACE("sent");

	return (ISC_R_SUCCESS);

 cleanup_message:
	if (cleanup_cctx)
		dns_compress_invalidate(&cctx);

	dns_message_reset(fctx->qmessage, DNS_MESSAGE_INTENTRENDER);

	/*
	 * Stop the dispatcher from listening.
	 */
	dns_dispatch_removeresponse(&query->dispentry, NULL);

 cleanup_temps:
	if (qname != NULL)
		dns_message_puttempname(fctx->qmessage, &qname);
	if (qrdataset != NULL)
		dns_message_puttemprdataset(fctx->qmessage, &qrdataset);

	return (result);
}

static void
resquery_connected(isc_task_t *task, isc_event_t *event) {
	isc_socketevent_t *sevent = (isc_socketevent_t *)event;
	resquery_t *query = event->ev_arg;
	isc_boolean_t retry = ISC_FALSE;
	isc_interval_t interval;
	isc_result_t result;
	unsigned int attrs;
	fetchctx_t *fctx;

	REQUIRE(event->ev_type == ISC_SOCKEVENT_CONNECT);
	REQUIRE(VALID_QUERY(query));

	QTRACE("connected");

	UNUSED(task);

	/*
	 * XXXRTH
	 *
	 * Currently we don't wait for the connect event before retrying
	 * a query.  This means that if we get really behind, we may end
	 * up doing extra work!
	 */

	query->connects--;
	fctx = query->fctx;

	if (RESQUERY_CANCELED(query)) {
		/*
		 * This query was canceled while the connect() was in
		 * progress.
		 */
		isc_socket_detach(&query->tcpsocket);
		resquery_destroy(&query);
	} else {
		switch (sevent->result) {
		case ISC_R_SUCCESS:

			/*
			 * Extend the idle timer for TCP.  20 seconds
			 * should be long enough for a TCP connection to be
			 * established, a single DNS request to be sent,
			 * and the response received.
			 */
			isc_interval_set(&interval, 20, 0);
			result = fctx_startidletimer(query->fctx, &interval);
			if (result != ISC_R_SUCCESS) {
				FCTXTRACE("query canceled: idle timer failed; "
					  "responding");

				fctx_cancelquery(&query, NULL, NULL, ISC_FALSE,
						 ISC_FALSE);
				fctx_done(fctx, result, __LINE__);
				break;
			}
			/*
			 * We are connected.  Create a dispatcher and
			 * send the query.
			 */
			attrs = 0;
			attrs |= DNS_DISPATCHATTR_TCP;
			attrs |= DNS_DISPATCHATTR_PRIVATE;
			attrs |= DNS_DISPATCHATTR_CONNECTED;
			if (isc_sockaddr_pf(&query->addrinfo->sockaddr) ==
			    AF_INET)
				attrs |= DNS_DISPATCHATTR_IPV4;
			else
				attrs |= DNS_DISPATCHATTR_IPV6;
			attrs |= DNS_DISPATCHATTR_MAKEQUERY;

			result = dns_dispatch_createtcp(query->dispatchmgr,
						     query->tcpsocket,
						     query->fctx->res->taskmgr,
						     4096, 2, 1, 1, 3, attrs,
						     &query->dispatch);

			/*
			 * Regardless of whether dns_dispatch_create()
			 * succeeded or not, we don't need our reference
			 * to the socket anymore.
			 */
			isc_socket_detach(&query->tcpsocket);

			if (result == ISC_R_SUCCESS)
				result = resquery_send(query);

			if (result != ISC_R_SUCCESS) {
				FCTXTRACE("query canceled: "
					  "resquery_send() failed; responding");

				fctx_cancelquery(&query, NULL, NULL, ISC_FALSE, ISC_FALSE);
				fctx_done(fctx, result, __LINE__);
			}
			break;

		case ISC_R_NETUNREACH:
		case ISC_R_HOSTUNREACH:
		case ISC_R_CONNREFUSED:
		case ISC_R_NOPERM:
		case ISC_R_ADDRNOTAVAIL:
		case ISC_R_CONNECTIONRESET:
			FCTXTRACE3("query canceled in connected(): "
				   "no route to host; no response",
				   sevent->result);

			/*
			 * No route to remote.
			 */
			isc_socket_detach(&query->tcpsocket);
			fctx_cancelquery(&query, NULL, NULL, ISC_TRUE, ISC_FALSE);
			retry = ISC_TRUE;
			break;

		default:
			FCTXTRACE3("query canceled in connected() due to "
				   "unexpected event result; responding",
				   sevent->result);

			isc_socket_detach(&query->tcpsocket);
			fctx_cancelquery(&query, NULL, NULL, ISC_FALSE, ISC_FALSE);
			break;
		}
	}

	isc_event_free(&event);

	if (retry) {
		/*
		 * Behave as if the idle timer has expired.  For TCP
		 * connections this may not actually reflect the latest timer.
		 */
		fctx->attributes &= ~FCTX_ATTR_ADDRWAIT;
		result = fctx_stopidletimer(fctx);
		if (result != ISC_R_SUCCESS)
			fctx_done(fctx, result, __LINE__);
		else
			fctx_try(fctx, ISC_TRUE, ISC_FALSE);
	}
}

static void
fctx_finddone(isc_task_t *task, isc_event_t *event) {
	fetchctx_t *fctx;
	dns_adbfind_t *find;
	dns_resolver_t *res;
	isc_boolean_t want_try = ISC_FALSE;
	isc_boolean_t want_done = ISC_FALSE;
	isc_boolean_t bucket_empty = ISC_FALSE;
	unsigned int bucketnum;
	isc_boolean_t dodestroy = ISC_FALSE;

	find = event->ev_sender;
	fctx = event->ev_arg;
	REQUIRE(VALID_FCTX(fctx));
	res = fctx->res;

	UNUSED(task);

	FCTXTRACE("finddone");

	bucketnum = fctx->bucketnum;
	LOCK(&res->buckets[bucketnum].lock);

	INSIST(fctx->pending > 0);
	fctx->pending--;

	if (ADDRWAIT(fctx)) {
		/*
		 * The fetch is waiting for a name to be found.
		 */
		INSIST(!SHUTTINGDOWN(fctx));
		if (event->ev_type == DNS_EVENT_ADBMOREADDRESSES) {
			fctx->attributes &= ~FCTX_ATTR_ADDRWAIT;
			want_try = ISC_TRUE;
		} else {
			fctx->findfail++;
			if (fctx->pending == 0) {
				/*
				 * We've got nothing else to wait for and don't
				 * know the answer.  There's nothing to do but
				 * fail the fctx.
				 */
				fctx->attributes &= ~FCTX_ATTR_ADDRWAIT;
				want_done = ISC_TRUE;
			}
		}
	} else if (SHUTTINGDOWN(fctx) && fctx->pending == 0 &&
		   fctx->nqueries == 0 && ISC_LIST_EMPTY(fctx->validators)) {

		if (fctx->references == 0) {
			bucket_empty = fctx_unlink(fctx);
			dodestroy = ISC_TRUE;
		}
	}
	UNLOCK(&res->buckets[bucketnum].lock);

	isc_event_free(&event);
	dns_adb_destroyfind(&find);

	if (want_try) {
		fctx_try(fctx, ISC_TRUE, ISC_FALSE);
	} else if (want_done) {
		FCTXTRACE("fetch failed in finddone(); return ISC_R_FAILURE");
		fctx_done(fctx, ISC_R_FAILURE, __LINE__);
	} else if (dodestroy) {
		fctx_destroy(fctx);
		if (bucket_empty)
			empty_bucket(res);
	}
}


static inline isc_boolean_t
bad_server(fetchctx_t *fctx, isc_sockaddr_t *address) {
	isc_sockaddr_t *sa;

	for (sa = ISC_LIST_HEAD(fctx->bad);
	     sa != NULL;
	     sa = ISC_LIST_NEXT(sa, link)) {
		if (isc_sockaddr_equal(sa, address))
			return (ISC_TRUE);
	}

	return (ISC_FALSE);
}

static inline isc_boolean_t
mark_bad(fetchctx_t *fctx) {
	dns_adbfind_t *curr;
	dns_adbaddrinfo_t *addrinfo;
	isc_boolean_t all_bad = ISC_TRUE;

	/*
	 * Mark all known bad servers, so we don't try to talk to them
	 * again.
	 */

	/*
	 * Mark any bad nameservers.
	 */
	for (curr = ISC_LIST_HEAD(fctx->finds);
	     curr != NULL;
	     curr = ISC_LIST_NEXT(curr, publink)) {
		for (addrinfo = ISC_LIST_HEAD(curr->list);
		     addrinfo != NULL;
		     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
			if (bad_server(fctx, &addrinfo->sockaddr))
				addrinfo->flags |= FCTX_ADDRINFO_MARK;
			else
				all_bad = ISC_FALSE;
		}
	}

	/*
	 * Mark any bad forwarders.
	 */
	for (addrinfo = ISC_LIST_HEAD(fctx->forwaddrs);
	     addrinfo != NULL;
	     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
		if (bad_server(fctx, &addrinfo->sockaddr))
			addrinfo->flags |= FCTX_ADDRINFO_MARK;
		else
			all_bad = ISC_FALSE;
	}

	/*
	 * Mark any bad alternates.
	 */
	for (curr = ISC_LIST_HEAD(fctx->altfinds);
	     curr != NULL;
	     curr = ISC_LIST_NEXT(curr, publink)) {
		for (addrinfo = ISC_LIST_HEAD(curr->list);
		     addrinfo != NULL;
		     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
			if (bad_server(fctx, &addrinfo->sockaddr))
				addrinfo->flags |= FCTX_ADDRINFO_MARK;
			else
				all_bad = ISC_FALSE;
		}
	}

	for (addrinfo = ISC_LIST_HEAD(fctx->altaddrs);
	     addrinfo != NULL;
	     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
		if (bad_server(fctx, &addrinfo->sockaddr))
			addrinfo->flags |= FCTX_ADDRINFO_MARK;
		else
			all_bad = ISC_FALSE;
	}

	return (all_bad);
}

static void
add_bad(fetchctx_t *fctx, dns_adbaddrinfo_t *addrinfo, isc_result_t reason,
	badnstype_t badtype)
{
	char namebuf[DNS_NAME_FORMATSIZE];
	char addrbuf[ISC_SOCKADDR_FORMATSIZE];
	char classbuf[64];
	char typebuf[64];
	char code[64];
	isc_buffer_t b;
	isc_sockaddr_t *sa;
	const char *spc = "";
	isc_sockaddr_t *address = &addrinfo->sockaddr;

	if (reason == DNS_R_LAME)
		fctx->lamecount++;
	else {
		switch (badtype) {
		case badns_unreachable:
			fctx->neterr++;
			break;
		case badns_response:
			fctx->badresp++;
			break;
		case badns_validation:
			break;	/* counted as 'valfail' */
		}
	}

	if (bad_server(fctx, address)) {
		/*
		 * We already know this server is bad.
		 */
		return;
	}

	FCTXTRACE("add_bad");

	sa = isc_mem_get(fctx->mctx, sizeof(*sa));
	if (sa == NULL)
		return;
	*sa = *address;
	ISC_LIST_INITANDAPPEND(fctx->bad, sa, link);

	if (reason == DNS_R_LAME)       /* already logged */
		return;

	if (reason == DNS_R_UNEXPECTEDRCODE &&
	    fctx->rmessage->rcode == dns_rcode_servfail &&
	    ISFORWARDER(addrinfo))
		return;

	if (reason == DNS_R_UNEXPECTEDRCODE) {
		isc_buffer_init(&b, code, sizeof(code) - 1);
		dns_rcode_totext(fctx->rmessage->rcode, &b);
		code[isc_buffer_usedlength(&b)] = '\0';
		spc = " ";
	} else if (reason == DNS_R_UNEXPECTEDOPCODE) {
		isc_buffer_init(&b, code, sizeof(code) - 1);
		dns_opcode_totext((dns_opcode_t)fctx->rmessage->opcode, &b);
		code[isc_buffer_usedlength(&b)] = '\0';
		spc = " ";
	} else {
		code[0] = '\0';
	}
	dns_name_format(&fctx->name, namebuf, sizeof(namebuf));
	dns_rdatatype_format(fctx->type, typebuf, sizeof(typebuf));
	dns_rdataclass_format(fctx->res->rdclass, classbuf, sizeof(classbuf));
	isc_sockaddr_format(address, addrbuf, sizeof(addrbuf));
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_LAME_SERVERS,
		      DNS_LOGMODULE_RESOLVER, ISC_LOG_INFO,
		      "%s%s%s resolving '%s/%s/%s': %s",
		      code, spc, dns_result_totext(reason),
		      namebuf, typebuf, classbuf, addrbuf);
}

/*
 * Sort addrinfo list by RTT.
 */
static void
sort_adbfind(dns_adbfind_t *find) {
	dns_adbaddrinfo_t *best, *curr;
	dns_adbaddrinfolist_t sorted;

	/* Lame N^2 bubble sort. */
	ISC_LIST_INIT(sorted);
	while (!ISC_LIST_EMPTY(find->list)) {
		best = ISC_LIST_HEAD(find->list);
		curr = ISC_LIST_NEXT(best, publink);
		while (curr != NULL) {
			if (curr->srtt < best->srtt)
				best = curr;
			curr = ISC_LIST_NEXT(curr, publink);
		}
		ISC_LIST_UNLINK(find->list, best, publink);
		ISC_LIST_APPEND(sorted, best, publink);
	}
	find->list = sorted;
}

/*
 * Sort a list of finds by server RTT.
 */
static void
sort_finds(dns_adbfindlist_t *findlist) {
	dns_adbfind_t *best, *curr;
	dns_adbfindlist_t sorted;
	dns_adbaddrinfo_t *addrinfo, *bestaddrinfo;

	/* Sort each find's addrinfo list by SRTT. */
	for (curr = ISC_LIST_HEAD(*findlist);
	     curr != NULL;
	     curr = ISC_LIST_NEXT(curr, publink))
		sort_adbfind(curr);

	/* Lame N^2 bubble sort. */
	ISC_LIST_INIT(sorted);
	while (!ISC_LIST_EMPTY(*findlist)) {
		best = ISC_LIST_HEAD(*findlist);
		bestaddrinfo = ISC_LIST_HEAD(best->list);
		INSIST(bestaddrinfo != NULL);
		curr = ISC_LIST_NEXT(best, publink);
		while (curr != NULL) {
			addrinfo = ISC_LIST_HEAD(curr->list);
			INSIST(addrinfo != NULL);
			if (addrinfo->srtt < bestaddrinfo->srtt) {
				best = curr;
				bestaddrinfo = addrinfo;
			}
			curr = ISC_LIST_NEXT(curr, publink);
		}
		ISC_LIST_UNLINK(*findlist, best, publink);
		ISC_LIST_APPEND(sorted, best, publink);
	}
	*findlist = sorted;
}

static void
findname(fetchctx_t *fctx, dns_name_t *name, in_port_t port,
	 unsigned int options, unsigned int flags, isc_stdtime_t now,
	 isc_boolean_t *overquota, isc_boolean_t *need_alternate)
{
	dns_adbaddrinfo_t *ai;
	dns_adbfind_t *find;
	dns_resolver_t *res;
	isc_boolean_t unshared;
	isc_result_t result;

#ifndef ENABLE_FETCHLIMIT
	UNUSED(overquota);
#endif /* !ENABLE_FETCHLIMIT */

	res = fctx->res;
	unshared = ISC_TF((fctx->options & DNS_FETCHOPT_UNSHARED) != 0);
	/*
	 * If this name is a subdomain of the query domain, tell
	 * the ADB to start looking using zone/hint data. This keeps us
	 * from getting stuck if the nameserver is beneath the zone cut
	 * and we don't know its address (e.g. because the A record has
	 * expired).
	 */
	if (dns_name_issubdomain(name, &fctx->domain))
		options |= DNS_ADBFIND_STARTATZONE;
	options |= DNS_ADBFIND_GLUEOK;
	options |= DNS_ADBFIND_HINTOK;

	/*
	 * See what we know about this address.
	 */
	find = NULL;
	result = dns_adb_createfind2(fctx->adb,
				     res->buckets[fctx->bucketnum].task,
				     fctx_finddone, fctx, name,
				     &fctx->name, fctx->type,
				     options, now, NULL,
				     res->view->dstport,
				     fctx->depth + 1, fctx->qc, &find);
	if (result != ISC_R_SUCCESS) {
		if (result == DNS_R_ALIAS) {
			char namebuf[DNS_NAME_FORMATSIZE];

			/*
			 * XXXRTH  Follow the CNAME/DNAME chain?
			 */
			dns_adb_destroyfind(&find);
			fctx->adberr++;
			dns_name_format(name, namebuf, sizeof(namebuf));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_CNAME,
				      DNS_LOGMODULE_RESOLVER, ISC_LOG_INFO,
				      "skipping nameserver '%s' because it "
				      "is a CNAME, while resolving '%s'",
				      namebuf, fctx->info);
		}
	} else if (!ISC_LIST_EMPTY(find->list)) {
		/*
		 * We have at least some of the addresses for the
		 * name.
		 */
		INSIST((find->options & DNS_ADBFIND_WANTEVENT) == 0);
		if (flags != 0 || port != 0) {
			for (ai = ISC_LIST_HEAD(find->list);
			     ai != NULL;
			     ai = ISC_LIST_NEXT(ai, publink)) {
				ai->flags |= flags;
				if (port != 0)
					isc_sockaddr_setport(&ai->sockaddr,
							     port);
			}
		}
		if ((flags & FCTX_ADDRINFO_FORWARDER) != 0)
			ISC_LIST_APPEND(fctx->altfinds, find, publink);
		else
			ISC_LIST_APPEND(fctx->finds, find, publink);
	} else {
		/*
		 * We don't know any of the addresses for this
		 * name.
		 */
		if ((find->options & DNS_ADBFIND_WANTEVENT) != 0) {
			/*
			 * We're looking for them and will get an
			 * event about it later.
			 */
			fctx->pending++;
			/*
			 * Bootstrap.
			 */
			if (need_alternate != NULL &&
			    !*need_alternate && unshared &&
			    ((res->dispatches4 == NULL &&
			      find->result_v6 != DNS_R_NXDOMAIN) ||
			     (res->dispatches6 == NULL &&
			      find->result_v4 != DNS_R_NXDOMAIN)))
				*need_alternate = ISC_TRUE;
		} else {
#ifdef ENABLE_FETCHLIMIT
			if ((find->options & DNS_ADBFIND_OVERQUOTA) != 0) {
				if (overquota != NULL)
					*overquota = ISC_TRUE;
				fctx->quotacount++; /* quota exceeded */
			}
			else
#endif /* ENABLE_FETCHLIMIT */
			if ((find->options & DNS_ADBFIND_LAMEPRUNED) != 0)
				fctx->lamecount++; /* cached lame server */
			else
				fctx->adberr++; /* unreachable server, etc. */

			/*
			 * If we know there are no addresses for
			 * the family we are using then try to add
			 * an alternative server.
			 */
			if (need_alternate != NULL && !*need_alternate &&
			    ((res->dispatches4 == NULL &&
			      find->result_v6 == DNS_R_NXRRSET) ||
			     (res->dispatches6 == NULL &&
			      find->result_v4 == DNS_R_NXRRSET)))
				*need_alternate = ISC_TRUE;
			dns_adb_destroyfind(&find);
		}
	}
}

static isc_boolean_t
isstrictsubdomain(dns_name_t *name1, dns_name_t *name2) {
	int order;
	unsigned int nlabels;
	dns_namereln_t namereln;

	namereln = dns_name_fullcompare(name1, name2, &order, &nlabels);
	return (ISC_TF(namereln == dns_namereln_subdomain));
}

static isc_result_t
fctx_getaddresses(fetchctx_t *fctx, isc_boolean_t badcache) {
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_result_t result;
	dns_resolver_t *res;
	isc_stdtime_t now;
	unsigned int stdoptions = 0;
	dns_forwarder_t *fwd;
	dns_adbaddrinfo_t *ai;
	isc_boolean_t all_bad;
	dns_rdata_ns_t ns;
	isc_boolean_t need_alternate = ISC_FALSE;
#ifdef ENABLE_FETCHLIMIT
	isc_boolean_t all_spilled = ISC_TRUE;
#endif /* ENABLE_FETCHLIMIT */

	FCTXTRACE5("getaddresses", "fctx->depth=", fctx->depth);

	/*
	 * Don't pound on remote servers.  (Failsafe!)
	 */
	fctx->restarts++;
	if (fctx->restarts > 100) {
		FCTXTRACE("too many restarts");
		return (DNS_R_SERVFAIL);
	}

	res = fctx->res;

	if (fctx->depth > res->maxdepth) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
			      DNS_LOGMODULE_RESOLVER, ISC_LOG_DEBUG(3),
			      "too much NS indirection resolving '%s' "
			      "(depth=%u, maxdepth=%u)",
			      fctx->info, fctx->depth, res->maxdepth);
		return (DNS_R_SERVFAIL);
	}

	/*
	 * Forwarders.
	 */

	INSIST(ISC_LIST_EMPTY(fctx->forwaddrs));
	INSIST(ISC_LIST_EMPTY(fctx->altaddrs));

	/*
	 * If this fctx has forwarders, use them; otherwise use any
	 * selective forwarders specified in the view; otherwise use the
	 * resolver's forwarders (if any).
	 */
	fwd = ISC_LIST_HEAD(fctx->forwarders);
	if (fwd == NULL) {
		dns_forwarders_t *forwarders = NULL;
		dns_name_t *name = &fctx->name;
		dns_name_t suffix;
		unsigned int labels;
		dns_fixedname_t fixed;
		dns_name_t *domain;

		/*
		 * DS records are found in the parent server.
		 * Strip label to get the correct forwarder (if any).
		 */
		if (dns_rdatatype_atparent(fctx->type) &&
		    dns_name_countlabels(name) > 1) {
			dns_name_init(&suffix, NULL);
			labels = dns_name_countlabels(name);
			dns_name_getlabelsequence(name, 1, labels - 1, &suffix);
			name = &suffix;
		}

		dns_fixedname_init(&fixed);
		domain = dns_fixedname_name(&fixed);
		result = dns_fwdtable_find2(res->view->fwdtable, name,
					    domain, &forwarders);
		if (result == ISC_R_SUCCESS) {
			fwd = ISC_LIST_HEAD(forwarders->fwdrs);
			fctx->fwdpolicy = forwarders->fwdpolicy;
			if (fctx->fwdpolicy == dns_fwdpolicy_only &&
			    isstrictsubdomain(domain, &fctx->domain)) {
#ifdef ENABLE_FETCHLIMIT
				fcount_decr(fctx);
#endif /* ENABLE_FETCHLIMIT */
				dns_name_free(&fctx->domain, fctx->mctx);
				dns_name_init(&fctx->domain, NULL);
				result = dns_name_dup(domain, fctx->mctx,
						      &fctx->domain);
				if (result != ISC_R_SUCCESS)
					return (result);
#ifdef ENABLE_FETCHLIMIT
				result = fcount_incr(fctx, ISC_TRUE);
				if (result != ISC_R_SUCCESS)
					return (result);
#endif /* ENABLE_FETCHLIMIT */
			}
		}
	}

	while (fwd != NULL) {
		if ((isc_sockaddr_pf(&fwd->addr) == AF_INET &&
		     res->dispatches4 == NULL) ||
		    (isc_sockaddr_pf(&fwd->addr) == AF_INET6 &&
		     res->dispatches6 == NULL))
		{
				fwd = ISC_LIST_NEXT(fwd, link);
				continue;
		}
		ai = NULL;
		result = dns_adb_findaddrinfo(fctx->adb, &fwd->addr, &ai, 0);
		if (result == ISC_R_SUCCESS) {
			dns_adbaddrinfo_t *cur;
			ai->flags |= FCTX_ADDRINFO_FORWARDER;
			ai->dscp = fwd->dscp;
			cur = ISC_LIST_HEAD(fctx->forwaddrs);
			while (cur != NULL && cur->srtt < ai->srtt)
				cur = ISC_LIST_NEXT(cur, publink);
			if (cur != NULL)
				ISC_LIST_INSERTBEFORE(fctx->forwaddrs, cur,
						      ai, publink);
			else
				ISC_LIST_APPEND(fctx->forwaddrs, ai, publink);
		}
		fwd = ISC_LIST_NEXT(fwd, link);
	}

	/*
	 * If the forwarding policy is "only", we don't need the addresses
	 * of the nameservers.
	 */
	if (fctx->fwdpolicy == dns_fwdpolicy_only)
		goto out;

	/*
	 * Normal nameservers.
	 */

	stdoptions = DNS_ADBFIND_WANTEVENT | DNS_ADBFIND_EMPTYEVENT;
	if (fctx->restarts == 1) {
		/*
		 * To avoid sending out a flood of queries likely to
		 * result in NXRRSET, we suppress fetches for address
		 * families we don't have the first time through,
		 * provided that we have addresses in some family we
		 * can use.
		 *
		 * We don't want to set this option all the time, since
		 * if fctx->restarts > 1, we've clearly been having trouble
		 * with the addresses we had, so getting more could help.
		 */
		stdoptions |= DNS_ADBFIND_AVOIDFETCHES;
	}
	if (res->dispatches4 != NULL)
		stdoptions |= DNS_ADBFIND_INET;
	if (res->dispatches6 != NULL)
		stdoptions |= DNS_ADBFIND_INET6;

	if ((stdoptions & DNS_ADBFIND_ADDRESSMASK) == 0)
		return (DNS_R_SERVFAIL);

	isc_stdtime_get(&now);

	INSIST(ISC_LIST_EMPTY(fctx->finds));
	INSIST(ISC_LIST_EMPTY(fctx->altfinds));

	for (result = dns_rdataset_first(&fctx->nameservers);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(&fctx->nameservers))
	{
		isc_boolean_t overquota = ISC_FALSE;

		dns_rdataset_current(&fctx->nameservers, &rdata);
		/*
		 * Extract the name from the NS record.
		 */
		result = dns_rdata_tostruct(&rdata, &ns, NULL);
		if (result != ISC_R_SUCCESS)
			continue;

		findname(fctx, &ns.name, 0, stdoptions, 0, now,
			 &overquota, &need_alternate);

#ifdef ENABLE_FETCHLIMIT
		if (!overquota)
			all_spilled = ISC_FALSE;
#endif /* ENABLE_FETCHLIMIT */

		dns_rdata_reset(&rdata);
		dns_rdata_freestruct(&ns);
	}
	if (result != ISC_R_NOMORE)
		return (result);

	/*
	 * Do we need to use 6 to 4?
	 */
	if (need_alternate) {
		int family;
		alternate_t *a;
		family = (res->dispatches6 != NULL) ? AF_INET6 : AF_INET;
		for (a = ISC_LIST_HEAD(res->alternates);
		     a != NULL;
		     a = ISC_LIST_NEXT(a, link)) {
			if (!a->isaddress) {
				findname(fctx, &a->_u._n.name, a->_u._n.port,
					 stdoptions, FCTX_ADDRINFO_FORWARDER,
					 now, NULL, NULL);
				continue;
			}
			if (isc_sockaddr_pf(&a->_u.addr) != family)
				continue;
			ai = NULL;
			result = dns_adb_findaddrinfo(fctx->adb, &a->_u.addr,
						      &ai, 0);
			if (result == ISC_R_SUCCESS) {
				dns_adbaddrinfo_t *cur;
				ai->flags |= FCTX_ADDRINFO_FORWARDER;
				cur = ISC_LIST_HEAD(fctx->altaddrs);
				while (cur != NULL && cur->srtt < ai->srtt)
					cur = ISC_LIST_NEXT(cur, publink);
				if (cur != NULL)
					ISC_LIST_INSERTBEFORE(fctx->altaddrs,
							      cur, ai, publink);
				else
					ISC_LIST_APPEND(fctx->altaddrs, ai,
							publink);
			}
		}
	}

 out:
	/*
	 * Mark all known bad servers.
	 */
	all_bad = mark_bad(fctx);

	/*
	 * How are we doing?
	 */
	if (all_bad) {
		/*
		 * We've got no addresses.
		 */
		if (fctx->pending > 0) {
			/*
			 * We're fetching the addresses, but don't have any
			 * yet.   Tell the caller to wait for an answer.
			 */
			result = DNS_R_WAIT;
		} else {
			isc_time_t expire;
			isc_interval_t i;
			/*
			 * We've lost completely.  We don't know any
			 * addresses, and the ADB has told us it can't get
			 * them.
			 */
			FCTXTRACE("no addresses");
			isc_interval_set(&i, DNS_BADCACHE_TTL(fctx), 0);
			result = isc_time_nowplusinterval(&expire, &i);
			if (badcache &&
			    (fctx->type == dns_rdatatype_dnskey ||
			     fctx->type == dns_rdatatype_dlv ||
			     fctx->type == dns_rdatatype_ds) &&
			     result == ISC_R_SUCCESS)
				dns_resolver_addbadcache(res, &fctx->name,
							 fctx->type, &expire);

			result = ISC_R_FAILURE;

#ifdef ENABLE_FETCHLIMIT
			/*
			 * If all of the addresses found were over the
			 * fetches-per-server quota, return the configured
			 * response.
			 */
			if (all_spilled) {
				result = res->quotaresp[dns_quotatype_server];
				inc_stats(res, dns_resstatscounter_serverquota);
			}
#endif /* ENABLE_FETCHLIMIT */
		}
	} else {
		/*
		 * We've found some addresses.  We might still be looking
		 * for more addresses.
		 */
		sort_finds(&fctx->finds);
		sort_finds(&fctx->altfinds);
		result = ISC_R_SUCCESS;
	}

	return (result);
}

static inline void
possibly_mark(fetchctx_t *fctx, dns_adbaddrinfo_t *addr) {
	isc_netaddr_t na;
	char buf[ISC_NETADDR_FORMATSIZE];
	isc_sockaddr_t *sa;
	isc_boolean_t aborted = ISC_FALSE;
	isc_boolean_t bogus;
	dns_acl_t *blackhole;
	isc_netaddr_t ipaddr;
	dns_peer_t *peer = NULL;
	dns_resolver_t *res;
	const char *msg = NULL;

	sa = &addr->sockaddr;

	res = fctx->res;
	isc_netaddr_fromsockaddr(&ipaddr, sa);
	blackhole = dns_dispatchmgr_getblackhole(res->dispatchmgr);
	(void) dns_peerlist_peerbyaddr(res->view->peers, &ipaddr, &peer);

	if (blackhole != NULL) {
		int match;

		if (dns_acl_match(&ipaddr, NULL, blackhole,
				  &res->view->aclenv,
				  &match, NULL) == ISC_R_SUCCESS &&
		    match > 0)
			aborted = ISC_TRUE;
	}

	if (peer != NULL &&
	    dns_peer_getbogus(peer, &bogus) == ISC_R_SUCCESS &&
	    bogus)
		aborted = ISC_TRUE;

	if (aborted) {
		addr->flags |= FCTX_ADDRINFO_MARK;
		msg = "ignoring blackholed / bogus server: ";
	} else if (isc_sockaddr_isnetzero(sa)) {
		addr->flags |= FCTX_ADDRINFO_MARK;
		msg = "ignoring net zero address: ";
	} else if (isc_sockaddr_ismulticast(sa)) {
		addr->flags |= FCTX_ADDRINFO_MARK;
		msg = "ignoring multicast address: ";
	} else if (isc_sockaddr_isexperimental(sa)) {
		addr->flags |= FCTX_ADDRINFO_MARK;
		msg = "ignoring experimental address: ";
	} else if (sa->type.sa.sa_family != AF_INET6) {
		return;
	} else if (IN6_IS_ADDR_V4MAPPED(&sa->type.sin6.sin6_addr)) {
		addr->flags |= FCTX_ADDRINFO_MARK;
		msg = "ignoring IPv6 mapped IPV4 address: ";
	} else if (IN6_IS_ADDR_V4COMPAT(&sa->type.sin6.sin6_addr)) {
		addr->flags |= FCTX_ADDRINFO_MARK;
		msg = "ignoring IPv6 compatibility IPV4 address: ";
	} else
		return;

	if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(3))) {
		isc_netaddr_fromsockaddr(&na, sa);
		isc_netaddr_format(&na, buf, sizeof(buf));
		FCTXTRACE2(msg, buf);
	}
}

static inline dns_adbaddrinfo_t *
fctx_nextaddress(fetchctx_t *fctx) {
	dns_adbfind_t *find, *start;
	dns_adbaddrinfo_t *addrinfo;
	dns_adbaddrinfo_t *faddrinfo;

	/*
	 * Return the next untried address, if any.
	 */

	/*
	 * Find the first unmarked forwarder (if any).
	 */
	for (addrinfo = ISC_LIST_HEAD(fctx->forwaddrs);
	     addrinfo != NULL;
	     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
		if (!UNMARKED(addrinfo))
			continue;
		possibly_mark(fctx, addrinfo);
		if (UNMARKED(addrinfo)) {
			addrinfo->flags |= FCTX_ADDRINFO_MARK;
			fctx->find = NULL;
			return (addrinfo);
		}
	}

	/*
	 * No forwarders.  Move to the next find.
	 */

	fctx->attributes |= FCTX_ATTR_TRIEDFIND;

	find = fctx->find;
	if (find == NULL)
		find = ISC_LIST_HEAD(fctx->finds);
	else {
		find = ISC_LIST_NEXT(find, publink);
		if (find == NULL)
			find = ISC_LIST_HEAD(fctx->finds);
	}

	/*
	 * Find the first unmarked addrinfo.
	 */
	addrinfo = NULL;
	if (find != NULL) {
		start = find;
		do {
			for (addrinfo = ISC_LIST_HEAD(find->list);
			     addrinfo != NULL;
			     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
				if (!UNMARKED(addrinfo))
					continue;
				possibly_mark(fctx, addrinfo);
				if (UNMARKED(addrinfo)) {
					addrinfo->flags |= FCTX_ADDRINFO_MARK;
					break;
				}
			}
			if (addrinfo != NULL)
				break;
			find = ISC_LIST_NEXT(find, publink);
			if (find == NULL)
				find = ISC_LIST_HEAD(fctx->finds);
		} while (find != start);
	}

	fctx->find = find;
	if (addrinfo != NULL)
		return (addrinfo);

	/*
	 * No nameservers left.  Try alternates.
	 */

	fctx->attributes |= FCTX_ATTR_TRIEDALT;

	find = fctx->altfind;
	if (find == NULL)
		find = ISC_LIST_HEAD(fctx->altfinds);
	else {
		find = ISC_LIST_NEXT(find, publink);
		if (find == NULL)
			find = ISC_LIST_HEAD(fctx->altfinds);
	}

	/*
	 * Find the first unmarked addrinfo.
	 */
	addrinfo = NULL;
	if (find != NULL) {
		start = find;
		do {
			for (addrinfo = ISC_LIST_HEAD(find->list);
			     addrinfo != NULL;
			     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
				if (!UNMARKED(addrinfo))
					continue;
				possibly_mark(fctx, addrinfo);
				if (UNMARKED(addrinfo)) {
					addrinfo->flags |= FCTX_ADDRINFO_MARK;
					break;
				}
			}
			if (addrinfo != NULL)
				break;
			find = ISC_LIST_NEXT(find, publink);
			if (find == NULL)
				find = ISC_LIST_HEAD(fctx->altfinds);
		} while (find != start);
	}

	faddrinfo = addrinfo;

	/*
	 * See if we have a better alternate server by address.
	 */

	for (addrinfo = ISC_LIST_HEAD(fctx->altaddrs);
	     addrinfo != NULL;
	     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
		if (!UNMARKED(addrinfo))
			continue;
		possibly_mark(fctx, addrinfo);
		if (UNMARKED(addrinfo) &&
		    (faddrinfo == NULL ||
		     addrinfo->srtt < faddrinfo->srtt)) {
			if (faddrinfo != NULL)
				faddrinfo->flags &= ~FCTX_ADDRINFO_MARK;
			addrinfo->flags |= FCTX_ADDRINFO_MARK;
			break;
		}
	}

	if (addrinfo == NULL) {
		addrinfo = faddrinfo;
		fctx->altfind = find;
	}

	return (addrinfo);
}

static void
fctx_try(fetchctx_t *fctx, isc_boolean_t retrying, isc_boolean_t badcache) {
	isc_result_t result;
	dns_adbaddrinfo_t *addrinfo = NULL;
	dns_resolver_t *res;
	unsigned int bucketnum;
	isc_boolean_t bucket_empty;

	FCTXTRACE5("try", "fctx->qc=", isc_counter_used(fctx->qc));

	REQUIRE(!ADDRWAIT(fctx));

	res = fctx->res;

	/* We've already exceeded maximum query count */
	if (isc_counter_used(fctx->qc) > res->maxqueries) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
			      DNS_LOGMODULE_RESOLVER, ISC_LOG_DEBUG(3),
			      "exceeded max queries resolving '%s' "
			      "(querycount=%u, maxqueries=%u)",
			      fctx->info,
			      isc_counter_used(fctx->qc), res->maxqueries);
		fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
		return;
	}

	addrinfo = fctx_nextaddress(fctx);

#ifdef ENABLE_FETCHLIMIT
	/* Try to find an address that isn't over quota */
	while (addrinfo != NULL && dns_adbentry_overquota(addrinfo->entry))
		addrinfo = fctx_nextaddress(fctx);
#endif /* ENABLE_FETCHLIMIT */

	if (addrinfo == NULL) {
		/* We have no more addresses.  Start over. */
		fctx_cancelqueries(fctx, ISC_TRUE, ISC_FALSE);
		fctx_cleanupfinds(fctx);
		fctx_cleanupaltfinds(fctx);
		fctx_cleanupforwaddrs(fctx);
		fctx_cleanupaltaddrs(fctx);
		result = fctx_getaddresses(fctx, badcache);
		if (result == DNS_R_WAIT) {
			/*
			 * Sleep waiting for addresses.
			 */
			FCTXTRACE("addrwait");
			fctx->attributes |= FCTX_ATTR_ADDRWAIT;
			return;
		} else if (result != ISC_R_SUCCESS) {
			/*
			 * Something bad happened.
			 */
			fctx_done(fctx, result, __LINE__);
			return;
		}

		addrinfo = fctx_nextaddress(fctx);

#ifdef ENABLE_FETCHLIMIT
		/* Try to find an address that isn't over quota */
		while (addrinfo != NULL &&
		       dns_adbentry_overquota(addrinfo->entry))
			addrinfo = fctx_nextaddress(fctx);
#endif /* ENABLE_FETCHLIMIT */

		/*
		 * While we may have addresses from the ADB, they
		 * might be bad ones.  In this case, return SERVFAIL.
		 */
		if (addrinfo == NULL) {
			fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
			return;
		}
	}

	if (dns_name_countlabels(&fctx->domain) > 2) {
		result = isc_counter_increment(fctx->qc);
		if (result != ISC_R_SUCCESS) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
				      DNS_LOGMODULE_RESOLVER, ISC_LOG_DEBUG(3),
				      "exceeded max queries resolving '%s'",
				      fctx->info);
			fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
			return;
		}
	}

	bucketnum = fctx->bucketnum;
	fctx_increference(fctx);
	result = fctx_query(fctx, addrinfo, fctx->options);
	if (result != ISC_R_SUCCESS) {
		fctx_done(fctx, result, __LINE__);
		LOCK(&res->buckets[bucketnum].lock);
		bucket_empty = fctx_decreference(fctx);
		UNLOCK(&res->buckets[bucketnum].lock);
		if (bucket_empty)
			empty_bucket(res);
	} else if (retrying)
		inc_stats(res, dns_resstatscounter_retry);
}

static isc_boolean_t
fctx_unlink(fetchctx_t *fctx) {
	dns_resolver_t *res;
	unsigned int bucketnum;

	/*
	 * Caller must be holding the bucket lock.
	 */

	REQUIRE(VALID_FCTX(fctx));
	REQUIRE(fctx->state == fetchstate_done ||
		fctx->state == fetchstate_init);
	REQUIRE(ISC_LIST_EMPTY(fctx->events));
	REQUIRE(ISC_LIST_EMPTY(fctx->queries));
	REQUIRE(ISC_LIST_EMPTY(fctx->finds));
	REQUIRE(ISC_LIST_EMPTY(fctx->altfinds));
	REQUIRE(fctx->pending == 0);
	REQUIRE(fctx->references == 0);
	REQUIRE(ISC_LIST_EMPTY(fctx->validators));

	FCTXTRACE("unlink");

	res = fctx->res;
	bucketnum = fctx->bucketnum;

	ISC_LIST_UNLINK(res->buckets[bucketnum].fctxs, fctx, link);

	LOCK(&res->nlock);
	res->nfctx--;
	UNLOCK(&res->nlock);
	dec_stats(res, dns_resstatscounter_nfetch);

	if (res->buckets[bucketnum].exiting &&
	    ISC_LIST_EMPTY(res->buckets[bucketnum].fctxs))
		return (ISC_TRUE);

	return (ISC_FALSE);
}

static void
fctx_destroy(fetchctx_t *fctx) {
	isc_sockaddr_t *sa, *next_sa;
	struct tried *tried;

	REQUIRE(VALID_FCTX(fctx));
	REQUIRE(fctx->state == fetchstate_done ||
		fctx->state == fetchstate_init);
	REQUIRE(ISC_LIST_EMPTY(fctx->events));
	REQUIRE(ISC_LIST_EMPTY(fctx->queries));
	REQUIRE(ISC_LIST_EMPTY(fctx->finds));
	REQUIRE(ISC_LIST_EMPTY(fctx->altfinds));
	REQUIRE(fctx->pending == 0);
	REQUIRE(fctx->references == 0);
	REQUIRE(ISC_LIST_EMPTY(fctx->validators));
	REQUIRE(!ISC_LINK_LINKED(fctx, link));

	FCTXTRACE("destroy");

	/*
	 * Free bad.
	 */
	for (sa = ISC_LIST_HEAD(fctx->bad);
	     sa != NULL;
	     sa = next_sa) {
		next_sa = ISC_LIST_NEXT(sa, link);
		ISC_LIST_UNLINK(fctx->bad, sa, link);
		isc_mem_put(fctx->mctx, sa, sizeof(*sa));
	}

	for (tried = ISC_LIST_HEAD(fctx->edns);
	     tried != NULL;
	     tried = ISC_LIST_HEAD(fctx->edns)) {
		ISC_LIST_UNLINK(fctx->edns, tried, link);
		isc_mem_put(fctx->mctx, tried, sizeof(*tried));
	}

	for (tried = ISC_LIST_HEAD(fctx->edns512);
	     tried != NULL;
	     tried = ISC_LIST_HEAD(fctx->edns512)) {
		ISC_LIST_UNLINK(fctx->edns512, tried, link);
		isc_mem_put(fctx->mctx, tried, sizeof(*tried));
	}

	for (sa = ISC_LIST_HEAD(fctx->bad_edns);
	     sa != NULL;
	     sa = next_sa) {
		next_sa = ISC_LIST_NEXT(sa, link);
		ISC_LIST_UNLINK(fctx->bad_edns, sa, link);
		isc_mem_put(fctx->mctx, sa, sizeof(*sa));
	}

	isc_counter_detach(&fctx->qc);

#ifdef ENABLE_FETCHLIMIT
	fcount_decr(fctx);
#endif /* ENABLE_FETCHLIMIT */

	isc_timer_detach(&fctx->timer);
	dns_message_destroy(&fctx->rmessage);
	dns_message_destroy(&fctx->qmessage);
	if (dns_name_countlabels(&fctx->domain) > 0)
		dns_name_free(&fctx->domain, fctx->mctx);
	if (dns_rdataset_isassociated(&fctx->nameservers))
		dns_rdataset_disassociate(&fctx->nameservers);
	dns_name_free(&fctx->name, fctx->mctx);
	dns_db_detach(&fctx->cache);
	dns_adb_detach(&fctx->adb);
	isc_mem_free(fctx->mctx, fctx->info);
	isc_mem_putanddetach(&fctx->mctx, fctx, sizeof(*fctx));
}

/*
 * Fetch event handlers.
 */

static void
fctx_timeout(isc_task_t *task, isc_event_t *event) {
	fetchctx_t *fctx = event->ev_arg;
	isc_timerevent_t *tevent = (isc_timerevent_t *)event;
	resquery_t *query;

	REQUIRE(VALID_FCTX(fctx));

	UNUSED(task);

	FCTXTRACE("timeout");

	inc_stats(fctx->res, dns_resstatscounter_querytimeout);

	if (event->ev_type == ISC_TIMEREVENT_LIFE) {
		fctx->reason = NULL;
		fctx_done(fctx, ISC_R_TIMEDOUT, __LINE__);
	} else {
		isc_result_t result;

		fctx->timeouts++;
		fctx->timeout = ISC_TRUE;

		/*
		 * We could cancel the running queries here, or we could let
		 * them keep going.  Since we normally use separate sockets for
		 * different queries, we adopt the former approach to reduce
		 * the number of open sockets: cancel the oldest query if it
		 * expired after the query had started (this is usually the
		 * case but is not always so, depending on the task schedule
		 * timing).
		 */
		query = ISC_LIST_HEAD(fctx->queries);
		if (query != NULL &&
		    isc_time_compare(&tevent->due, &query->start) >= 0)
		{
			FCTXTRACE("query timed out; no response");
			fctx_cancelquery(&query, NULL, NULL, ISC_TRUE, ISC_FALSE);
		}
		fctx->attributes &= ~FCTX_ATTR_ADDRWAIT;

		/*
		 * Our timer has triggered.  Reestablish the fctx lifetime
		 * timer.
		 */
		result = fctx_starttimer(fctx);
		if (result != ISC_R_SUCCESS)
			fctx_done(fctx, result, __LINE__);
		else
			/*
			 * Keep trying.
			 */
			fctx_try(fctx, ISC_TRUE, ISC_FALSE);
	}

	isc_event_free(&event);
}

static void
fctx_shutdown(fetchctx_t *fctx) {
	isc_event_t *cevent;

	/*
	 * Start the shutdown process for fctx, if it isn't already underway.
	 */

	FCTXTRACE("shutdown");

	/*
	 * The caller must be holding the appropriate bucket lock.
	 */

	if (fctx->want_shutdown)
		return;

	fctx->want_shutdown = ISC_TRUE;

	/*
	 * Unless we're still initializing (in which case the
	 * control event is still outstanding), we need to post
	 * the control event to tell the fetch we want it to
	 * exit.
	 */
	if (fctx->state != fetchstate_init) {
		cevent = &fctx->control_event;
		isc_task_send(fctx->res->buckets[fctx->bucketnum].task,
			      &cevent);
	}
}

static void
fctx_doshutdown(isc_task_t *task, isc_event_t *event) {
	fetchctx_t *fctx = event->ev_arg;
	isc_boolean_t bucket_empty = ISC_FALSE;
	dns_resolver_t *res;
	unsigned int bucketnum;
	dns_validator_t *validator;
	isc_boolean_t dodestroy = ISC_FALSE;

	REQUIRE(VALID_FCTX(fctx));

	UNUSED(task);

	res = fctx->res;
	bucketnum = fctx->bucketnum;

	FCTXTRACE("doshutdown");

	/*
	 * An fctx that is shutting down is no longer in ADDRWAIT mode.
	 */
	fctx->attributes &= ~FCTX_ATTR_ADDRWAIT;

	/*
	 * Cancel all pending validators.  Note that this must be done
	 * without the bucket lock held, since that could cause deadlock.
	 */
	validator = ISC_LIST_HEAD(fctx->validators);
	while (validator != NULL) {
		dns_validator_cancel(validator);
		validator = ISC_LIST_NEXT(validator, link);
	}

	if (fctx->nsfetch != NULL)
		dns_resolver_cancelfetch(fctx->nsfetch);

	/*
	 * Shut down anything still running on behalf of this
	 * fetch, and clean up finds and addresses.  To avoid deadlock
	 * with the ADB, we must do this before we lock the bucket lock.
	 */
	fctx_stopqueries(fctx, ISC_FALSE, ISC_FALSE);
	fctx_cleanupall(fctx);

	LOCK(&res->buckets[bucketnum].lock);

	fctx->attributes |= FCTX_ATTR_SHUTTINGDOWN;

	INSIST(fctx->state == fetchstate_active ||
	       fctx->state == fetchstate_done);
	INSIST(fctx->want_shutdown);

	if (fctx->state != fetchstate_done) {
		fctx->state = fetchstate_done;
		fctx_sendevents(fctx, ISC_R_CANCELED, __LINE__);
	}

	if (fctx->references == 0 && fctx->pending == 0 &&
	    fctx->nqueries == 0 && ISC_LIST_EMPTY(fctx->validators)) {
		bucket_empty = fctx_unlink(fctx);
		dodestroy = ISC_TRUE;
	}

	UNLOCK(&res->buckets[bucketnum].lock);

	if (dodestroy) {
		fctx_destroy(fctx);
		if (bucket_empty)
			empty_bucket(res);
	}
}

static void
fctx_start(isc_task_t *task, isc_event_t *event) {
	fetchctx_t *fctx = event->ev_arg;
	isc_boolean_t done = ISC_FALSE, bucket_empty = ISC_FALSE;
	dns_resolver_t *res;
	unsigned int bucketnum;
	isc_boolean_t dodestroy = ISC_FALSE;

	REQUIRE(VALID_FCTX(fctx));

	UNUSED(task);

	res = fctx->res;
	bucketnum = fctx->bucketnum;

	FCTXTRACE("start");

	LOCK(&res->buckets[bucketnum].lock);

	INSIST(fctx->state == fetchstate_init);
	if (fctx->want_shutdown) {
		/*
		 * We haven't started this fctx yet, and we've been requested
		 * to shut it down.
		 */
		fctx->attributes |= FCTX_ATTR_SHUTTINGDOWN;
		fctx->state = fetchstate_done;
		fctx_sendevents(fctx, ISC_R_CANCELED, __LINE__);
		/*
		 * Since we haven't started, we INSIST that we have no
		 * pending ADB finds and no pending validations.
		 */
		INSIST(fctx->pending == 0);
		INSIST(fctx->nqueries == 0);
		INSIST(ISC_LIST_EMPTY(fctx->validators));
		if (fctx->references == 0) {
			/*
			 * It's now safe to destroy this fctx.
			 */
			bucket_empty = fctx_unlink(fctx);
			dodestroy = ISC_TRUE;
		}
		done = ISC_TRUE;
	} else {
		/*
		 * Normal fctx startup.
		 */
		fctx->state = fetchstate_active;
		/*
		 * Reset the control event for later use in shutting down
		 * the fctx.
		 */
		ISC_EVENT_INIT(event, sizeof(*event), 0, NULL,
			       DNS_EVENT_FETCHCONTROL, fctx_doshutdown, fctx,
			       NULL, NULL, NULL);
	}

	UNLOCK(&res->buckets[bucketnum].lock);

	if (!done) {
		isc_result_t result;

		INSIST(!dodestroy);

		/*
		 * All is well.  Start working on the fetch.
		 */
		result = fctx_starttimer(fctx);
		if (result != ISC_R_SUCCESS)
			fctx_done(fctx, result, __LINE__);
		else
			fctx_try(fctx, ISC_FALSE, ISC_FALSE);
	} else if (dodestroy) {
			fctx_destroy(fctx);
		if (bucket_empty)
			empty_bucket(res);
	}
}

/*
 * Fetch Creation, Joining, and Cancelation.
 */

static inline isc_result_t
fctx_join(fetchctx_t *fctx, isc_task_t *task, isc_sockaddr_t *client,
	  dns_messageid_t id, isc_taskaction_t action, void *arg,
	  dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset,
	  dns_fetch_t *fetch)
{
	isc_task_t *tclone;
	dns_fetchevent_t *event;

	FCTXTRACE("join");

	/*
	 * We store the task we're going to send this event to in the
	 * sender field.  We'll make the fetch the sender when we actually
	 * send the event.
	 */
	tclone = NULL;
	isc_task_attach(task, &tclone);
	event = (dns_fetchevent_t *)
		isc_event_allocate(fctx->res->mctx, tclone, DNS_EVENT_FETCHDONE,
				   action, arg, sizeof(*event));
	if (event == NULL) {
		isc_task_detach(&tclone);
		return (ISC_R_NOMEMORY);
	}
	event->result = DNS_R_SERVFAIL;
	event->qtype = fctx->type;
	event->db = NULL;
	event->node = NULL;
	event->rdataset = rdataset;
	event->sigrdataset = sigrdataset;
	event->fetch = fetch;
	event->client = client;
	event->id = id;
	dns_fixedname_init(&event->foundname);

	/*
	 * Make sure that we can store the sigrdataset in the
	 * first event if it is needed by any of the events.
	 */
	if (event->sigrdataset != NULL)
		ISC_LIST_PREPEND(fctx->events, event, ev_link);
	else
		ISC_LIST_APPEND(fctx->events, event, ev_link);
	fctx->references++;
	fctx->client = client;

	fetch->magic = DNS_FETCH_MAGIC;
	fetch->private = fctx;

	return (ISC_R_SUCCESS);
}

static inline void
log_ns_ttl(fetchctx_t *fctx, const char *where) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char domainbuf[DNS_NAME_FORMATSIZE];

	dns_name_format(&fctx->name, namebuf, sizeof(namebuf));
	dns_name_format(&fctx->domain, domainbuf, sizeof(domainbuf));
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
		      DNS_LOGMODULE_RESOLVER, ISC_LOG_DEBUG(10),
		      "log_ns_ttl: fctx %p: %s: %s (in '%s'?): %u %u",
		      fctx, where, namebuf, domainbuf,
		      fctx->ns_ttl_ok, fctx->ns_ttl);
}

static isc_result_t
fctx_create(dns_resolver_t *res, dns_name_t *name, dns_rdatatype_t type,
	    dns_name_t *domain, dns_rdataset_t *nameservers,
	    unsigned int options, unsigned int bucketnum, unsigned int depth,
	    isc_counter_t *qc, fetchctx_t **fctxp)
{
	fetchctx_t *fctx;
	isc_result_t result;
	isc_result_t iresult;
	isc_interval_t interval;
	dns_fixedname_t fixed;
	unsigned int findoptions = 0;
	char buf[DNS_NAME_FORMATSIZE + DNS_RDATATYPE_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	isc_mem_t *mctx;

	/*
	 * Caller must be holding the lock for bucket number 'bucketnum'.
	 */
	REQUIRE(fctxp != NULL && *fctxp == NULL);

	mctx = res->buckets[bucketnum].mctx;
	fctx = isc_mem_get(mctx, sizeof(*fctx));
	if (fctx == NULL)
		return (ISC_R_NOMEMORY);

	fctx->qc = NULL;
	if (qc != NULL) {
		isc_counter_attach(qc, &fctx->qc);
	} else {
		result = isc_counter_create(res->mctx,
					    res->maxqueries, &fctx->qc);
		if (result != ISC_R_SUCCESS)
			goto cleanup_fetch;
	}

	/*
	 * Make fctx->info point to a copy of a formatted string
	 * "name/type".
	 */
	dns_name_format(name, buf, sizeof(buf));
	dns_rdatatype_format(type, typebuf, sizeof(typebuf));
	strlcat(buf, "/", sizeof(buf));
	strlcat(buf, typebuf, sizeof(buf));
	fctx->info = isc_mem_strdup(mctx, buf);
	if (fctx->info == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup_counter;
	}

	FCTXTRACE("create");
	dns_name_init(&fctx->name, NULL);
	result = dns_name_dup(name, mctx, &fctx->name);
	if (result != ISC_R_SUCCESS)
		goto cleanup_info;
	dns_name_init(&fctx->domain, NULL);
	dns_rdataset_init(&fctx->nameservers);

	fctx->type = type;
	fctx->options = options;
	/*
	 * Note!  We do not attach to the task.  We are relying on the
	 * resolver to ensure that this task doesn't go away while we are
	 * using it.
	 */
	fctx->res = res;
	fctx->references = 0;
	fctx->bucketnum = bucketnum;
	fctx->dbucketnum = RES_NOBUCKET;
	fctx->state = fetchstate_init;
	fctx->want_shutdown = ISC_FALSE;
	fctx->cloned = ISC_FALSE;
	fctx->depth = depth;
	ISC_LIST_INIT(fctx->queries);
	ISC_LIST_INIT(fctx->finds);
	ISC_LIST_INIT(fctx->altfinds);
	ISC_LIST_INIT(fctx->forwaddrs);
	ISC_LIST_INIT(fctx->altaddrs);
	ISC_LIST_INIT(fctx->forwarders);
	fctx->fwdpolicy = dns_fwdpolicy_none;
	ISC_LIST_INIT(fctx->bad);
	ISC_LIST_INIT(fctx->edns);
	ISC_LIST_INIT(fctx->edns512);
	ISC_LIST_INIT(fctx->bad_edns);
	ISC_LIST_INIT(fctx->validators);
	fctx->validator = NULL;
	fctx->find = NULL;
	fctx->altfind = NULL;
	fctx->pending = 0;
	fctx->restarts = 0;
	fctx->querysent = 0;
	fctx->referrals = 0;
	TIME_NOW(&fctx->start);
	fctx->timeouts = 0;
	fctx->lamecount = 0;
	fctx->quotacount = 0;
	fctx->adberr = 0;
	fctx->neterr = 0;
	fctx->badresp = 0;
	fctx->findfail = 0;
	fctx->valfail = 0;
	fctx->result = ISC_R_FAILURE;
	fctx->vresult = ISC_R_SUCCESS;
	fctx->exitline = -1;	/* sentinel */
	fctx->logged = ISC_FALSE;
	fctx->attributes = 0;
	fctx->spilled = ISC_FALSE;
	fctx->nqueries = 0;
	fctx->reason = NULL;
	fctx->rand_buf = 0;
	fctx->rand_bits = 0;
	fctx->timeout = ISC_FALSE;
	fctx->addrinfo = NULL;
	fctx->client = NULL;
	fctx->ns_ttl = 0;
	fctx->ns_ttl_ok = ISC_FALSE;

	dns_name_init(&fctx->nsname, NULL);
	fctx->nsfetch = NULL;
	dns_rdataset_init(&fctx->nsrrset);

	if (domain == NULL) {
		dns_forwarders_t *forwarders = NULL;
		unsigned int labels;
		dns_name_t *fwdname = name;
		dns_name_t suffix;

		/*
		 * DS records are found in the parent server. Strip one
		 * leading label from the name (to be used in finding
		 * the forwarder).
		 */
		if (dns_rdatatype_atparent(fctx->type) &&
		    dns_name_countlabels(name) > 1) {
			dns_name_init(&suffix, NULL);
			labels = dns_name_countlabels(name);
			dns_name_getlabelsequence(name, 1, labels - 1, &suffix);
			fwdname = &suffix;
		}

		/* Find the forwarder for this name. */
		dns_fixedname_init(&fixed);
		domain = dns_fixedname_name(&fixed);
		result = dns_fwdtable_find2(fctx->res->view->fwdtable, fwdname,
					    domain, &forwarders);
		if (result == ISC_R_SUCCESS)
			fctx->fwdpolicy = forwarders->fwdpolicy;

		if (fctx->fwdpolicy != dns_fwdpolicy_only) {
			/*
			 * The caller didn't supply a query domain and
			 * nameservers, and we're not in forward-only mode,
			 * so find the best nameservers to use.
			 */
			if (dns_rdatatype_atparent(fctx->type))
				findoptions |= DNS_DBFIND_NOEXACT;
			result = dns_view_findzonecut(res->view, name,
						      domain, 0, findoptions,
						      ISC_TRUE,
						      &fctx->nameservers,
						      NULL);
			if (result != ISC_R_SUCCESS)
				goto cleanup_nameservers;

			result = dns_name_dup(domain, mctx, &fctx->domain);
			if (result != ISC_R_SUCCESS)
				goto cleanup_nameservers;

			fctx->ns_ttl = fctx->nameservers.ttl;
			fctx->ns_ttl_ok = ISC_TRUE;
		} else {
			/*
			 * We're in forward-only mode.  Set the query domain.
			 */
			result = dns_name_dup(domain, mctx, &fctx->domain);
			if (result != ISC_R_SUCCESS)
				goto cleanup_name;
		}
	} else {
		result = dns_name_dup(domain, mctx, &fctx->domain);
		if (result != ISC_R_SUCCESS)
			goto cleanup_name;
		dns_rdataset_clone(nameservers, &fctx->nameservers);
		fctx->ns_ttl = fctx->nameservers.ttl;
		fctx->ns_ttl_ok = ISC_TRUE;
	}

#ifdef ENABLE_FETCHLIMIT
	/*
	 * Are there too many simultaneous queries for this domain?
	 */
	result = fcount_incr(fctx, ISC_FALSE);
	if (result != ISC_R_SUCCESS) {
		result = fctx->res->quotaresp[dns_quotatype_zone];
		inc_stats(res, dns_resstatscounter_zonequota);
		goto cleanup_domain;
	}
#endif /* ENABLE_FETCHLIMIT */

	log_ns_ttl(fctx, "fctx_create");

	INSIST(dns_name_issubdomain(&fctx->name, &fctx->domain));

	fctx->qmessage = NULL;
	result = dns_message_create(mctx, DNS_MESSAGE_INTENTRENDER,
				    &fctx->qmessage);

	if (result != ISC_R_SUCCESS)
#ifdef ENABLE_FETCHLIMIT
		goto cleanup_fcount;
#else
		goto cleanup_domain;
#endif /* !ENABLE_FETCHLIMIT */

	fctx->rmessage = NULL;
	result = dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE,
				    &fctx->rmessage);

	if (result != ISC_R_SUCCESS)
		goto cleanup_qmessage;

	/*
	 * Compute an expiration time for the entire fetch.
	 */
	isc_interval_set(&interval, res->query_timeout, 0);
	iresult = isc_time_nowplusinterval(&fctx->expires, &interval);
	if (iresult != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_time_nowplusinterval: %s",
				 isc_result_totext(iresult));
		result = ISC_R_UNEXPECTED;
		goto cleanup_rmessage;
	}

	/*
	 * Default retry interval initialization.  We set the interval now
	 * mostly so it won't be uninitialized.  It will be set to the
	 * correct value before a query is issued.
	 */
	isc_interval_set(&fctx->interval, 2, 0);

	/*
	 * Create an inactive timer.  It will be made active when the fetch
	 * is actually started.
	 */
	fctx->timer = NULL;
	iresult = isc_timer_create(res->timermgr, isc_timertype_inactive,
				   NULL, NULL,
				   res->buckets[bucketnum].task, fctx_timeout,
				   fctx, &fctx->timer);
	if (iresult != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_timer_create: %s",
				 isc_result_totext(iresult));
		result = ISC_R_UNEXPECTED;
		goto cleanup_rmessage;
	}

	/*
	 * Attach to the view's cache and adb.
	 */
	fctx->cache = NULL;
	dns_db_attach(res->view->cachedb, &fctx->cache);
	fctx->adb = NULL;
	dns_adb_attach(res->view->adb, &fctx->adb);
	fctx->mctx = NULL;
	isc_mem_attach(mctx, &fctx->mctx);

	ISC_LIST_INIT(fctx->events);
	ISC_LINK_INIT(fctx, link);
	fctx->magic = FCTX_MAGIC;

	ISC_LIST_APPEND(res->buckets[bucketnum].fctxs, fctx, link);

	LOCK(&res->nlock);
	res->nfctx++;
	UNLOCK(&res->nlock);
	inc_stats(res, dns_resstatscounter_nfetch);

	*fctxp = fctx;

	return (ISC_R_SUCCESS);

 cleanup_rmessage:
	dns_message_destroy(&fctx->rmessage);

 cleanup_qmessage:
	dns_message_destroy(&fctx->qmessage);

#ifdef ENABLE_FETCHLIMIT
 cleanup_fcount:
	fcount_decr(fctx);
#endif /* ENABLE_FETCHLIMIT */

 cleanup_domain:
	if (dns_name_countlabels(&fctx->domain) > 0)
		dns_name_free(&fctx->domain, mctx);

 cleanup_nameservers:
	if (dns_rdataset_isassociated(&fctx->nameservers))
		dns_rdataset_disassociate(&fctx->nameservers);

 cleanup_name:
	dns_name_free(&fctx->name, mctx);

 cleanup_info:
	isc_mem_free(mctx, fctx->info);

 cleanup_counter:
	isc_counter_detach(&fctx->qc);

 cleanup_fetch:
	isc_mem_put(mctx, fctx, sizeof(*fctx));

	return (result);
}

/*
 * Handle Responses
 */
static inline isc_boolean_t
is_lame(fetchctx_t *fctx) {
	dns_message_t *message = fctx->rmessage;
	dns_name_t *name;
	dns_rdataset_t *rdataset;
	isc_result_t result;

	if (message->rcode != dns_rcode_noerror &&
	    message->rcode != dns_rcode_yxdomain &&
	    message->rcode != dns_rcode_nxdomain)
		return (ISC_FALSE);

	if (message->counts[DNS_SECTION_ANSWER] != 0)
		return (ISC_FALSE);

	if (message->counts[DNS_SECTION_AUTHORITY] == 0)
		return (ISC_FALSE);

	result = dns_message_firstname(message, DNS_SECTION_AUTHORITY);
	while (result == ISC_R_SUCCESS) {
		name = NULL;
		dns_message_currentname(message, DNS_SECTION_AUTHORITY, &name);
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link)) {
			dns_namereln_t namereln;
			int order;
			unsigned int labels;
			if (rdataset->type != dns_rdatatype_ns)
				continue;
			namereln = dns_name_fullcompare(name, &fctx->domain,
							&order, &labels);
			if (namereln == dns_namereln_equal &&
			    (message->flags & DNS_MESSAGEFLAG_AA) != 0)
				return (ISC_FALSE);
			if (namereln == dns_namereln_subdomain)
				return (ISC_FALSE);
			return (ISC_TRUE);
		}
		result = dns_message_nextname(message, DNS_SECTION_AUTHORITY);
	}

	return (ISC_FALSE);
}

static inline void
log_lame(fetchctx_t *fctx, dns_adbaddrinfo_t *addrinfo) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char domainbuf[DNS_NAME_FORMATSIZE];
	char addrbuf[ISC_SOCKADDR_FORMATSIZE];

	dns_name_format(&fctx->name, namebuf, sizeof(namebuf));
	dns_name_format(&fctx->domain, domainbuf, sizeof(domainbuf));
	isc_sockaddr_format(&addrinfo->sockaddr, addrbuf, sizeof(addrbuf));
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_LAME_SERVERS,
		      DNS_LOGMODULE_RESOLVER, ISC_LOG_INFO,
		      "lame server resolving '%s' (in '%s'?): %s",
		      namebuf, domainbuf, addrbuf);
}

static inline void
log_formerr(fetchctx_t *fctx, const char *format, ...) {
	char nsbuf[ISC_SOCKADDR_FORMATSIZE];
	char clbuf[ISC_SOCKADDR_FORMATSIZE];
	const char *clmsg = "";
	char msgbuf[2048];
	va_list args;

	va_start(args, format);
	vsnprintf(msgbuf, sizeof(msgbuf), format, args);
	va_end(args);

	isc_sockaddr_format(&fctx->addrinfo->sockaddr, nsbuf, sizeof(nsbuf));

	if (fctx->client != NULL) {
		clmsg = " for client ";
		isc_sockaddr_format(fctx->client, clbuf, sizeof(clbuf));
	} else {
		clbuf[0] = '\0';
	}

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
		      DNS_LOGMODULE_RESOLVER, ISC_LOG_NOTICE,
		      "DNS format error from %s resolving %s%s%s: %s",
		      nsbuf, fctx->info, clmsg, clbuf, msgbuf);
}

static isc_result_t
same_question(fetchctx_t *fctx) {
	isc_result_t result;
	dns_message_t *message = fctx->rmessage;
	dns_name_t *name;
	dns_rdataset_t *rdataset;

	/*
	 * Caller must be holding the fctx lock.
	 */

	/*
	 * XXXRTH  Currently we support only one question.
	 */
	if (ISC_UNLIKELY(message->counts[DNS_SECTION_QUESTION] == 0)) {
		if ((message->flags & DNS_MESSAGEFLAG_TC) != 0) {
			/*
			 * If TC=1 and the question section is empty, we
			 * accept the reply message as a truncated
			 * answer, to be retried over TCP.
			 *
			 * It is really a FORMERR condition, but this is
			 * a workaround to accept replies from some
			 * implementations.
			 *
			 * Because the question section matching is not
			 * performed, the worst that could happen is
			 * that an attacker who gets past the ID and
			 * source port checks can force the use of
			 * TCP. This is considered an acceptable risk.
			 */
			log_formerr(fctx,
				    "empty question section, "
				    "accepting it anyway as TC=1");
			return (ISC_R_SUCCESS);
		} else {
			log_formerr(fctx, "empty question section");
			return (DNS_R_FORMERR);
		}
	} else if (ISC_UNLIKELY(message->counts[DNS_SECTION_QUESTION] > 1)) {
		log_formerr(fctx, "too many questions");
		return (DNS_R_FORMERR);
	}

	result = dns_message_firstname(message, DNS_SECTION_QUESTION);
	if (result != ISC_R_SUCCESS)
		return (result);
	name = NULL;
	dns_message_currentname(message, DNS_SECTION_QUESTION, &name);
	rdataset = ISC_LIST_HEAD(name->list);
	INSIST(rdataset != NULL);
	INSIST(ISC_LIST_NEXT(rdataset, link) == NULL);

	if (fctx->type != rdataset->type ||
	    fctx->res->rdclass != rdataset->rdclass ||
	    !dns_name_equal(&fctx->name, name)) {
		char namebuf[DNS_NAME_FORMATSIZE];
		char classbuf[DNS_RDATACLASS_FORMATSIZE];
		char typebuf[DNS_RDATATYPE_FORMATSIZE];

		dns_name_format(name, namebuf, sizeof(namebuf));
		dns_rdataclass_format(rdataset->rdclass, classbuf,
				      sizeof(classbuf));
		dns_rdatatype_format(rdataset->type, typebuf, sizeof(typebuf));
		log_formerr(fctx, "question section mismatch: got %s/%s/%s",
			    namebuf, classbuf, typebuf);
		return (DNS_R_FORMERR);
	}

	return (ISC_R_SUCCESS);
}

static void
clone_results(fetchctx_t *fctx) {
	dns_fetchevent_t *event, *hevent;
	isc_result_t result;
	dns_name_t *name, *hname;

	FCTXTRACE("clone_results");

	/*
	 * Set up any other events to have the same data as the first
	 * event.
	 *
	 * Caller must be holding the appropriate lock.
	 */

	fctx->cloned = ISC_TRUE;
	hevent = ISC_LIST_HEAD(fctx->events);
	if (hevent == NULL)
		return;
	hname = dns_fixedname_name(&hevent->foundname);
	for (event = ISC_LIST_NEXT(hevent, ev_link);
	     event != NULL;
	     event = ISC_LIST_NEXT(event, ev_link)) {
		name = dns_fixedname_name(&event->foundname);
		result = dns_name_copy(hname, name, NULL);
		if (result != ISC_R_SUCCESS)
			event->result = result;
		else
			event->result = hevent->result;
		dns_db_attach(hevent->db, &event->db);
		dns_db_attachnode(hevent->db, hevent->node, &event->node);
		INSIST(hevent->rdataset != NULL);
		INSIST(event->rdataset != NULL);
		if (dns_rdataset_isassociated(hevent->rdataset))
			dns_rdataset_clone(hevent->rdataset, event->rdataset);
		INSIST(! (hevent->sigrdataset == NULL &&
			  event->sigrdataset != NULL));
		if (hevent->sigrdataset != NULL &&
		    dns_rdataset_isassociated(hevent->sigrdataset) &&
		    event->sigrdataset != NULL)
			dns_rdataset_clone(hevent->sigrdataset,
					   event->sigrdataset);
	}
}

#define CACHE(r)        (((r)->attributes & DNS_RDATASETATTR_CACHE) != 0)
#define ANSWER(r)       (((r)->attributes & DNS_RDATASETATTR_ANSWER) != 0)
#define ANSWERSIG(r)    (((r)->attributes & DNS_RDATASETATTR_ANSWERSIG) != 0)
#define EXTERNAL(r)     (((r)->attributes & DNS_RDATASETATTR_EXTERNAL) != 0)
#define CHAINING(r)     (((r)->attributes & DNS_RDATASETATTR_CHAINING) != 0)
#define CHASE(r)        (((r)->attributes & DNS_RDATASETATTR_CHASE) != 0)
#define CHECKNAMES(r)   (((r)->attributes & DNS_RDATASETATTR_CHECKNAMES) != 0)

/*
 * Destroy '*fctx' if it is ready to be destroyed (i.e., if it has
 * no references and is no longer waiting for any events).
 *
 * Requires:
 *      '*fctx' is shutting down.
 *
 * Returns:
 *	true if the resolver is exiting and this is the last fctx in the bucket.
 */
static isc_boolean_t
maybe_destroy(fetchctx_t *fctx, isc_boolean_t locked) {
	unsigned int bucketnum;
	isc_boolean_t bucket_empty = ISC_FALSE;
	dns_resolver_t *res = fctx->res;
	dns_validator_t *validator, *next_validator;
	isc_boolean_t dodestroy = ISC_FALSE;

	REQUIRE(SHUTTINGDOWN(fctx));

	bucketnum = fctx->bucketnum;
	if (!locked)
		LOCK(&res->buckets[bucketnum].lock);
	if (fctx->pending != 0 || fctx->nqueries != 0)
		goto unlock;

	for (validator = ISC_LIST_HEAD(fctx->validators);
	     validator != NULL; validator = next_validator) {
		next_validator = ISC_LIST_NEXT(validator, link);
		dns_validator_cancel(validator);
	}

	if (fctx->references == 0 && ISC_LIST_EMPTY(fctx->validators)) {
		bucket_empty = fctx_unlink(fctx);
		dodestroy = ISC_TRUE;
	}
 unlock:
	if (!locked)
		UNLOCK(&res->buckets[bucketnum].lock);
	if (dodestroy)
		fctx_destroy(fctx);
	return (bucket_empty);
}

/*
 * The validator has finished.
 */
static void
validated(isc_task_t *task, isc_event_t *event) {
	dns_adbaddrinfo_t *addrinfo;
	dns_dbnode_t *node = NULL;
	dns_dbnode_t *nsnode = NULL;
	dns_fetchevent_t *hevent;
	dns_name_t *name;
	dns_rdataset_t *ardataset = NULL;
	dns_rdataset_t *asigrdataset = NULL;
	dns_rdataset_t *rdataset;
	dns_rdataset_t *sigrdataset;
	dns_resolver_t *res;
	dns_valarg_t *valarg;
	dns_validatorevent_t *vevent;
	fetchctx_t *fctx;
	isc_boolean_t chaining;
	isc_boolean_t negative;
	isc_boolean_t sentresponse;
	isc_result_t eresult = ISC_R_SUCCESS;
	isc_result_t result = ISC_R_SUCCESS;
	isc_stdtime_t now;
	isc_uint32_t ttl;
	unsigned options;
	isc_uint32_t bucketnum;

	UNUSED(task); /* for now */

	REQUIRE(event->ev_type == DNS_EVENT_VALIDATORDONE);
	valarg = event->ev_arg;
	fctx = valarg->fctx;
	res = fctx->res;
	addrinfo = valarg->addrinfo;
	REQUIRE(VALID_FCTX(fctx));
	REQUIRE(!ISC_LIST_EMPTY(fctx->validators));

	vevent = (dns_validatorevent_t *)event;
	fctx->vresult = vevent->result;

	FCTXTRACE("received validation completion event");

	bucketnum = fctx->bucketnum;
	LOCK(&res->buckets[bucketnum].lock);

	ISC_LIST_UNLINK(fctx->validators, vevent->validator, link);
	fctx->validator = NULL;

	/*
	 * Destroy the validator early so that we can
	 * destroy the fctx if necessary.
	 */
	dns_validator_destroy(&vevent->validator);
	isc_mem_put(fctx->mctx, valarg, sizeof(*valarg));

	negative = ISC_TF(vevent->rdataset == NULL);

	sentresponse = ISC_TF((fctx->options & DNS_FETCHOPT_NOVALIDATE) != 0);

	/*
	 * If shutting down, ignore the results.  Check to see if we're
	 * done waiting for validator completions and ADB pending events; if
	 * so, destroy the fctx.
	 */
	if (SHUTTINGDOWN(fctx) && !sentresponse) {
		isc_boolean_t bucket_empty;
		bucket_empty = maybe_destroy(fctx, ISC_TRUE);
		UNLOCK(&res->buckets[bucketnum].lock);
		if (bucket_empty)
			empty_bucket(res);
		goto cleanup_event;
	}

	isc_stdtime_get(&now);

	/*
	 * If chaining, we need to make sure that the right result code is
	 * returned, and that the rdatasets are bound.
	 */
	if (vevent->result == ISC_R_SUCCESS &&
	    !negative &&
	    vevent->rdataset != NULL &&
	    CHAINING(vevent->rdataset))
	{
		if (vevent->rdataset->type == dns_rdatatype_cname)
			eresult = DNS_R_CNAME;
		else {
			INSIST(vevent->rdataset->type == dns_rdatatype_dname);
			eresult = DNS_R_DNAME;
		}
		chaining = ISC_TRUE;
	} else
		chaining = ISC_FALSE;

	/*
	 * Either we're not shutting down, or we are shutting down but want
	 * to cache the result anyway (if this was a validation started by
	 * a query with cd set)
	 */

	hevent = ISC_LIST_HEAD(fctx->events);
	if (hevent != NULL) {
		if (!negative && !chaining &&
		    (fctx->type == dns_rdatatype_any ||
		     fctx->type == dns_rdatatype_rrsig ||
		     fctx->type == dns_rdatatype_sig)) {
			/*
			 * Don't bind rdatasets; the caller
			 * will iterate the node.
			 */
		} else {
			ardataset = hevent->rdataset;
			asigrdataset = hevent->sigrdataset;
		}
	}

	if (vevent->result != ISC_R_SUCCESS) {
		FCTXTRACE("validation failed");
		inc_stats(res, dns_resstatscounter_valfail);
		fctx->valfail++;
		fctx->vresult = vevent->result;
		if (fctx->vresult != DNS_R_BROKENCHAIN) {
			result = ISC_R_NOTFOUND;
			if (vevent->rdataset != NULL)
				result = dns_db_findnode(fctx->cache,
							 vevent->name,
							 ISC_TRUE, &node);
			if (result == ISC_R_SUCCESS)
				(void)dns_db_deleterdataset(fctx->cache, node,
							     NULL,
							    vevent->type, 0);
			if (result == ISC_R_SUCCESS &&
			     vevent->sigrdataset != NULL)
				(void)dns_db_deleterdataset(fctx->cache, node,
							    NULL,
							    dns_rdatatype_rrsig,
							    vevent->type);
			if (result == ISC_R_SUCCESS)
				dns_db_detachnode(fctx->cache, &node);
		}
		if (fctx->vresult == DNS_R_BROKENCHAIN && !negative) {
			/*
			 * Cache the data as pending for later validation.
			 */
			result = ISC_R_NOTFOUND;
			if (vevent->rdataset != NULL)
				result = dns_db_findnode(fctx->cache,
							 vevent->name,
							 ISC_TRUE, &node);
			if (result == ISC_R_SUCCESS) {
				(void)dns_db_addrdataset(fctx->cache, node,
							 NULL, now,
							 vevent->rdataset, 0,
							 NULL);
			}
			if (result == ISC_R_SUCCESS &&
			    vevent->sigrdataset != NULL)
				(void)dns_db_addrdataset(fctx->cache, node,
							 NULL, now,
							 vevent->sigrdataset,
							 0, NULL);
			if (result == ISC_R_SUCCESS)
				dns_db_detachnode(fctx->cache, &node);
		}
		result = fctx->vresult;
		add_bad(fctx, addrinfo, result, badns_validation);
		isc_event_free(&event);
		UNLOCK(&res->buckets[bucketnum].lock);
		INSIST(fctx->validator == NULL);
		fctx->validator = ISC_LIST_HEAD(fctx->validators);
		if (fctx->validator != NULL)
			dns_validator_send(fctx->validator);
		else if (sentresponse)
			fctx_done(fctx, result, __LINE__); /* Locks bucket. */
		else if (result == DNS_R_BROKENCHAIN) {
			isc_result_t tresult;
			isc_time_t expire;
			isc_interval_t i;

			isc_interval_set(&i, DNS_BADCACHE_TTL(fctx), 0);
			tresult = isc_time_nowplusinterval(&expire, &i);
			if (negative &&
			    (fctx->type == dns_rdatatype_dnskey ||
			     fctx->type == dns_rdatatype_dlv ||
			     fctx->type == dns_rdatatype_ds) &&
			     tresult == ISC_R_SUCCESS)
				dns_resolver_addbadcache(res, &fctx->name,
							 fctx->type, &expire);
			fctx_done(fctx, result, __LINE__); /* Locks bucket. */
		} else
			fctx_try(fctx, ISC_TRUE, ISC_TRUE); /* Locks bucket. */
		return;
	}


	if (negative) {
		dns_rdatatype_t covers;
		FCTXTRACE("nonexistence validation OK");

		inc_stats(res, dns_resstatscounter_valnegsuccess);

		/*
		 * Cache DS NXDOMAIN seperately to other types.
		 */
		if (fctx->rmessage->rcode == dns_rcode_nxdomain &&
		    fctx->type != dns_rdatatype_ds)
			covers = dns_rdatatype_any;
		else
			covers = fctx->type;

		result = dns_db_findnode(fctx->cache, vevent->name, ISC_TRUE,
					 &node);
		if (result != ISC_R_SUCCESS)
			goto noanswer_response;

		/*
		 * If we are asking for a SOA record set the cache time
		 * to zero to facilitate locating the containing zone of
		 * a arbitrary zone.
		 */
		ttl = res->view->maxncachettl;
		if (fctx->type == dns_rdatatype_soa &&
		    covers == dns_rdatatype_any && res->zero_no_soa_ttl)
			ttl = 0;

		result = ncache_adderesult(fctx->rmessage, fctx->cache, node,
					   covers, now, ttl, vevent->optout,
					   vevent->secure, ardataset, &eresult);
		if (result != ISC_R_SUCCESS)
			goto noanswer_response;
		goto answer_response;
	} else
		inc_stats(res, dns_resstatscounter_valsuccess);

	FCTXTRACE("validation OK");

	if (vevent->proofs[DNS_VALIDATOR_NOQNAMEPROOF] != NULL) {
		result = dns_rdataset_addnoqname(vevent->rdataset,
				   vevent->proofs[DNS_VALIDATOR_NOQNAMEPROOF]);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		INSIST(vevent->sigrdataset != NULL);
		vevent->sigrdataset->ttl = vevent->rdataset->ttl;
		if (vevent->proofs[DNS_VALIDATOR_CLOSESTENCLOSER] != NULL) {
			result = dns_rdataset_addclosest(vevent->rdataset,
				 vevent->proofs[DNS_VALIDATOR_CLOSESTENCLOSER]);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
		}
	} else if (vevent->rdataset->trust == dns_trust_answer &&
		   vevent->rdataset->type != dns_rdatatype_rrsig)
	{
		isc_result_t tresult;
		dns_name_t *noqname = NULL;
		tresult = findnoqname(fctx, vevent->name,
				      vevent->rdataset->type, &noqname);
		if (tresult == ISC_R_SUCCESS && noqname != NULL) {
			tresult = dns_rdataset_addnoqname(vevent->rdataset,
							  noqname);
			RUNTIME_CHECK(tresult == ISC_R_SUCCESS);
		}
	}

	/*
	 * The data was already cached as pending data.
	 * Re-cache it as secure and bind the cached
	 * rdatasets to the first event on the fetch
	 * event list.
	 */
	result = dns_db_findnode(fctx->cache, vevent->name, ISC_TRUE, &node);
	if (result != ISC_R_SUCCESS)
		goto noanswer_response;

	options = 0;
	if ((fctx->options & DNS_FETCHOPT_PREFETCH) != 0)
		options = DNS_DBADD_PREFETCH;
	result = dns_db_addrdataset(fctx->cache, node, NULL, now,
				    vevent->rdataset, options, ardataset);
	if (result != ISC_R_SUCCESS &&
	    result != DNS_R_UNCHANGED)
		goto noanswer_response;
	if (ardataset != NULL && NEGATIVE(ardataset)) {
		if (NXDOMAIN(ardataset))
			eresult = DNS_R_NCACHENXDOMAIN;
		else
			eresult = DNS_R_NCACHENXRRSET;
	} else if (vevent->sigrdataset != NULL) {
		result = dns_db_addrdataset(fctx->cache, node, NULL, now,
					    vevent->sigrdataset, options,
					    asigrdataset);
		if (result != ISC_R_SUCCESS &&
		    result != DNS_R_UNCHANGED)
			goto noanswer_response;
	}

	if (sentresponse) {
		isc_boolean_t bucket_empty = ISC_FALSE;
		/*
		 * If we only deferred the destroy because we wanted to cache
		 * the data, destroy now.
		 */
		dns_db_detachnode(fctx->cache, &node);
		if (SHUTTINGDOWN(fctx))
			bucket_empty = maybe_destroy(fctx, ISC_TRUE);
		UNLOCK(&res->buckets[bucketnum].lock);
		if (bucket_empty)
			empty_bucket(res);
		goto cleanup_event;
	}

	if (!ISC_LIST_EMPTY(fctx->validators)) {
		INSIST(!negative);
		INSIST(fctx->type == dns_rdatatype_any ||
		       fctx->type == dns_rdatatype_rrsig ||
		       fctx->type == dns_rdatatype_sig);
		/*
		 * Don't send a response yet - we have
		 * more rdatasets that still need to
		 * be validated.
		 */
		dns_db_detachnode(fctx->cache, &node);
		UNLOCK(&res->buckets[bucketnum].lock);
		dns_validator_send(ISC_LIST_HEAD(fctx->validators));
		goto cleanup_event;
	}

 answer_response:
	/*
	 * Cache any NS/NSEC records that happened to be validated.
	 */
	result = dns_message_firstname(fctx->rmessage, DNS_SECTION_AUTHORITY);
	while (result == ISC_R_SUCCESS) {
		name = NULL;
		dns_message_currentname(fctx->rmessage, DNS_SECTION_AUTHORITY,
					&name);
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link)) {
			if ((rdataset->type != dns_rdatatype_ns &&
			     rdataset->type != dns_rdatatype_nsec) ||
			    rdataset->trust != dns_trust_secure)
				continue;
			for (sigrdataset = ISC_LIST_HEAD(name->list);
			     sigrdataset != NULL;
			     sigrdataset = ISC_LIST_NEXT(sigrdataset, link)) {
				if (sigrdataset->type != dns_rdatatype_rrsig ||
				    sigrdataset->covers != rdataset->type)
					continue;
				break;
			}
			if (sigrdataset == NULL ||
			    sigrdataset->trust != dns_trust_secure)
				continue;
			result = dns_db_findnode(fctx->cache, name, ISC_TRUE,
						 &nsnode);
			if (result != ISC_R_SUCCESS)
				continue;

			result = dns_db_addrdataset(fctx->cache, nsnode, NULL,
						    now, rdataset, 0, NULL);
			if (result == ISC_R_SUCCESS)
				result = dns_db_addrdataset(fctx->cache, nsnode,
							    NULL, now,
							    sigrdataset, 0,
							    NULL);
			dns_db_detachnode(fctx->cache, &nsnode);
			if (result != ISC_R_SUCCESS)
				continue;
		}
		result = dns_message_nextname(fctx->rmessage,
					      DNS_SECTION_AUTHORITY);
	}

	result = ISC_R_SUCCESS;

	/*
	 * Respond with an answer, positive or negative,
	 * as opposed to an error.  'node' must be non-NULL.
	 */

	fctx->attributes |= FCTX_ATTR_HAVEANSWER;

	if (hevent != NULL) {
		/*
		 * Negative results must be indicated in event->result.
		 */
		if (dns_rdataset_isassociated(hevent->rdataset) &&
		    NEGATIVE(hevent->rdataset)) {
			INSIST(eresult == DNS_R_NCACHENXDOMAIN ||
			       eresult == DNS_R_NCACHENXRRSET);
		}
		hevent->result = eresult;
		RUNTIME_CHECK(dns_name_copy(vevent->name,
			      dns_fixedname_name(&hevent->foundname), NULL)
			      == ISC_R_SUCCESS);
		dns_db_attach(fctx->cache, &hevent->db);
		dns_db_transfernode(fctx->cache, &node, &hevent->node);
		clone_results(fctx);
	}

 noanswer_response:
	if (node != NULL)
		dns_db_detachnode(fctx->cache, &node);

	UNLOCK(&res->buckets[bucketnum].lock);
	fctx_done(fctx, result, __LINE__); /* Locks bucket. */

 cleanup_event:
	INSIST(node == NULL);
	isc_event_free(&event);
}

static void
fctx_log(void *arg, int level, const char *fmt, ...) {
	char msgbuf[2048];
	va_list args;
	fetchctx_t *fctx = arg;

	va_start(args, fmt);
	vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
	va_end(args);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
		      DNS_LOGMODULE_RESOLVER, level,
		      "fctx %p(%s): %s", fctx, fctx->info, msgbuf);
}

static inline isc_result_t
findnoqname(fetchctx_t *fctx, dns_name_t *name, dns_rdatatype_t type,
	    dns_name_t **noqnamep)
{
	dns_rdataset_t *nrdataset, *next, *sigrdataset;
	dns_rdata_rrsig_t rrsig;
	isc_result_t result;
	unsigned int labels;
	dns_section_t section;
	dns_name_t *zonename;
	dns_fixedname_t fzonename;
	dns_name_t *closest;
	dns_fixedname_t fclosest;
	dns_name_t *nearest;
	dns_fixedname_t fnearest;
	dns_rdatatype_t found = dns_rdatatype_none;
	dns_name_t *noqname = NULL;

	FCTXTRACE("findnoqname");

	REQUIRE(noqnamep != NULL && *noqnamep == NULL);

	/*
	 * Find the SIG for this rdataset, if we have it.
	 */
	for (sigrdataset = ISC_LIST_HEAD(name->list);
	     sigrdataset != NULL;
	     sigrdataset = ISC_LIST_NEXT(sigrdataset, link)) {
		if (sigrdataset->type == dns_rdatatype_rrsig &&
		    sigrdataset->covers == type)
			break;
	}

	if (sigrdataset == NULL)
		return (ISC_R_NOTFOUND);

	labels = dns_name_countlabels(name);

	for (result = dns_rdataset_first(sigrdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(sigrdataset)) {
		dns_rdata_t rdata = DNS_RDATA_INIT;
		dns_rdataset_current(sigrdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &rrsig, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		/* Wildcard has rrsig.labels < labels - 1. */
		if (rrsig.labels + 1U >= labels)
			continue;
		break;
	}

	if (result == ISC_R_NOMORE)
		return (ISC_R_NOTFOUND);
	if (result != ISC_R_SUCCESS)
		return (result);

	dns_fixedname_init(&fzonename);
	zonename = dns_fixedname_name(&fzonename);
	dns_fixedname_init(&fclosest);
	closest = dns_fixedname_name(&fclosest);
	dns_fixedname_init(&fnearest);
	nearest = dns_fixedname_name(&fnearest);

#define NXND(x) ((x) == ISC_R_SUCCESS)

	section = DNS_SECTION_AUTHORITY;
	for (result = dns_message_firstname(fctx->rmessage, section);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(fctx->rmessage, section)) {
		dns_name_t *nsec = NULL;
		dns_message_currentname(fctx->rmessage, section, &nsec);
		for (nrdataset = ISC_LIST_HEAD(nsec->list);
		      nrdataset != NULL; nrdataset = next) {
			isc_boolean_t data = ISC_FALSE, exists = ISC_FALSE;
			isc_boolean_t optout = ISC_FALSE, unknown = ISC_FALSE;
			isc_boolean_t setclosest = ISC_FALSE;
			isc_boolean_t setnearest = ISC_FALSE;

			next = ISC_LIST_NEXT(nrdataset, link);
			if (nrdataset->type != dns_rdatatype_nsec &&
			    nrdataset->type != dns_rdatatype_nsec3)
				continue;

			if (nrdataset->type == dns_rdatatype_nsec &&
			    NXND(dns_nsec_noexistnodata(type, name, nsec,
							nrdataset, &exists,
							&data, NULL, fctx_log,
							fctx)))
			{
				if (!exists) {
					noqname = nsec;
					found = dns_rdatatype_nsec;
				}
			}

			if (nrdataset->type == dns_rdatatype_nsec3 &&
			    NXND(dns_nsec3_noexistnodata(type, name, nsec,
							 nrdataset, zonename,
							 &exists, &data,
							 &optout, &unknown,
							 &setclosest,
							 &setnearest,
							 closest, nearest,
							 fctx_log, fctx)))
			{
				if (!exists && setnearest) {
					noqname = nsec;
					found = dns_rdatatype_nsec3;
				}
			}
		}
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;
	if (noqname != NULL) {
		for (sigrdataset = ISC_LIST_HEAD(noqname->list);
		     sigrdataset != NULL;
		     sigrdataset = ISC_LIST_NEXT(sigrdataset, link)) {
			if (sigrdataset->type == dns_rdatatype_rrsig &&
			    sigrdataset->covers == found)
				break;
		}
		if (sigrdataset != NULL)
			*noqnamep = noqname;
	}
	return (result);
}

static inline isc_result_t
cache_name(fetchctx_t *fctx, dns_name_t *name, dns_adbaddrinfo_t *addrinfo,
	   isc_stdtime_t now)
{
	dns_rdataset_t *rdataset = NULL, *sigrdataset = NULL;
	dns_rdataset_t *addedrdataset = NULL;
	dns_rdataset_t *ardataset = NULL, *asigrdataset = NULL;
	dns_rdataset_t *valrdataset = NULL, *valsigrdataset = NULL;
	dns_dbnode_t *node = NULL, **anodep = NULL;
	dns_db_t **adbp = NULL;
	dns_name_t *aname = NULL;
	dns_resolver_t *res = fctx->res;
	isc_boolean_t need_validation = ISC_FALSE;
	isc_boolean_t secure_domain = ISC_FALSE;
	isc_boolean_t have_answer = ISC_FALSE;
	isc_result_t result, eresult = ISC_R_SUCCESS;
	dns_fetchevent_t *event = NULL;
	unsigned int options;
	isc_task_t *task;
	isc_boolean_t fail;
	unsigned int valoptions = 0;

	/*
	 * The appropriate bucket lock must be held.
	 */
	task = res->buckets[fctx->bucketnum].task;

	/*
	 * Is DNSSEC validation required for this name?
	 */
	if (res->view->enablevalidation) {
		result = dns_view_issecuredomain(res->view, name,
						 &secure_domain);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}

		if (!secure_domain && res->view->dlv != NULL) {
			valoptions = DNS_VALIDATOR_DLV;
			secure_domain = ISC_TRUE;
		}
	}

	if ((fctx->options & DNS_FETCHOPT_NOCDFLAG) != 0) {
		valoptions |= DNS_VALIDATOR_NOCDFLAG;
	}

	if ((fctx->options & DNS_FETCHOPT_NOVALIDATE) != 0) {
		need_validation = ISC_FALSE;
	} else {
		need_validation = secure_domain;
	}

	if (((name->attributes & DNS_NAMEATTR_ANSWER) != 0) &&
	    (!need_validation))
	{
		have_answer = ISC_TRUE;
		event = ISC_LIST_HEAD(fctx->events);
		if (event != NULL) {
			adbp = &event->db;
			aname = dns_fixedname_name(&event->foundname);
			result = dns_name_copy(name, aname, NULL);
			if (result != ISC_R_SUCCESS) {
				return (result);
			}
			anodep = &event->node;
			/*
			 * If this is an ANY, SIG or RRSIG query, we're not
			 * going to return any rdatasets, unless we encountered
			 * a CNAME or DNAME as "the answer".  In this case,
			 * we're going to return DNS_R_CNAME or DNS_R_DNAME
			 * and we must set up the rdatasets.
			 */
			if ((fctx->type != dns_rdatatype_any &&
			     fctx->type != dns_rdatatype_rrsig &&
			     fctx->type != dns_rdatatype_sig) ||
			    (name->attributes & DNS_NAMEATTR_CHAINING) != 0)
			{
				ardataset = event->rdataset;
				asigrdataset = event->sigrdataset;
			}
		}
	}

	/*
	 * Find or create the cache node.
	 */
	node = NULL;
	result = dns_db_findnode(fctx->cache, name, ISC_TRUE, &node);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	/*
	 * Cache or validate each cacheable rdataset.
	 */
	fail = ISC_TF((fctx->res->options & DNS_RESOLVER_CHECKNAMESFAIL) != 0);
	for (rdataset = ISC_LIST_HEAD(name->list);
	     rdataset != NULL;
	     rdataset = ISC_LIST_NEXT(rdataset, link))
	{
		if (!CACHE(rdataset)) {
			continue;
		}
		if (CHECKNAMES(rdataset)) {
			char namebuf[DNS_NAME_FORMATSIZE];
			char typebuf[DNS_RDATATYPE_FORMATSIZE];
			char classbuf[DNS_RDATATYPE_FORMATSIZE];

			dns_name_format(name, namebuf, sizeof(namebuf));
			dns_rdatatype_format(rdataset->type, typebuf,
					     sizeof(typebuf));
			dns_rdataclass_format(rdataset->rdclass, classbuf,
					      sizeof(classbuf));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
				      DNS_LOGMODULE_RESOLVER, ISC_LOG_NOTICE,
				      "check-names %s %s/%s/%s",
				      fail ? "failure" : "warning",
				      namebuf, typebuf, classbuf);
			if (fail) {
				if (ANSWER(rdataset)) {
					dns_db_detachnode(fctx->cache, &node);
					return (DNS_R_BADNAME);
				}
				continue;
			}
		}

		/*
		 * Enforce the configure maximum cache TTL.
		 */
		if (rdataset->ttl > res->view->maxcachettl) {
			rdataset->ttl = res->view->maxcachettl;
		}

		/*
		 * Mark the rdataset as being prefetch eligible.
		 */
		if (rdataset->ttl > fctx->res->view->prefetch_eligible) {
			rdataset->attributes |= DNS_RDATASETATTR_PREFETCH;
		}

		/*
		 * Find the SIG for this rdataset, if we have it.
		 */
		for (sigrdataset = ISC_LIST_HEAD(name->list);
		     sigrdataset != NULL;
		     sigrdataset = ISC_LIST_NEXT(sigrdataset, link))
		{
			if (sigrdataset->type == dns_rdatatype_rrsig &&
			    sigrdataset->covers == rdataset->type)
			{
				break;
			}
		}

		/*
		 * If this RRset is in a secure domain, is in bailiwick,
		 * and is not glue, attempt DNSSEC validation.	(We do not
		 * attempt to validate glue or out-of-bailiwick data--even
		 * though there might be some performance benefit to doing
		 * so--because it makes it simpler and safer to ensure that
		 * records from a secure domain are only cached if validated
		 * within the context of a query to the domain that owns
		 * them.)
		 */
		if (secure_domain && rdataset->trust != dns_trust_glue &&
		    !EXTERNAL(rdataset))
		{
			dns_trust_t trust;

			/*
			 * RRSIGs are validated as part of validating the
			 * type they cover.
			 */
			if (rdataset->type == dns_rdatatype_rrsig) {
				continue;
			}

			if (sigrdataset == NULL && need_validation &&
			    !ANSWER(rdataset))
			{
				/*
				 * Ignore unrelated non-answer
				 * rdatasets that are missing signatures.
				 */
				continue;
			}

			/*
			 * Normalize the rdataset and sigrdataset TTLs.
			 */
			if (sigrdataset != NULL) {
				rdataset->ttl = ISC_MIN(rdataset->ttl,
							sigrdataset->ttl);
				sigrdataset->ttl = rdataset->ttl;
			}

			/*
			 * Mark the rdataset as being prefetch eligible.
			 */
			if (rdataset->ttl > fctx->res->view->prefetch_eligible)
			{
				rdataset->attributes |=
					DNS_RDATASETATTR_PREFETCH;
			}

			/*
			 * Cache this rdataset/sigrdataset pair as
			 * pending data.  Track whether it was additional
			 * or not. If this was a priming query, additional
			 * should be cached as glue.
			 */
			if (rdataset->trust == dns_trust_additional) {
				trust = dns_trust_pending_additional;
			} else {
				trust = dns_trust_pending_answer;
			}

			rdataset->trust = trust;
			if (sigrdataset != NULL) {
				sigrdataset->trust = trust;
			}
			if (!need_validation || !ANSWER(rdataset)) {
				options = 0;
				if (ANSWER(rdataset) &&
				   rdataset->type != dns_rdatatype_rrsig)
				{
					isc_result_t tresult;
					dns_name_t *noqname = NULL;
					tresult = findnoqname(fctx, name,
							      rdataset->type,
							      &noqname);
					if (tresult == ISC_R_SUCCESS &&
					    noqname != NULL)
					{
						(void) dns_rdataset_addnoqname(
							    rdataset, noqname);
					}
				}
				if ((fctx->options &
				     DNS_FETCHOPT_PREFETCH) != 0)
				{
					options = DNS_DBADD_PREFETCH;
				}
				if ((fctx->options &
				     DNS_FETCHOPT_NOCACHED) != 0)
				{
					options |= DNS_DBADD_FORCE;
				}
				addedrdataset = ardataset;
				result = dns_db_addrdataset(fctx->cache, node,
							    NULL, now, rdataset,
							    options,
							    addedrdataset);
				if (result == DNS_R_UNCHANGED) {
					result = ISC_R_SUCCESS;
					if (!need_validation &&
					    ardataset != NULL &&
					    NEGATIVE(ardataset))
					{
						/*
						 * The answer in the cache is
						 * better than the answer we
						 * found, and is a negative
						 * cache entry, so we must set
						 * eresult appropriately.
						 */
						if (NXDOMAIN(ardataset)) {
							eresult =
							   DNS_R_NCACHENXDOMAIN;
						} else {
							eresult =
							   DNS_R_NCACHENXRRSET;
						}
						/*
						 * We have a negative response
						 * from the cache so don't
						 * attempt to add the RRSIG
						 * rrset.
						 */
						continue;
					}
				}
				if (result != ISC_R_SUCCESS) {
					break;
				}
				if (sigrdataset != NULL) {
					addedrdataset = asigrdataset;
					result = dns_db_addrdataset(fctx->cache,
								node, NULL, now,
								sigrdataset,
								options,
								addedrdataset);
					if (result == DNS_R_UNCHANGED) {
						result = ISC_R_SUCCESS;
					}
					if (result != ISC_R_SUCCESS) {
						break;
					}
				} else if (!ANSWER(rdataset)) {
					continue;
				}
			}

			if (ANSWER(rdataset) && need_validation) {
				if (fctx->type != dns_rdatatype_any &&
				    fctx->type != dns_rdatatype_rrsig &&
				    fctx->type != dns_rdatatype_sig)
				{
					/*
					 * This is The Answer.  We will
					 * validate it, but first we cache
					 * the rest of the response - it may
					 * contain useful keys.
					 */
					INSIST(valrdataset == NULL &&
					       valsigrdataset == NULL);
					valrdataset = rdataset;
					valsigrdataset = sigrdataset;
				} else {
					/*
					 * This is one of (potentially)
					 * multiple answers to an ANY
					 * or SIG query.  To keep things
					 * simple, we just start the
					 * validator right away rather
					 * than caching first and
					 * having to remember which
					 * rdatasets needed validation.
					 */
					result = valcreate(fctx, addrinfo,
							   name, rdataset->type,
							   rdataset,
							   sigrdataset,
							   valoptions, task);
				}
			} else if (CHAINING(rdataset)) {
				if (rdataset->type == dns_rdatatype_cname) {
					eresult = DNS_R_CNAME;
				} else {
					INSIST(rdataset->type ==
					       dns_rdatatype_dname);
					eresult = DNS_R_DNAME;
				}
			}
		} else if (!EXTERNAL(rdataset)) {
			/*
			 * It's OK to cache this rdataset now.
			 */
			if (ANSWER(rdataset)) {
				addedrdataset = ardataset;
			} else if (ANSWERSIG(rdataset)) {
				addedrdataset = asigrdataset;
			} else {
				addedrdataset = NULL;
			}
			if (CHAINING(rdataset)) {
				if (rdataset->type == dns_rdatatype_cname) {
					eresult = DNS_R_CNAME;
				} else {
					INSIST(rdataset->type ==
					       dns_rdatatype_dname);
					eresult = DNS_R_DNAME;
				}
			}
			if (rdataset->trust == dns_trust_glue &&
			    (rdataset->type == dns_rdatatype_ns ||
			     (rdataset->type == dns_rdatatype_rrsig &&
			      rdataset->covers == dns_rdatatype_ns)))
			{
				/*
				 * If the trust level is 'dns_trust_glue'
				 * then we are adding data from a referral
				 * we got while executing the search algorithm.
				 * New referral data always takes precedence
				 * over the existing cache contents.
				 */
				options = DNS_DBADD_FORCE;
			} else if ((fctx->options & DNS_FETCHOPT_PREFETCH) != 0)
			{
				options = DNS_DBADD_PREFETCH;
			} else {
				options = 0;
			}

			if (ANSWER(rdataset) &&
			   rdataset->type != dns_rdatatype_rrsig)
			{
				isc_result_t tresult;
				dns_name_t *noqname = NULL;
				tresult = findnoqname(fctx, name,
						      rdataset->type, &noqname);
				if (tresult == ISC_R_SUCCESS &&
				    noqname != NULL)
				{
					(void) dns_rdataset_addnoqname(
						       rdataset, noqname);
				}
			}

			/*
			 * Now we can add the rdataset.
			 */
			result = dns_db_addrdataset(fctx->cache,
						    node, NULL, now,
						    rdataset,
						    options,
						    addedrdataset);

			if (result == DNS_R_UNCHANGED) {
				if (ANSWER(rdataset) &&
				    ardataset != NULL &&
				    NEGATIVE(ardataset))
				{
					/*
					 * The answer in the cache is better
					 * than the answer we found, and is
					 * a negative cache entry, so we
					 * must set eresult appropriately.
					 */
					if (NXDOMAIN(ardataset)) {
						eresult = DNS_R_NCACHENXDOMAIN;
					} else {
						eresult = DNS_R_NCACHENXRRSET;
					}
				}
				result = ISC_R_SUCCESS;
			} else if (result != ISC_R_SUCCESS) {
				break;
			}
		}
	}

	if (valrdataset != NULL) {
		dns_rdatatype_t vtype = fctx->type;
		if (CHAINING(valrdataset)) {
			if (valrdataset->type == dns_rdatatype_cname) {
				vtype = dns_rdatatype_cname;
			} else {
				vtype = dns_rdatatype_dname;
			}
		}
		result = valcreate(fctx, addrinfo, name, vtype, valrdataset,
				   valsigrdataset, valoptions, task);
	}

	if (result == ISC_R_SUCCESS && have_answer) {
		fctx->attributes |= FCTX_ATTR_HAVEANSWER;
		if (event != NULL) {
			/*
			 * Negative results must be indicated in event->result.
			 */
			if (dns_rdataset_isassociated(event->rdataset) &&
			    NEGATIVE(event->rdataset))
			{
				INSIST(eresult == DNS_R_NCACHENXDOMAIN ||
				       eresult == DNS_R_NCACHENXRRSET);
			}
			event->result = eresult;
			if (adbp != NULL && *adbp != NULL) {
				if (anodep != NULL && *anodep != NULL) {
					dns_db_detachnode(*adbp, anodep);
				}
				dns_db_detach(adbp);
			}
			dns_db_attach(fctx->cache, adbp);
			dns_db_transfernode(fctx->cache, &node, anodep);
			clone_results(fctx);
		}
	}

	if (node != NULL) {
		dns_db_detachnode(fctx->cache, &node);
	}

	return (result);
}

static inline isc_result_t
cache_message(fetchctx_t *fctx, dns_adbaddrinfo_t *addrinfo, isc_stdtime_t now)
{
	isc_result_t result;
	dns_section_t section;
	dns_name_t *name;

	FCTXTRACE("cache_message");

	fctx->attributes &= ~FCTX_ATTR_WANTCACHE;

	LOCK(&fctx->res->buckets[fctx->bucketnum].lock);

	for (section = DNS_SECTION_ANSWER;
	     section <= DNS_SECTION_ADDITIONAL;
	     section++) {
		result = dns_message_firstname(fctx->rmessage, section);
		while (result == ISC_R_SUCCESS) {
			name = NULL;
			dns_message_currentname(fctx->rmessage, section,
						&name);
			if ((name->attributes & DNS_NAMEATTR_CACHE) != 0) {
				result = cache_name(fctx, name, addrinfo, now);
				if (result != ISC_R_SUCCESS)
					break;
			}
			result = dns_message_nextname(fctx->rmessage, section);
		}
		if (result != ISC_R_NOMORE)
			break;
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;

	UNLOCK(&fctx->res->buckets[fctx->bucketnum].lock);

	return (result);
}

/*
 * Do what dns_ncache_addoptout() does, and then compute an appropriate eresult.
 */
static isc_result_t
ncache_adderesult(dns_message_t *message, dns_db_t *cache, dns_dbnode_t *node,
		  dns_rdatatype_t covers, isc_stdtime_t now, dns_ttl_t maxttl,
		  isc_boolean_t optout, isc_boolean_t secure,
		  dns_rdataset_t *ardataset, isc_result_t *eresultp)
{
	isc_result_t result;
	dns_rdataset_t rdataset;

	if (ardataset == NULL) {
		dns_rdataset_init(&rdataset);
		ardataset = &rdataset;
	}
	if (secure)
		result = dns_ncache_addoptout(message, cache, node, covers,
					      now, maxttl, optout, ardataset);
	else
		result = dns_ncache_add(message, cache, node, covers, now,
					maxttl, ardataset);
	if (result == DNS_R_UNCHANGED || result == ISC_R_SUCCESS) {
		/*
		 * If the cache now contains a negative entry and we
		 * care about whether it is DNS_R_NCACHENXDOMAIN or
		 * DNS_R_NCACHENXRRSET then extract it.
		 */
		if (NEGATIVE(ardataset)) {
			/*
			 * The cache data is a negative cache entry.
			 */
			if (NXDOMAIN(ardataset))
				*eresultp = DNS_R_NCACHENXDOMAIN;
			else
				*eresultp = DNS_R_NCACHENXRRSET;
		} else {
			/*
			 * Either we don't care about the nature of the
			 * cache rdataset (because no fetch is interested
			 * in the outcome), or the cache rdataset is not
			 * a negative cache entry.  Whichever case it is,
			 * we can return success.
			 *
			 * XXXRTH  There's a CNAME/DNAME problem here.
			 */
			*eresultp = ISC_R_SUCCESS;
		}
		result = ISC_R_SUCCESS;
	}
	if (ardataset == &rdataset && dns_rdataset_isassociated(ardataset))
		dns_rdataset_disassociate(ardataset);

	return (result);
}

static inline isc_result_t
ncache_message(fetchctx_t *fctx, dns_adbaddrinfo_t *addrinfo,
	       dns_rdatatype_t covers, isc_stdtime_t now)
{
	isc_result_t result, eresult;
	dns_name_t *name;
	dns_resolver_t *res;
	dns_db_t **adbp;
	dns_dbnode_t *node, **anodep;
	dns_rdataset_t *ardataset;
	isc_boolean_t need_validation, secure_domain;
	dns_name_t *aname;
	dns_fetchevent_t *event;
	isc_uint32_t ttl;
	unsigned int valoptions = 0;

	FCTXTRACE("ncache_message");

	fctx->attributes &= ~FCTX_ATTR_WANTNCACHE;

	res = fctx->res;
	need_validation = ISC_FALSE;
	POST(need_validation);
	secure_domain = ISC_FALSE;
	eresult = ISC_R_SUCCESS;
	name = &fctx->name;
	node = NULL;

	/*
	 * XXXMPA remove when we follow cnames and adjust the setting
	 * of FCTX_ATTR_WANTNCACHE in noanswer_response().
	 */
	INSIST(fctx->rmessage->counts[DNS_SECTION_ANSWER] == 0);

	/*
	 * Is DNSSEC validation required for this name?
	 */
	if (fctx->res->view->enablevalidation) {
		result = dns_view_issecuredomain(res->view, name,
						 &secure_domain);
		if (result != ISC_R_SUCCESS)
			return (result);

		if (!secure_domain && res->view->dlv != NULL) {
			valoptions = DNS_VALIDATOR_DLV;
			secure_domain = ISC_TRUE;
		}
	}

	if ((fctx->options & DNS_FETCHOPT_NOCDFLAG) != 0)
		valoptions |= DNS_VALIDATOR_NOCDFLAG;

	if ((fctx->options & DNS_FETCHOPT_NOVALIDATE) != 0)
		need_validation = ISC_FALSE;
	else
		need_validation = secure_domain;

	if (secure_domain) {
		/*
		 * Mark all rdatasets as pending.
		 */
		dns_rdataset_t *trdataset;
		dns_name_t *tname;

		result = dns_message_firstname(fctx->rmessage,
					       DNS_SECTION_AUTHORITY);
		while (result == ISC_R_SUCCESS) {
			tname = NULL;
			dns_message_currentname(fctx->rmessage,
						DNS_SECTION_AUTHORITY,
						&tname);
			for (trdataset = ISC_LIST_HEAD(tname->list);
			     trdataset != NULL;
			     trdataset = ISC_LIST_NEXT(trdataset, link))
				trdataset->trust = dns_trust_pending_answer;
			result = dns_message_nextname(fctx->rmessage,
						      DNS_SECTION_AUTHORITY);
		}
		if (result != ISC_R_NOMORE)
			return (result);

	}

	if (need_validation) {
		/*
		 * Do negative response validation.
		 */
		result = valcreate(fctx, addrinfo, name, fctx->type,
				   NULL, NULL, valoptions,
				   res->buckets[fctx->bucketnum].task);
		/*
		 * If validation is necessary, return now.  Otherwise continue
		 * to process the message, letting the validation complete
		 * in its own good time.
		 */
		return (result);
	}

	LOCK(&res->buckets[fctx->bucketnum].lock);

	adbp = NULL;
	aname = NULL;
	anodep = NULL;
	ardataset = NULL;
	if (!HAVE_ANSWER(fctx)) {
		event = ISC_LIST_HEAD(fctx->events);
		if (event != NULL) {
			adbp = &event->db;
			aname = dns_fixedname_name(&event->foundname);
			result = dns_name_copy(name, aname, NULL);
			if (result != ISC_R_SUCCESS)
				goto unlock;
			anodep = &event->node;
			ardataset = event->rdataset;
		}
	} else
		event = NULL;

	result = dns_db_findnode(fctx->cache, name, ISC_TRUE, &node);
	if (result != ISC_R_SUCCESS)
		goto unlock;

	/*
	 * If we are asking for a SOA record set the cache time
	 * to zero to facilitate locating the containing zone of
	 * a arbitrary zone.
	 */
	ttl = fctx->res->view->maxncachettl;
	if (fctx->type == dns_rdatatype_soa &&
	    covers == dns_rdatatype_any &&
	    fctx->res->zero_no_soa_ttl)
		ttl = 0;

	result = ncache_adderesult(fctx->rmessage, fctx->cache, node,
				   covers, now, ttl, ISC_FALSE,
				   ISC_FALSE, ardataset, &eresult);
	if (result != ISC_R_SUCCESS)
		goto unlock;

	if (!HAVE_ANSWER(fctx)) {
		fctx->attributes |= FCTX_ATTR_HAVEANSWER;
		if (event != NULL) {
			event->result = eresult;
			if (adbp != NULL && *adbp != NULL) {
				if (anodep != NULL && *anodep != NULL)
					dns_db_detachnode(*adbp, anodep);
				dns_db_detach(adbp);
			}
			dns_db_attach(fctx->cache, adbp);
			dns_db_transfernode(fctx->cache, &node, anodep);
			clone_results(fctx);
		}
	}

 unlock:
	UNLOCK(&res->buckets[fctx->bucketnum].lock);

	if (node != NULL)
		dns_db_detachnode(fctx->cache, &node);

	return (result);
}

static inline void
mark_related(dns_name_t *name, dns_rdataset_t *rdataset,
	     isc_boolean_t external, isc_boolean_t gluing)
{
	name->attributes |= DNS_NAMEATTR_CACHE;
	if (gluing) {
		rdataset->trust = dns_trust_glue;
		/*
		 * Glue with 0 TTL causes problems.  We force the TTL to
		 * 1 second to prevent this.
		 */
		if (rdataset->ttl == 0)
			rdataset->ttl = 1;
	} else
		rdataset->trust = dns_trust_additional;
	/*
	 * Avoid infinite loops by only marking new rdatasets.
	 */
	if (!CACHE(rdataset)) {
		name->attributes |= DNS_NAMEATTR_CHASE;
		rdataset->attributes |= DNS_RDATASETATTR_CHASE;
	}
	rdataset->attributes |= DNS_RDATASETATTR_CACHE;
	if (external)
		rdataset->attributes |= DNS_RDATASETATTR_EXTERNAL;
}

static isc_result_t
check_section(void *arg, dns_name_t *addname, dns_rdatatype_t type,
	      dns_section_t section)
{
	fetchctx_t *fctx = arg;
	isc_result_t result;
	dns_name_t *name = NULL;
	dns_rdataset_t *rdataset = NULL;
	isc_boolean_t external;
	dns_rdatatype_t rtype;
	isc_boolean_t gluing;

	REQUIRE(VALID_FCTX(fctx));

#if CHECK_FOR_GLUE_IN_ANSWER
	if (section == DNS_SECTION_ANSWER && type != dns_rdatatype_a)
		return (ISC_R_SUCCESS);
#endif

	gluing = ISC_TF(GLUING(fctx) ||
			(fctx->type == dns_rdatatype_ns &&
			 dns_name_equal(&fctx->name, dns_rootname)));

	result = dns_message_findname(fctx->rmessage, section, addname,
				      dns_rdatatype_any, 0, &name, NULL);
	if (result == ISC_R_SUCCESS) {
		external = ISC_TF(!dns_name_issubdomain(name, &fctx->domain));
		if (type == dns_rdatatype_a) {
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link)) {
				if (rdataset->type == dns_rdatatype_rrsig)
					rtype = rdataset->covers;
				else
					rtype = rdataset->type;
				if (rtype == dns_rdatatype_a ||
				    rtype == dns_rdatatype_aaaa)
					mark_related(name, rdataset, external,
						     gluing);
			}
		} else {
			result = dns_message_findtype(name, type, 0,
						      &rdataset);
			if (result == ISC_R_SUCCESS) {
				mark_related(name, rdataset, external, gluing);
				/*
				 * Do we have its SIG too?
				 */
				rdataset = NULL;
				result = dns_message_findtype(name,
						      dns_rdatatype_rrsig,
						      type, &rdataset);
				if (result == ISC_R_SUCCESS)
					mark_related(name, rdataset, external,
						     gluing);
			}
		}
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
check_related(void *arg, dns_name_t *addname, dns_rdatatype_t type) {
	return (check_section(arg, addname, type, DNS_SECTION_ADDITIONAL));
}

#ifndef CHECK_FOR_GLUE_IN_ANSWER
#define CHECK_FOR_GLUE_IN_ANSWER 0
#endif
#if CHECK_FOR_GLUE_IN_ANSWER
static isc_result_t
check_answer(void *arg, dns_name_t *addname, dns_rdatatype_t type) {
	return (check_section(arg, addname, type, DNS_SECTION_ANSWER));
}
#endif

static void
chase_additional(fetchctx_t *fctx) {
	isc_boolean_t rescan;
	dns_section_t section = DNS_SECTION_ADDITIONAL;
	isc_result_t result;

 again:
	rescan = ISC_FALSE;

	for (result = dns_message_firstname(fctx->rmessage, section);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(fctx->rmessage, section)) {
		dns_name_t *name = NULL;
		dns_rdataset_t *rdataset;
		dns_message_currentname(fctx->rmessage, DNS_SECTION_ADDITIONAL,
					&name);
		if ((name->attributes & DNS_NAMEATTR_CHASE) == 0)
			continue;
		name->attributes &= ~DNS_NAMEATTR_CHASE;
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link)) {
			if (CHASE(rdataset)) {
				rdataset->attributes &= ~DNS_RDATASETATTR_CHASE;
				(void)dns_rdataset_additionaldata(rdataset,
								  check_related,
								  fctx);
				rescan = ISC_TRUE;
			}
		}
	}
	if (rescan)
		goto again;
}

static isc_boolean_t
is_answeraddress_allowed(dns_view_t *view, dns_name_t *name,
			 dns_rdataset_t *rdataset)
{
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	struct in_addr ina;
	struct in6_addr in6a;
	isc_netaddr_t netaddr;
	char addrbuf[ISC_NETADDR_FORMATSIZE];
	char namebuf[DNS_NAME_FORMATSIZE];
	char classbuf[64];
	char typebuf[64];
	int match;

	/* By default, we allow any addresses. */
	if (view->denyansweracl == NULL)
		return (ISC_TRUE);

	/*
	 * If the owner name matches one in the exclusion list, either exactly
	 * or partially, allow it.
	 */
	if (view->answeracl_exclude != NULL) {
		dns_rbtnode_t *node = NULL;

		result = dns_rbt_findnode(view->answeracl_exclude, name, NULL,
					  &node, NULL, 0, NULL, NULL);

		if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH)
			return (ISC_TRUE);
	}

	/*
	 * Otherwise, search the filter list for a match for each address
	 * record.  If a match is found, the address should be filtered,
	 * so should the entire answer.
	 */
	for (result = dns_rdataset_first(rdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(rdataset)) {
		dns_rdata_reset(&rdata);
		dns_rdataset_current(rdataset, &rdata);
		if (rdataset->type == dns_rdatatype_a) {
			INSIST(rdata.length == sizeof(ina.s_addr));
			memmove(&ina.s_addr, rdata.data, sizeof(ina.s_addr));
			isc_netaddr_fromin(&netaddr, &ina);
		} else {
			INSIST(rdata.length == sizeof(in6a.s6_addr));
			memmove(in6a.s6_addr, rdata.data, sizeof(in6a.s6_addr));
			isc_netaddr_fromin6(&netaddr, &in6a);
		}

		result = dns_acl_match(&netaddr, NULL, view->denyansweracl,
				       &view->aclenv, &match, NULL);

		if (result == ISC_R_SUCCESS && match > 0) {
			isc_netaddr_format(&netaddr, addrbuf, sizeof(addrbuf));
			dns_name_format(name, namebuf, sizeof(namebuf));
			dns_rdatatype_format(rdataset->type, typebuf,
					     sizeof(typebuf));
			dns_rdataclass_format(rdataset->rdclass, classbuf,
					      sizeof(classbuf));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
				      DNS_LOGMODULE_RESOLVER, ISC_LOG_NOTICE,
				      "answer address %s denied for %s/%s/%s",
				      addrbuf, namebuf, typebuf, classbuf);
			return (ISC_FALSE);
		}
	}

	return (ISC_TRUE);
}

static isc_boolean_t
is_answertarget_allowed(fetchctx_t *fctx, dns_name_t *qname, dns_name_t *rname,
			dns_rdataset_t *rdataset, isc_boolean_t *chainingp)
{
	isc_result_t result;
	dns_rbtnode_t *node = NULL;
	char qnamebuf[DNS_NAME_FORMATSIZE];
	char tnamebuf[DNS_NAME_FORMATSIZE];
	char classbuf[64];
	char typebuf[64];
	dns_name_t *tname = NULL;
	dns_rdata_cname_t cname;
	dns_rdata_dname_t dname;
	dns_view_t *view = fctx->res->view;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	unsigned int nlabels;
	dns_fixedname_t fixed;
	dns_name_t prefix;

	REQUIRE(rdataset != NULL);
	REQUIRE(rdataset->type == dns_rdatatype_cname ||
		rdataset->type == dns_rdatatype_dname);

	/*
	 * By default, we allow any target name.
	 * If newqname != NULL we also need to extract the newqname.
	 */
	if (chainingp == NULL && view->denyanswernames == NULL)
		return (ISC_TRUE);

	result = dns_rdataset_first(rdataset);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	dns_rdataset_current(rdataset, &rdata);
	switch (rdataset->type) {
	case dns_rdatatype_cname:
		result = dns_rdata_tostruct(&rdata, &cname, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		tname = &cname.cname;
		break;
	case dns_rdatatype_dname:
		result = dns_rdata_tostruct(&rdata, &dname, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		dns_name_init(&prefix, NULL);
		dns_fixedname_init(&fixed);
		tname = dns_fixedname_name(&fixed);
		nlabels = dns_name_countlabels(qname) -
			  dns_name_countlabels(rname);
		dns_name_split(qname, nlabels, &prefix, NULL);
		result = dns_name_concatenate(&prefix, &dname.dname, tname,
					      NULL);
		if (result == DNS_R_NAMETOOLONG)
			return (ISC_TRUE);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		break;
	default:
		INSIST(0);
	}

	if (chainingp != NULL)
		*chainingp = ISC_TRUE;

	if (view->denyanswernames == NULL)
		return (ISC_TRUE);

	/*
	 * If the owner name matches one in the exclusion list, either exactly
	 * or partially, allow it.
	 */
	if (view->answernames_exclude != NULL) {
		result = dns_rbt_findnode(view->answernames_exclude, qname,
					  NULL, &node, NULL, 0, NULL, NULL);
		if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH)
			return (ISC_TRUE);
	}

	/*
	 * If the target name is a subdomain of the search domain, allow it.
	 */
	if (dns_name_issubdomain(tname, &fctx->domain))
		return (ISC_TRUE);

	/*
	 * Otherwise, apply filters.
	 */
	result = dns_rbt_findnode(view->denyanswernames, tname, NULL, &node,
				  NULL, 0, NULL, NULL);
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH) {
		dns_name_format(qname, qnamebuf, sizeof(qnamebuf));
		dns_name_format(tname, tnamebuf, sizeof(tnamebuf));
		dns_rdatatype_format(rdataset->type, typebuf, sizeof(typebuf));
		dns_rdataclass_format(view->rdclass, classbuf,
				      sizeof(classbuf));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
			      DNS_LOGMODULE_RESOLVER, ISC_LOG_NOTICE,
			      "%s target %s denied for %s/%s",
			      typebuf, tnamebuf, qnamebuf, classbuf);
		return (ISC_FALSE);
	}

	return (ISC_TRUE);
}

static void
trim_ns_ttl(fetchctx_t *fctx, dns_name_t *name, dns_rdataset_t *rdataset) {
	char ns_namebuf[DNS_NAME_FORMATSIZE];
	char namebuf[DNS_NAME_FORMATSIZE];
	char tbuf[DNS_RDATATYPE_FORMATSIZE];

	if (fctx->ns_ttl_ok && rdataset->ttl > fctx->ns_ttl) {
		dns_name_format(name, ns_namebuf, sizeof(ns_namebuf));
		dns_name_format(&fctx->name, namebuf, sizeof(namebuf));
		dns_rdatatype_format(fctx->type, tbuf, sizeof(tbuf));

		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
			      DNS_LOGMODULE_RESOLVER, ISC_LOG_DEBUG(10),
			      "fctx %p: trimming ttl of %s/NS for %s/%s: "
			      "%u -> %u", fctx, ns_namebuf, namebuf, tbuf,
			      rdataset->ttl, fctx->ns_ttl);
		rdataset->ttl = fctx->ns_ttl;
	}
}

/*
 * Handle a no-answer response (NXDOMAIN, NXRRSET, or referral).
 * If look_in_options has LOOK_FOR_NS_IN_ANSWER then we look in the answer
 * section for the NS RRset if the query type is NS; if it has
 * LOOK_FOR_GLUE_IN_ANSWER we look for glue incorrectly returned in the answer
 * section for A and AAAA queries.
 */
#define LOOK_FOR_NS_IN_ANSWER 0x1
#define LOOK_FOR_GLUE_IN_ANSWER 0x2

static isc_result_t
noanswer_response(fetchctx_t *fctx, dns_name_t *oqname,
		  unsigned int look_in_options)
{
	isc_result_t result;
	dns_message_t *message;
	dns_name_t *name, *qname, *ns_name, *soa_name, *ds_name, *save_name;
	dns_rdataset_t *rdataset, *ns_rdataset;
	isc_boolean_t aa, negative_response;
	dns_rdatatype_t type, save_type;
	dns_section_t section;

	FCTXTRACE("noanswer_response");

	if ((look_in_options & LOOK_FOR_NS_IN_ANSWER) != 0) {
		INSIST(fctx->type == dns_rdatatype_ns);
		section = DNS_SECTION_ANSWER;
	} else
		section = DNS_SECTION_AUTHORITY;

	message = fctx->rmessage;

	/*
	 * Setup qname.
	 */
	if (oqname == NULL) {
		/*
		 * We have a normal, non-chained negative response or
		 * referral.
		 */
		if ((message->flags & DNS_MESSAGEFLAG_AA) != 0)
			aa = ISC_TRUE;
		else
			aa = ISC_FALSE;
		qname = &fctx->name;
	} else {
		/*
		 * We're being invoked by answer_response() after it has
		 * followed a CNAME/DNAME chain.
		 */
		qname = oqname;
		aa = ISC_FALSE;
		/*
		 * If the current qname is not a subdomain of the query
		 * domain, there's no point in looking at the authority
		 * section without doing DNSSEC validation.
		 *
		 * Until we do that validation, we'll just return success
		 * in this case.
		 */
		if (!dns_name_issubdomain(qname, &fctx->domain))
			return (ISC_R_SUCCESS);
	}

	/*
	 * We have to figure out if this is a negative response, or a
	 * referral.
	 */

	/*
	 * Sometimes we can tell if its a negative response by looking at
	 * the message header.
	 */
	negative_response = ISC_FALSE;
	if (message->rcode == dns_rcode_nxdomain ||
	    (message->counts[DNS_SECTION_ANSWER] == 0 &&
	     message->counts[DNS_SECTION_AUTHORITY] == 0))
		negative_response = ISC_TRUE;

	/*
	 * Process the authority section.
	 */
	ns_name = NULL;
	ns_rdataset = NULL;
	soa_name = NULL;
	ds_name = NULL;
	save_name = NULL;
	save_type = dns_rdatatype_none;
	result = dns_message_firstname(message, section);
	while (result == ISC_R_SUCCESS) {
		name = NULL;
		dns_message_currentname(message, section, &name);
		if (dns_name_issubdomain(name, &fctx->domain)) {
			/*
			 * Look for NS/SOA RRsets first.
			 */
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link)) {
				type = rdataset->type;
				if (type == dns_rdatatype_rrsig)
					type = rdataset->covers;
				if (((type == dns_rdatatype_ns ||
				      type == dns_rdatatype_soa) &&
				     !dns_name_issubdomain(qname, name))) {
					char qbuf[DNS_NAME_FORMATSIZE];
					char nbuf[DNS_NAME_FORMATSIZE];
					char tbuf[DNS_RDATATYPE_FORMATSIZE];
					dns_rdatatype_format(type, tbuf,
							     sizeof(tbuf));
					dns_name_format(name, nbuf,
							     sizeof(nbuf));
					dns_name_format(qname, qbuf,
							     sizeof(qbuf));
					log_formerr(fctx,
						    "unrelated %s %s in "
						    "%s authority section",
						    tbuf, nbuf, qbuf);
					goto nextname;
				}
				if (type == dns_rdatatype_ns) {
					/*
					 * NS or RRSIG NS.
					 *
					 * Only one set of NS RRs is allowed.
					 */
					if (rdataset->type ==
					    dns_rdatatype_ns) {
						if (ns_name != NULL &&
						    name != ns_name) {
							log_formerr(fctx,
								"multiple NS "
								"RRsets in "
								"authority "
								"section");
							return (DNS_R_FORMERR);
						}
						ns_name = name;
						ns_rdataset = rdataset;
					}
					name->attributes |=
						DNS_NAMEATTR_CACHE;
					rdataset->attributes |=
						DNS_RDATASETATTR_CACHE;
					rdataset->trust = dns_trust_glue;
				}
				if (type == dns_rdatatype_soa) {
					/*
					 * SOA, or RRSIG SOA.
					 *
					 * Only one SOA is allowed.
					 */
					if (rdataset->type ==
					    dns_rdatatype_soa) {
						if (soa_name != NULL &&
						    name != soa_name) {
							log_formerr(fctx,
								"multiple SOA "
								"RRs in "
								"authority "
								"section");
							return (DNS_R_FORMERR);
						}
						soa_name = name;
					}
					name->attributes |=
						DNS_NAMEATTR_NCACHE;
					rdataset->attributes |=
						DNS_RDATASETATTR_NCACHE;
					if (aa)
						rdataset->trust =
						    dns_trust_authauthority;
					else if (ISFORWARDER(fctx->addrinfo))
						rdataset->trust =
							dns_trust_answer;
					else
						rdataset->trust =
							dns_trust_additional;
				}
			}
		}
 nextname:
		result = dns_message_nextname(message, section);
		if (result == ISC_R_NOMORE)
			break;
		else if (result != ISC_R_SUCCESS)
			return (result);
	}

	log_ns_ttl(fctx, "noanswer_response");

	if (ns_rdataset != NULL && dns_name_equal(&fctx->domain, ns_name) &&
	    !dns_name_equal(ns_name, dns_rootname))
		trim_ns_ttl(fctx, ns_name, ns_rdataset);

	/*
	 * A negative response has a SOA record (Type 2)
	 * and a optional NS RRset (Type 1) or it has neither
	 * a SOA or a NS RRset (Type 3, handled above) or
	 * rcode is NXDOMAIN (handled above) in which case
	 * the NS RRset is allowed (Type 4).
	 */
	if (soa_name != NULL)
		negative_response = ISC_TRUE;

	result = dns_message_firstname(message, section);
	while (result == ISC_R_SUCCESS) {
		name = NULL;
		dns_message_currentname(message, section, &name);
		if (dns_name_issubdomain(name, &fctx->domain)) {
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link)) {
				type = rdataset->type;
				if (type == dns_rdatatype_rrsig)
					type = rdataset->covers;
				if (type == dns_rdatatype_nsec ||
				    type == dns_rdatatype_nsec3) {
					/*
					 * NSEC or RRSIG NSEC.
					 */
					if (negative_response) {
						name->attributes |=
							DNS_NAMEATTR_NCACHE;
						rdataset->attributes |=
							DNS_RDATASETATTR_NCACHE;
					} else if (type == dns_rdatatype_nsec) {
						name->attributes |=
							DNS_NAMEATTR_CACHE;
						rdataset->attributes |=
							DNS_RDATASETATTR_CACHE;
					}
					if (aa)
						rdataset->trust =
						    dns_trust_authauthority;
					else if (ISFORWARDER(fctx->addrinfo))
						rdataset->trust =
							dns_trust_answer;
					else
						rdataset->trust =
							dns_trust_additional;
					/*
					 * No additional data needs to be
					 * marked.
					 */
				} else if (type == dns_rdatatype_ds) {
					/*
					 * DS or SIG DS.
					 *
					 * These should only be here if
					 * this is a referral, and there
					 * should only be one DS RRset.
					 */
					if (ns_name == NULL) {
						log_formerr(fctx,
							    "DS with no "
							    "referral");
						return (DNS_R_FORMERR);
					}
					if (rdataset->type ==
					    dns_rdatatype_ds) {
						if (ds_name != NULL &&
						    name != ds_name) {
							log_formerr(fctx,
								"DS doesn't "
								"match "
								"referral "
								"(NS)");
							return (DNS_R_FORMERR);
						}
						ds_name = name;
					}
					name->attributes |=
						DNS_NAMEATTR_CACHE;
					rdataset->attributes |=
						DNS_RDATASETATTR_CACHE;
					if (aa)
						rdataset->trust =
						    dns_trust_authauthority;
					else if (ISFORWARDER(fctx->addrinfo))
						rdataset->trust =
							dns_trust_answer;
					else
						rdataset->trust =
							dns_trust_additional;
				}
			}
		} else {
			save_name = name;
			save_type = ISC_LIST_HEAD(name->list)->type;
		}
		result = dns_message_nextname(message, section);
		if (result == ISC_R_NOMORE)
			break;
		else if (result != ISC_R_SUCCESS)
			return (result);
	}

	/*
	 * Trigger lookups for DNS nameservers.
	 */
	if (negative_response && message->rcode == dns_rcode_noerror &&
	    fctx->type == dns_rdatatype_ds && soa_name != NULL &&
	    dns_name_equal(soa_name, qname) &&
	    !dns_name_equal(qname, dns_rootname))
		return (DNS_R_CHASEDSSERVERS);

	/*
	 * Did we find anything?
	 */
	if (!negative_response && ns_name == NULL) {
		/*
		 * Nope.
		 */
		if (oqname != NULL) {
			/*
			 * We've already got a partial CNAME/DNAME chain,
			 * and haven't found else anything useful here, but
			 * no error has occurred since we have an answer.
			 */
			return (ISC_R_SUCCESS);
		} else {
			/*
			 * The responder is insane.
			 */
			if (save_name == NULL) {
				log_formerr(fctx, "invalid response");
				return (DNS_R_FORMERR);
			}
			if (!dns_name_issubdomain(save_name, &fctx->domain)) {
				char nbuf[DNS_NAME_FORMATSIZE];
				char dbuf[DNS_NAME_FORMATSIZE];
				char tbuf[DNS_RDATATYPE_FORMATSIZE];

				dns_rdatatype_format(save_type, tbuf,
					sizeof(tbuf));
				dns_name_format(save_name, nbuf, sizeof(nbuf));
				dns_name_format(&fctx->domain, dbuf,
					sizeof(dbuf));

				log_formerr(fctx, "Name %s (%s) not subdomain"
					" of zone %s -- invalid response",
					nbuf, tbuf, dbuf);
			} else {
				log_formerr(fctx, "invalid response");
			}
			return (DNS_R_FORMERR);
		}
	}

	/*
	 * If we found both NS and SOA, they should be the same name.
	 */
	if (ns_name != NULL && soa_name != NULL && ns_name != soa_name) {
		log_formerr(fctx, "NS/SOA mismatch");
		return (DNS_R_FORMERR);
	}

	/*
	 * Do we have a referral?  (We only want to follow a referral if
	 * we're not following a chain.)
	 */
	if (!negative_response && ns_name != NULL && oqname == NULL) {
		/*
		 * We already know ns_name is a subdomain of fctx->domain.
		 * If ns_name is equal to fctx->domain, we're not making
		 * progress.  We return DNS_R_FORMERR so that we'll keep
		 * trying other servers.
		 */
		if (dns_name_equal(ns_name, &fctx->domain)) {
			log_formerr(fctx, "non-improving referral");
			return (DNS_R_FORMERR);
		}

		/*
		 * If the referral name is not a parent of the query
		 * name, consider the responder insane.
		 */
		if (! dns_name_issubdomain(&fctx->name, ns_name)) {
			/* Logged twice */
			log_formerr(fctx, "referral to non-parent");
			FCTXTRACE("referral to non-parent");
			return (DNS_R_FORMERR);
		}

		/*
		 * Mark any additional data related to this rdataset.
		 * It's important that we do this before we change the
		 * query domain.
		 */
		INSIST(ns_rdataset != NULL);
		fctx->attributes |= FCTX_ATTR_GLUING;
		(void)dns_rdataset_additionaldata(ns_rdataset, check_related,
						  fctx);
#if CHECK_FOR_GLUE_IN_ANSWER
		/*
		 * Look in the answer section for "glue" that is incorrectly
		 * returned as a answer.  This is needed if the server also
		 * minimizes the response size by not adding records to the
		 * additional section that are in the answer section or if
		 * the record gets dropped due to message size constraints.
		 */
		if ((look_in_options & LOOK_FOR_GLUE_IN_ANSWER) != 0 &&
		    (fctx->type == dns_rdatatype_aaaa ||
		     fctx->type == dns_rdatatype_a))
			(void)dns_rdataset_additionaldata(ns_rdataset,
							  check_answer, fctx);
#endif
		fctx->attributes &= ~FCTX_ATTR_GLUING;
		/*
		 * NS rdatasets with 0 TTL cause problems.
		 * dns_view_findzonecut() will not find them when we
		 * try to follow the referral, and we'll SERVFAIL
		 * because the best nameservers are now above QDOMAIN.
		 * We force the TTL to 1 second to prevent this.
		 */
		if (ns_rdataset->ttl == 0)
			ns_rdataset->ttl = 1;
		/*
		 * Set the current query domain to the referral name.
		 *
		 * XXXRTH  We should check if we're in forward-only mode, and
		 *		if so we should bail out.
		 */
		INSIST(dns_name_countlabels(&fctx->domain) > 0);

#ifdef ENABLE_FETCHLIMIT
		fcount_decr(fctx);
#endif /* ENABLE_FETCHLIMIT */

		dns_name_free(&fctx->domain, fctx->mctx);
		if (dns_rdataset_isassociated(&fctx->nameservers))
			dns_rdataset_disassociate(&fctx->nameservers);
		dns_name_init(&fctx->domain, NULL);
		result = dns_name_dup(ns_name, fctx->mctx, &fctx->domain);
		if (result != ISC_R_SUCCESS)
			return (result);

#ifdef ENABLE_FETCHLIMIT
		result = fcount_incr(fctx, ISC_TRUE);
		if (result != ISC_R_SUCCESS)
			return (result);
#endif /* ENABLE_FETCHLIMIT */

		fctx->attributes |= FCTX_ATTR_WANTCACHE;
		fctx->ns_ttl_ok = ISC_FALSE;
		log_ns_ttl(fctx, "DELEGATION");
		return (DNS_R_DELEGATION);
	}

	/*
	 * Since we're not doing a referral, we don't want to cache any
	 * NS RRs we may have found.
	 */
	if (ns_name != NULL)
		ns_name->attributes &= ~DNS_NAMEATTR_CACHE;

	if (negative_response && oqname == NULL)
		fctx->attributes |= FCTX_ATTR_WANTNCACHE;

	return (ISC_R_SUCCESS);
}

static isc_boolean_t
validinanswer(dns_rdataset_t *rdataset, fetchctx_t *fctx) {
	if (rdataset->type == dns_rdatatype_nsec3) {
		/*
		 * NSEC3 records are not allowed to
		 * appear in the answer section.
		 */
		log_formerr(fctx, "NSEC3 in answer");
		return (ISC_FALSE);
	}
	if (rdataset->type == dns_rdatatype_tkey) {
		/*
		 * TKEY is not a valid record in a
		 * response to any query we can make.
		 */
		log_formerr(fctx, "TKEY in answer");
		return (ISC_FALSE);
	}
	if (rdataset->rdclass != fctx->res->rdclass) {
		log_formerr(fctx, "Mismatched class in answer");
		return (ISC_FALSE);
	}
	return (ISC_TRUE);
}

static isc_result_t
answer_response(fetchctx_t *fctx) {
	isc_result_t result;
	dns_message_t *message = NULL;
	dns_name_t *name = NULL, *qname = NULL, *ns_name = NULL;
	dns_name_t *aname = NULL, *cname = NULL, *dname = NULL;
	dns_rdataset_t *rdataset = NULL, *sigrdataset = NULL;
	dns_rdataset_t *ardataset = NULL, *crdataset = NULL;
	dns_rdataset_t *drdataset = NULL, *ns_rdataset = NULL;
	isc_boolean_t done = ISC_FALSE, aa;
	unsigned int dname_labels, domain_labels;
	isc_boolean_t chaining = ISC_FALSE;
	dns_rdatatype_t type;
	dns_view_t *view = NULL;
	dns_trust_t trust;

	REQUIRE(VALID_FCTX(fctx));

	FCTXTRACE("answer_response");

	message = fctx->rmessage;
	qname = &fctx->name;
	view = fctx->res->view;
	type = fctx->type;

	/*
	 * There can be multiple RRSIG and SIG records at a name so
	 * we treat these types as a subset of ANY.
	 */
	if (type == dns_rdatatype_rrsig || type == dns_rdatatype_sig) {
		type = dns_rdatatype_any;
	}

	/*
	 * Bigger than any valid DNAME label count.
	 */
	dname_labels = dns_name_countlabels(qname);
	domain_labels = dns_name_countlabels(&fctx->domain);

	/*
	 * Perform a single pass looking for the answer, cname or covering
	 * dname.
	 */
	for (result = dns_message_firstname(message, DNS_SECTION_ANSWER);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(message, DNS_SECTION_ANSWER))
	{
		int order;
		unsigned int nlabels;
		dns_namereln_t namereln;

		name = NULL;
		dns_message_currentname(message, DNS_SECTION_ANSWER, &name);
		namereln = dns_name_fullcompare(qname, name, &order, &nlabels);
		switch (namereln) {
		case dns_namereln_equal:
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link))
			{
				if (rdataset->type == type ||
				    type == dns_rdatatype_any)
				{
					aname = name;
					if (type != dns_rdatatype_any) {
						ardataset = rdataset;
					}
					break;
				}
				if (rdataset->type == dns_rdatatype_cname) {
					cname = name;
					crdataset = rdataset;
					break;
				}
			}
			break;

		case dns_namereln_subdomain:
			/*
			 * In-scope DNAME records must have at least
			 * as many labels as the domain being queried.
			 * They also must be less that qname's labels
			 * and any previously found dname.
			 */
			if (nlabels >= dname_labels || nlabels < domain_labels)
			{
				continue;
			}

			/*
			 * We are looking for the shortest DNAME if there
			 * are multiple ones (which there shouldn't be).
			 */
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link))
			{
				if (rdataset->type != dns_rdatatype_dname) {
					continue;
				}
				dname = name;
				drdataset = rdataset;
				dname_labels = nlabels;
				break;
			}
			break;
		default:
			break;
		}
	}

	if (dname != NULL) {
		aname = NULL;
		ardataset = NULL;
		cname = NULL;
		crdataset = NULL;
	} else if (aname != NULL) {
		cname = NULL;
		crdataset = NULL;
	}

	aa = ISC_TF((message->flags & DNS_MESSAGEFLAG_AA) != 0);
	trust = aa ? dns_trust_authanswer : dns_trust_answer;

	if (aname != NULL && type == dns_rdatatype_any) {
		for (rdataset = ISC_LIST_HEAD(aname->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link))
		{
			if (!validinanswer(rdataset, fctx)) {
				return (DNS_R_FORMERR);
			}
			if ((fctx->type == dns_rdatatype_sig ||
			     fctx->type == dns_rdatatype_rrsig) &&
			     rdataset->type != fctx->type)
			{
				continue;
			}
			if ((rdataset->type == dns_rdatatype_a ||
			     rdataset->type == dns_rdatatype_aaaa) &&
			    !is_answeraddress_allowed(view, aname, rdataset))
			{
				return (DNS_R_SERVFAIL);
			}
			if ((rdataset->type == dns_rdatatype_cname ||
			     rdataset->type == dns_rdatatype_dname) &&
			     !is_answertarget_allowed(fctx, qname, aname,
						      rdataset, NULL))
			{
				return (DNS_R_SERVFAIL);
			}
			aname->attributes |= DNS_NAMEATTR_CACHE;
			aname->attributes |= DNS_NAMEATTR_ANSWER;
			rdataset->attributes |= DNS_RDATASETATTR_ANSWER;
			rdataset->attributes |= DNS_RDATASETATTR_CACHE;
			rdataset->trust = trust;
			(void)dns_rdataset_additionaldata(rdataset,
							  check_related,
							  fctx);
		}
	} else if (aname != NULL) {
		if (!validinanswer(ardataset, fctx))
			return (DNS_R_FORMERR);
		if ((ardataset->type == dns_rdatatype_a ||
		     ardataset->type == dns_rdatatype_aaaa) &&
		    !is_answeraddress_allowed(view, aname, ardataset)) {
			return (DNS_R_SERVFAIL);
		}
		if ((ardataset->type == dns_rdatatype_cname ||
		     ardataset->type == dns_rdatatype_dname) &&
		     !is_answertarget_allowed(fctx, qname, aname, ardataset,
					      NULL))
		{
			return (DNS_R_SERVFAIL);
		}
		aname->attributes |= DNS_NAMEATTR_CACHE;
		aname->attributes |= DNS_NAMEATTR_ANSWER;
		ardataset->attributes |= DNS_RDATASETATTR_ANSWER;
		ardataset->attributes |= DNS_RDATASETATTR_CACHE;
		ardataset->trust = trust;
		(void)dns_rdataset_additionaldata(ardataset, check_related,
						  fctx);
		for (sigrdataset = ISC_LIST_HEAD(aname->list);
		     sigrdataset != NULL;
		     sigrdataset = ISC_LIST_NEXT(sigrdataset, link)) {
			if (!validinanswer(sigrdataset, fctx))
				return (DNS_R_FORMERR);
			if (sigrdataset->type != dns_rdatatype_rrsig ||
			    sigrdataset->covers != type)
				continue;
			sigrdataset->attributes |= DNS_RDATASETATTR_ANSWERSIG;
			sigrdataset->attributes |= DNS_RDATASETATTR_CACHE;
			sigrdataset->trust = trust;
			break;
		}
	} else if (cname != NULL) {
		if (!validinanswer(crdataset, fctx)) {
			return (DNS_R_FORMERR);
		}
		if (type == dns_rdatatype_rrsig || type == dns_rdatatype_key ||
		    type == dns_rdatatype_nsec)
		{
			char buf[DNS_RDATATYPE_FORMATSIZE];
			dns_rdatatype_format(type, buf, sizeof(buf));
			log_formerr(fctx, "CNAME response for %s RR", buf);
			return (DNS_R_FORMERR);
		}
		if (!is_answertarget_allowed(fctx, qname, cname, crdataset,
					     NULL))
		{
			return (DNS_R_SERVFAIL);
		}
		cname->attributes |= DNS_NAMEATTR_CACHE;
		cname->attributes |= DNS_NAMEATTR_ANSWER;
		cname->attributes |= DNS_NAMEATTR_CHAINING;
		crdataset->attributes |= DNS_RDATASETATTR_ANSWER;
		crdataset->attributes |= DNS_RDATASETATTR_CACHE;
		crdataset->attributes |= DNS_RDATASETATTR_CHAINING;
		crdataset->trust = trust;
		for (sigrdataset = ISC_LIST_HEAD(cname->list);
		     sigrdataset != NULL;
		     sigrdataset = ISC_LIST_NEXT(sigrdataset, link))
		{
			if (!validinanswer(sigrdataset, fctx)) {
				return (DNS_R_FORMERR);
			}
			if (sigrdataset->type != dns_rdatatype_rrsig ||
			    sigrdataset->covers != dns_rdatatype_cname)
			{
				continue;
			}
			sigrdataset->attributes |= DNS_RDATASETATTR_ANSWERSIG;
			sigrdataset->attributes |= DNS_RDATASETATTR_CACHE;
			sigrdataset->trust = trust;
			break;
		}
		chaining = ISC_TRUE;
	} else if (dname != NULL) {
		if (!validinanswer(drdataset, fctx)) {
			return (DNS_R_FORMERR);
		}
		if (!is_answertarget_allowed(fctx, qname, dname, drdataset,
					     &chaining)) {
			return (DNS_R_SERVFAIL);
		}
		dname->attributes |= DNS_NAMEATTR_CACHE;
		dname->attributes |= DNS_NAMEATTR_ANSWER;
		dname->attributes |= DNS_NAMEATTR_CHAINING;
		drdataset->attributes |= DNS_RDATASETATTR_ANSWER;
		drdataset->attributes |= DNS_RDATASETATTR_CACHE;
		drdataset->attributes |= DNS_RDATASETATTR_CHAINING;
		drdataset->trust = trust;
		for (sigrdataset = ISC_LIST_HEAD(dname->list);
		     sigrdataset != NULL;
		     sigrdataset = ISC_LIST_NEXT(sigrdataset, link))
		{
			if (!validinanswer(sigrdataset, fctx)) {
				return (DNS_R_FORMERR);
			}
			if (sigrdataset->type != dns_rdatatype_rrsig ||
			    sigrdataset->covers != dns_rdatatype_dname)
			{
				continue;
			}
			sigrdataset->attributes |= DNS_RDATASETATTR_ANSWERSIG;
			sigrdataset->attributes |= DNS_RDATASETATTR_CACHE;
			sigrdataset->trust = trust;
			break;
		}
	} else {
		log_formerr(fctx, "reply has no answer");
		return (DNS_R_FORMERR);
	}

	/*
	 * This response is now potentially cacheable.
	 */
	fctx->attributes |= FCTX_ATTR_WANTCACHE;

	/*
	 * Did chaining end before we got the final answer?
	 */
	if (chaining) {
		return (ISC_R_SUCCESS);
	}

	/*
	 * We didn't end with an incomplete chain, so the rcode should be
	 * "no error".
	 */
	if (message->rcode != dns_rcode_noerror) {
		log_formerr(fctx, "CNAME/DNAME chain complete, but RCODE "
				  "indicates error");
		return (DNS_R_FORMERR);
	}

	/*
	 * Examine the authority section (if there is one).
	 *
	 * We expect there to be only one owner name for all the rdatasets
	 * in this section, and we expect that it is not external.
	 */
	result = dns_message_firstname(message, DNS_SECTION_AUTHORITY);
	while (!done && result == ISC_R_SUCCESS) {
		isc_boolean_t external;
		name = NULL;
		dns_message_currentname(message, DNS_SECTION_AUTHORITY, &name);
		external = ISC_TF(!dns_name_issubdomain(name, &fctx->domain));
		if (!external) {
			/*
			 * We expect to find NS or SIG NS rdatasets, and
			 * nothing else.
			 */
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link)) {
				if (rdataset->type == dns_rdatatype_ns ||
				    (rdataset->type == dns_rdatatype_rrsig &&
				     rdataset->covers == dns_rdatatype_ns)) {
					name->attributes |=
						DNS_NAMEATTR_CACHE;
					rdataset->attributes |=
						DNS_RDATASETATTR_CACHE;
					if (aa && !chaining) {
						rdataset->trust =
						    dns_trust_authauthority;
					} else {
						rdataset->trust =
						    dns_trust_additional;
					}

					if (rdataset->type == dns_rdatatype_ns)
					{
						ns_name = name;
						ns_rdataset = rdataset;
					}
					/*
					 * Mark any additional data related
					 * to this rdataset.
					 */
					(void)dns_rdataset_additionaldata(
							rdataset,
							check_related,
							fctx);
					done = ISC_TRUE;
				}
			}
		}
		result = dns_message_nextname(message, DNS_SECTION_AUTHORITY);
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;

	log_ns_ttl(fctx, "answer_response");

	if (ns_rdataset != NULL && dns_name_equal(&fctx->domain, ns_name) &&
	    !dns_name_equal(ns_name, dns_rootname))
		trim_ns_ttl(fctx, ns_name, ns_rdataset);

	return (result);
}

static void
fctx_increference(fetchctx_t *fctx) {
	REQUIRE(VALID_FCTX(fctx));

	LOCK(&fctx->res->buckets[fctx->bucketnum].lock);
	fctx->references++;
	UNLOCK(&fctx->res->buckets[fctx->bucketnum].lock);
}

static isc_boolean_t
fctx_decreference(fetchctx_t *fctx) {
	isc_boolean_t bucket_empty = ISC_FALSE;

	REQUIRE(VALID_FCTX(fctx));

	INSIST(fctx->references > 0);
	fctx->references--;
	if (fctx->references == 0) {
		/*
		 * No one cares about the result of this fetch anymore.
		 */
		if (fctx->pending == 0 && fctx->nqueries == 0 &&
		    ISC_LIST_EMPTY(fctx->validators) && SHUTTINGDOWN(fctx)) {
			/*
			 * This fctx is already shutdown; we were just
			 * waiting for the last reference to go away.
			 */
			bucket_empty = fctx_unlink(fctx);
			fctx_destroy(fctx);
		} else {
			/*
			 * Initiate shutdown.
			 */
			fctx_shutdown(fctx);
		}
	}
	return (bucket_empty);
}

static void
resume_dslookup(isc_task_t *task, isc_event_t *event) {
	dns_fetchevent_t *fevent;
	dns_resolver_t *res;
	fetchctx_t *fctx;
	isc_result_t result;
	isc_boolean_t bucket_empty;
	isc_boolean_t locked = ISC_FALSE;
	unsigned int bucketnum;
	dns_rdataset_t nameservers;
	dns_fixedname_t fixed;
	dns_name_t *domain;

	REQUIRE(event->ev_type == DNS_EVENT_FETCHDONE);
	fevent = (dns_fetchevent_t *)event;
	fctx = event->ev_arg;
	REQUIRE(VALID_FCTX(fctx));
	res = fctx->res;

	UNUSED(task);
	FCTXTRACE("resume_dslookup");

	if (fevent->node != NULL)
		dns_db_detachnode(fevent->db, &fevent->node);
	if (fevent->db != NULL)
		dns_db_detach(&fevent->db);

	dns_rdataset_init(&nameservers);

	bucketnum = fctx->bucketnum;

	/*
	 * Note: fevent->rdataset must be disassociated and
	 * isc_event_free(&event) be called before resuming
	 * processing of the 'fctx' to prevent use-after-free.
	 * 'fevent' is set to NULL so as to not have a dangling
	 * pointer.
	 */
	if (fevent->result == ISC_R_CANCELED) {
		if (dns_rdataset_isassociated(fevent->rdataset)) {
			dns_rdataset_disassociate(fevent->rdataset);
		}
		fevent = NULL;
		isc_event_free(&event);

		dns_resolver_destroyfetch(&fctx->nsfetch);
		fctx_done(fctx, ISC_R_CANCELED, __LINE__);
	} else if (fevent->result == ISC_R_SUCCESS) {
		FCTXTRACE("resuming DS lookup");

		dns_resolver_destroyfetch(&fctx->nsfetch);
		if (dns_rdataset_isassociated(&fctx->nameservers)) {
			dns_rdataset_disassociate(&fctx->nameservers);
		}
		dns_rdataset_clone(fevent->rdataset, &fctx->nameservers);
		fctx->ns_ttl = fctx->nameservers.ttl;
		fctx->ns_ttl_ok = ISC_TRUE;
		log_ns_ttl(fctx, "resume_dslookup");

		if (dns_rdataset_isassociated(fevent->rdataset)) {
			dns_rdataset_disassociate(fevent->rdataset);
		}
		fevent = NULL;
		isc_event_free(&event);

#ifdef ENABLE_FETCHLIMIT
		fcount_decr(fctx);
#endif /* ENABLE_FETCHLIMIT */

		dns_name_free(&fctx->domain, fctx->mctx);
		dns_name_init(&fctx->domain, NULL);
		result = dns_name_dup(&fctx->nsname, fctx->mctx, &fctx->domain);
		if (result != ISC_R_SUCCESS) {
			fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
			goto cleanup;
		}

#ifdef ENABLE_FETCHLIMIT
		result = fcount_incr(fctx, ISC_TRUE);
		if (result != ISC_R_SUCCESS) {
			fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
			goto cleanup;
		}
#endif /* ENABLE_FETCHLIMIT */

		/*
		 * Try again.
		 */
		fctx_try(fctx, ISC_TRUE, ISC_FALSE);
	} else {
		unsigned int n;
		dns_rdataset_t *nsrdataset = NULL;

		/*
		 * Retrieve state from fctx->nsfetch before we destroy it.
		 */
		dns_fixedname_init(&fixed);
		domain = dns_fixedname_name(&fixed);
		dns_name_copy(&fctx->nsfetch->private->domain, domain, NULL);
		if (dns_name_equal(&fctx->nsname, domain)) {
			if (dns_rdataset_isassociated(fevent->rdataset)) {
				dns_rdataset_disassociate(fevent->rdataset);
			}
			fevent = NULL;
			isc_event_free(&event);

			fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
			dns_resolver_destroyfetch(&fctx->nsfetch);
			goto cleanup;
		}
		if (dns_rdataset_isassociated(
		    &fctx->nsfetch->private->nameservers)) {
			dns_rdataset_clone(
			    &fctx->nsfetch->private->nameservers,
			    &nameservers);
			nsrdataset = &nameservers;
		} else
			domain = NULL;
		dns_resolver_destroyfetch(&fctx->nsfetch);
		n = dns_name_countlabels(&fctx->nsname);
		dns_name_getlabelsequence(&fctx->nsname, 1, n - 1,
					  &fctx->nsname);

		if (dns_rdataset_isassociated(fevent->rdataset))
			dns_rdataset_disassociate(fevent->rdataset);
		fevent = NULL;
		isc_event_free(&event);

		FCTXTRACE("continuing to look for parent's NS records");

		result = dns_resolver_createfetch(fctx->res, &fctx->nsname,
						  dns_rdatatype_ns, domain,
						  nsrdataset, NULL,
						  fctx->options, task,
						  resume_dslookup, fctx,
						  &fctx->nsrrset, NULL,
						  &fctx->nsfetch);
		/*
		 * fevent->rdataset (a.k.a. fctx->nsrrset) must not be
		 * accessed below this point to prevent races with
		 * another thread concurrently processing the fetch.
		 */
		if (result != ISC_R_SUCCESS) {
			fctx_done(fctx, result, __LINE__);
		} else {
			LOCK(&res->buckets[bucketnum].lock);
			locked = ISC_TRUE;
			fctx->references++;
		}
	}

 cleanup:
	INSIST(event == NULL);
	INSIST(fevent == NULL);
	if (dns_rdataset_isassociated(&nameservers))
		dns_rdataset_disassociate(&nameservers);
	if (!locked)
		LOCK(&res->buckets[bucketnum].lock);
	bucket_empty = fctx_decreference(fctx);
	UNLOCK(&res->buckets[bucketnum].lock);
	if (bucket_empty)
		empty_bucket(res);
}

static inline void
checknamessection(dns_message_t *message, dns_section_t section) {
	isc_result_t result;
	dns_name_t *name;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdataset_t *rdataset;

	for (result = dns_message_firstname(message, section);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(message, section))
	{
		name = NULL;
		dns_message_currentname(message, section, &name);
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link)) {
			for (result = dns_rdataset_first(rdataset);
			     result == ISC_R_SUCCESS;
			     result = dns_rdataset_next(rdataset)) {
				dns_rdataset_current(rdataset, &rdata);
				if (!dns_rdata_checkowner(name, rdata.rdclass,
							  rdata.type,
							  ISC_FALSE) ||
				    !dns_rdata_checknames(&rdata, name, NULL))
				{
					rdataset->attributes |=
						DNS_RDATASETATTR_CHECKNAMES;
				}
				dns_rdata_reset(&rdata);
			}
		}
	}
}

static void
checknames(dns_message_t *message) {

	checknamessection(message, DNS_SECTION_ANSWER);
	checknamessection(message, DNS_SECTION_AUTHORITY);
	checknamessection(message, DNS_SECTION_ADDITIONAL);
}

/*
 * Log server NSID at log level 'level'
 */
static void
log_nsid(isc_buffer_t *opt, size_t nsid_len, resquery_t *query,
	 int level, isc_mem_t *mctx)
{
	static const char hex[17] = "0123456789abcdef";
	char addrbuf[ISC_SOCKADDR_FORMATSIZE];
	isc_uint16_t buflen, i;
	unsigned char *p, *nsid;
	unsigned char *buf = NULL, *pbuf = NULL;

	/* Allocate buffer for storing hex version of the NSID */
	buflen = (isc_uint16_t)nsid_len * 2 + 1;
	buf = isc_mem_get(mctx, buflen);
	if (buf == NULL)
		goto cleanup;
	pbuf = isc_mem_get(mctx, nsid_len + 1);
	if (pbuf == NULL)
		goto cleanup;

	/* Convert to hex */
	p = buf;
	nsid = isc_buffer_current(opt);
	for (i = 0; i < nsid_len; i++) {
		*p++ = hex[(nsid[i] >> 4) & 0xf];
		*p++ = hex[nsid[i] & 0xf];
	}
	*p = '\0';

	/* Make printable version */
	p = pbuf;
	for (i = 0; i < nsid_len; i++) {
		if (isprint(nsid[i]))
			*p++ = nsid[i];
		else
			*p++ = '.';
	}
	*p = '\0';

	isc_sockaddr_format(&query->addrinfo->sockaddr, addrbuf,
			    sizeof(addrbuf));
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
		      DNS_LOGMODULE_RESOLVER, level,
		      "received NSID %s (\"%s\") from %s", buf, pbuf, addrbuf);
 cleanup:
	if (pbuf != NULL)
		isc_mem_put(mctx, pbuf, nsid_len + 1);
	if (buf != NULL)
		isc_mem_put(mctx, buf, buflen);
}

static isc_boolean_t
iscname(fetchctx_t *fctx) {
	isc_result_t result;

	result = dns_message_findname(fctx->rmessage, DNS_SECTION_ANSWER,
				      &fctx->name, dns_rdatatype_cname, 0,
				      NULL, NULL);
	return (result == ISC_R_SUCCESS ? ISC_TRUE : ISC_FALSE);
}

static isc_boolean_t
betterreferral(fetchctx_t *fctx) {
	isc_result_t result;
	dns_name_t *name;
	dns_rdataset_t *rdataset;
	dns_message_t *message = fctx->rmessage;

	for (result = dns_message_firstname(message, DNS_SECTION_AUTHORITY);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(message, DNS_SECTION_AUTHORITY)) {
		name = NULL;
		dns_message_currentname(message, DNS_SECTION_AUTHORITY, &name);
		if (!isstrictsubdomain(name, &fctx->domain))
			continue;
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link))
			if (rdataset->type == dns_rdatatype_ns)
				return (ISC_TRUE);
	}
	return (ISC_FALSE);
}

static void
process_opt(resquery_t *query, dns_rdataset_t *opt) {
	dns_rdata_t rdata;
	isc_buffer_t optbuf;
	isc_result_t result;
	isc_uint16_t optcode;
	isc_uint16_t optlen;
#ifdef ISC_PLATFORM_USESIT
	unsigned char *optvalue;
	dns_adbaddrinfo_t *addrinfo;
	unsigned char cookie[8];
	isc_boolean_t seen_cookie = ISC_FALSE;
#endif
	isc_boolean_t seen_nsid = ISC_FALSE;

	result = dns_rdataset_first(opt);
	if (result == ISC_R_SUCCESS) {
		dns_rdata_init(&rdata);
		dns_rdataset_current(opt, &rdata);
		isc_buffer_init(&optbuf, rdata.data, rdata.length);
		isc_buffer_add(&optbuf, rdata.length);
		while (isc_buffer_remaininglength(&optbuf) >= 4) {
			optcode = isc_buffer_getuint16(&optbuf);
			optlen = isc_buffer_getuint16(&optbuf);
			INSIST(optlen <= isc_buffer_remaininglength(&optbuf));
			switch (optcode) {
			case DNS_OPT_NSID:
				if (!seen_nsid &&
				    query->options & DNS_FETCHOPT_WANTNSID)
					log_nsid(&optbuf, optlen, query,
						 ISC_LOG_DEBUG(3),
						 query->fctx->res->mctx);
				isc_buffer_forward(&optbuf, optlen);
				seen_nsid = ISC_TRUE;
				break;
#ifdef ISC_PLATFORM_USESIT
			case DNS_OPT_COOKIE:
				/*
				 * Only process the first cookie option.
				 */
				if (seen_cookie) {
					isc_buffer_forward(&optbuf, optlen);
					break;
				}
				optvalue = isc_buffer_current(&optbuf);
				compute_cc(query, cookie, sizeof(cookie));
				INSIST(query->fctx->rmessage->sitbad == 0 &&
				       query->fctx->rmessage->sitok == 0);
				if (optlen >= 8U &&
				    memcmp(cookie, optvalue, 8) == 0) {
					query->fctx->rmessage->sitok = 1;
					inc_stats(query->fctx->res,
						  dns_resstatscounter_sitok);
					addrinfo = query->addrinfo;
					dns_adb_setsit(query->fctx->adb,
						       addrinfo, optvalue,
						       optlen);
				} else
					query->fctx->rmessage->sitbad = 1;
				isc_buffer_forward(&optbuf, optlen);
				inc_stats(query->fctx->res,
					  dns_resstatscounter_sitin);
				seen_cookie = ISC_TRUE;
				break;
#endif
			default:
				isc_buffer_forward(&optbuf, optlen);
				break;
			}
		}
		INSIST(isc_buffer_remaininglength(&optbuf) == 0U);
	}
}

static void
resquery_response(isc_task_t *task, isc_event_t *event) {
	isc_result_t result = ISC_R_SUCCESS;
	resquery_t *query = event->ev_arg;
	dns_dispatchevent_t *devent = (dns_dispatchevent_t *)event;
	isc_boolean_t keep_trying, get_nameservers, resend;
	isc_boolean_t truncated;
	dns_message_t *message;
	dns_rdataset_t *opt;
	fetchctx_t *fctx;
	dns_name_t *fname;
	dns_fixedname_t foundname;
	isc_stdtime_t now;
	isc_time_t tnow, *finish;
	dns_adbaddrinfo_t *addrinfo;
	unsigned int options;
	unsigned int findoptions;
	isc_result_t broken_server;
	badnstype_t broken_type = badns_response;
	isc_boolean_t no_response;
	unsigned int bucketnum;
	dns_resolver_t *res;
	isc_boolean_t bucket_empty;

	REQUIRE(VALID_QUERY(query));
	fctx = query->fctx;
	options = query->options;
	REQUIRE(VALID_FCTX(fctx));
	REQUIRE(event->ev_type == DNS_EVENT_DISPATCH);

	QTRACE("response");

	res = fctx->res;
	if (isc_sockaddr_pf(&query->addrinfo->sockaddr) == PF_INET)
		inc_stats(res, dns_resstatscounter_responsev4);
	else
		inc_stats(res, dns_resstatscounter_responsev6);

	(void)isc_timer_touch(fctx->timer);

	keep_trying = ISC_FALSE;
	broken_server = ISC_R_SUCCESS;
	get_nameservers = ISC_FALSE;
	resend = ISC_FALSE;
	truncated = ISC_FALSE;
	finish = NULL;
	no_response = ISC_FALSE;

	if (res->exiting) {
		result = ISC_R_SHUTTINGDOWN;
		FCTXTRACE("resolver shutting down");
		goto done;
	}

	fctx->timeouts = 0;
	fctx->timeout = ISC_FALSE;
	fctx->addrinfo = query->addrinfo;

	/*
	 * XXXRTH  We should really get the current time just once.  We
	 *		need a routine to convert from an isc_time_t to an
	 *		isc_stdtime_t.
	 */
	TIME_NOW(&tnow);
	finish = &tnow;
	isc_stdtime_get(&now);

	/*
	 * Did the dispatcher have a problem?
	 */
	if (devent->result != ISC_R_SUCCESS) {
		if (devent->result == ISC_R_EOF &&
		    (query->options & DNS_FETCHOPT_NOEDNS0) == 0) {
			/*
			 * The problem might be that they
			 * don't understand EDNS0.  Turn it
			 * off and try again.
			 */
			options |= DNS_FETCHOPT_NOEDNS0;
			resend = ISC_TRUE;
			add_bad_edns(fctx, &query->addrinfo->sockaddr);
		} else {
			/*
			 * There's no hope for this query.
			 */
			keep_trying = ISC_TRUE;

			/*
			 * If this is a network error on an exclusive query
			 * socket, mark the server as bad so that we won't try
			 * it for this fetch again.  Also adjust finish and
			 * no_response so that we penalize this address in SRTT
			 * adjustment later.
			 */
			if (query->exclusivesocket &&
			    (devent->result == ISC_R_HOSTUNREACH ||
			     devent->result == ISC_R_NETUNREACH ||
			     devent->result == ISC_R_CONNREFUSED ||
			     devent->result == ISC_R_CANCELED)) {
				    broken_server = devent->result;
				    broken_type = badns_unreachable;
				    finish = NULL;
				    no_response = ISC_TRUE;
			}
		}
		FCTXTRACE3("dispatcher failure", devent->result);
		goto done;
	}

	message = fctx->rmessage;

	if (query->tsig != NULL) {
		result = dns_message_setquerytsig(message, query->tsig);
		if (result != ISC_R_SUCCESS) {
			FCTXTRACE3("unable to set query tsig", result);
			goto done;
		}
	}

	if (query->tsigkey) {
		result = dns_message_settsigkey(message, query->tsigkey);
		if (result != ISC_R_SUCCESS) {
			FCTXTRACE3("unable to set tsig key", result);
			goto done;
		}
	}

	dns_message_setclass(message, res->rdclass);

	if ((options & DNS_FETCHOPT_TCP) == 0) {
		if ((options & DNS_FETCHOPT_NOEDNS0) == 0)
			dns_adb_setudpsize(fctx->adb, query->addrinfo,
				   isc_buffer_usedlength(&devent->buffer));
		else
			dns_adb_plainresponse(fctx->adb, query->addrinfo);
	}
	result = dns_message_parse(message, &devent->buffer, 0);
	if (result != ISC_R_SUCCESS) {
		FCTXTRACE3("message failed to parse", result);
		switch (result) {
		case ISC_R_UNEXPECTEDEND:
			if (!message->question_ok ||
			    (message->flags & DNS_MESSAGEFLAG_TC) == 0 ||
			    (options & DNS_FETCHOPT_TCP) != 0) {
				/*
				 * Either the message ended prematurely,
				 * and/or wasn't marked as being truncated,
				 * and/or this is a response to a query we
				 * sent over TCP.  In all of these cases,
				 * something is wrong with the remote
				 * server and we don't want to retry using
				 * TCP.
				 */
				if ((query->options & DNS_FETCHOPT_NOEDNS0)
				    == 0) {
					/*
					 * The problem might be that they
					 * don't understand EDNS0.  Turn it
					 * off and try again.
					 */
					options |= DNS_FETCHOPT_NOEDNS0;
					resend = ISC_TRUE;
					add_bad_edns(fctx,
						    &query->addrinfo->sockaddr);
					inc_stats(res,
						 dns_resstatscounter_edns0fail);
				} else {
					broken_server = result;
					keep_trying = ISC_TRUE;
				}
				goto done;
			}
			/*
			 * We defer retrying via TCP for a bit so we can
			 * check out this message further.
			 */
			truncated = ISC_TRUE;
			break;
		case DNS_R_FORMERR:
			if ((query->options & DNS_FETCHOPT_NOEDNS0) == 0) {
				/*
				 * The problem might be that they
				 * don't understand EDNS0.  Turn it
				 * off and try again.
				 */
				options |= DNS_FETCHOPT_NOEDNS0;
				resend = ISC_TRUE;
				add_bad_edns(fctx, &query->addrinfo->sockaddr);
				inc_stats(res, dns_resstatscounter_edns0fail);
			} else {
				broken_server = DNS_R_UNEXPECTEDRCODE;
				keep_trying = ISC_TRUE;
			}
			goto done;
		default:
			/*
			 * Something bad has happened.
			 */
			goto done;
		}
	}

	/*
	 * Log the incoming packet.
	 */
	dns_message_logfmtpacket(message, "received packet:\n",
				 DNS_LOGCATEGORY_RESOLVER,
				 DNS_LOGMODULE_PACKETS,
				 &dns_master_style_comment,
				 ISC_LOG_DEBUG(10),
				 res->mctx);

	if (message->rdclass != res->rdclass) {
		resend = ISC_TRUE;
		FCTXTRACE("bad class");
		goto done;
	}

	/*
	 * Process receive opt record.
	 */
	opt = dns_message_getopt(message);
	if (opt != NULL)
		process_opt(query, opt);

#ifdef notyet
#ifdef ISC_PLATFORM_USESIT
	if (message->sitbad) {
		/*
		 * If the COOKIE is bad assume it is a attack and retry.
		 */
		resend = ISC_TRUE;
		/* XXXMPA log it */
		FCTXTRACE("bad cookie");
		goto done;
	}
#endif
#endif

	/*
	 * If the message is signed, check the signature.  If not, this
	 * returns success anyway.
	 */
	result = dns_message_checksig(message, res->view);
	if (result != ISC_R_SUCCESS) {
		FCTXTRACE3("signature check failed", result);
		goto done;
	}

	/*
	 * The dispatcher should ensure we only get responses with QR set.
	 */
	INSIST((message->flags & DNS_MESSAGEFLAG_QR) != 0);
	/*
	 * INSIST() that the message comes from the place we sent it to,
	 * since the dispatch code should ensure this.
	 *
	 * INSIST() that the message id is correct (this should also be
	 * ensured by the dispatch code).
	 */

	/*
	 * We have an affirmative response to the query and we have
	 * previously got a response from this server which indicated
	 * EDNS may not be supported so we can now cache the lack of
	 * EDNS support.
	 */
	if (opt == NULL && !EDNSOK(query->addrinfo) &&
	    (message->rcode == dns_rcode_noerror ||
	     message->rcode == dns_rcode_nxdomain ||
	     message->rcode == dns_rcode_refused ||
	     message->rcode == dns_rcode_yxdomain) &&
	     bad_edns(fctx, &query->addrinfo->sockaddr)) {
		if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(3))) {
			char buf[4096], addrbuf[ISC_SOCKADDR_FORMATSIZE];
			isc_sockaddr_format(&query->addrinfo->sockaddr,
					    addrbuf, sizeof(addrbuf));
			snprintf(buf, sizeof(buf),
				 "received packet from %s (bad edns):\n",
				 addrbuf);
			dns_message_logpacket(message, buf,
				      DNS_LOGCATEGORY_RESOLVER,
				      DNS_LOGMODULE_RESOLVER,
				      ISC_LOG_DEBUG(3), res->mctx);
		}
		dns_adb_changeflags(fctx->adb, query->addrinfo,
				    DNS_FETCHOPT_NOEDNS0,
				    DNS_FETCHOPT_NOEDNS0);
	} else if (opt == NULL && (message->flags & DNS_MESSAGEFLAG_TC) == 0 &&
		   !EDNSOK(query->addrinfo) &&
		   (message->rcode == dns_rcode_noerror ||
		    message->rcode == dns_rcode_nxdomain) &&
		   (query->options & DNS_FETCHOPT_NOEDNS0) == 0) {
		/*
		 * We didn't get a OPT record in response to a EDNS query.
		 *
		 * Old versions of named incorrectly drop the OPT record
		 * when there is a signed, truncated response so we check
		 * that TC is not set.
		 *
		 * Record that the server is not talking EDNS.  While this
		 * should be safe to do for any rcode we limit it to NOERROR
		 * and NXDOMAIN.
		 */
		if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(3))) {
			char buf[4096], addrbuf[ISC_SOCKADDR_FORMATSIZE];
			isc_sockaddr_format(&query->addrinfo->sockaddr,
					    addrbuf, sizeof(addrbuf));
			snprintf(buf, sizeof(buf),
				 "received packet from %s (no opt):\n",
				 addrbuf);
			dns_message_logpacket(message, buf,
				      DNS_LOGCATEGORY_RESOLVER,
				      DNS_LOGMODULE_RESOLVER,
				      ISC_LOG_DEBUG(3), res->mctx);
		}
		dns_adb_changeflags(fctx->adb, query->addrinfo,
				    DNS_FETCHOPT_NOEDNS0,
				    DNS_FETCHOPT_NOEDNS0);
	}

	/*
	 * If we get a non error EDNS response record the fact so we
	 * won't fallback to plain DNS in the future for this server.
	 */
	if (opt != NULL && !EDNSOK(query->addrinfo) &&
	    (query->options & DNS_FETCHOPT_NOEDNS0) == 0 &&
	    (message->rcode == dns_rcode_noerror ||
	     message->rcode == dns_rcode_nxdomain ||
	     message->rcode == dns_rcode_refused ||
	     message->rcode == dns_rcode_yxdomain)) {
		dns_adb_changeflags(fctx->adb, query->addrinfo,
				    FCTX_ADDRINFO_EDNSOK,
				    FCTX_ADDRINFO_EDNSOK);
	}

	/*
	 * Deal with truncated responses by retrying using TCP.
	 */
	if ((message->flags & DNS_MESSAGEFLAG_TC) != 0)
		truncated = ISC_TRUE;

	if (truncated) {
		inc_stats(res, dns_resstatscounter_truncated);
		if ((options & DNS_FETCHOPT_TCP) != 0) {
			broken_server = DNS_R_TRUNCATEDTCP;
			keep_trying = ISC_TRUE;
		} else {
			options |= DNS_FETCHOPT_TCP;
			resend = ISC_TRUE;
		}
		FCTXTRACE3("message truncated", result);
		goto done;
	}

	/*
	 * Is it a query response?
	 */
	if (message->opcode != dns_opcode_query) {
		/* XXXRTH Log */
		broken_server = DNS_R_UNEXPECTEDOPCODE;
		keep_trying = ISC_TRUE;
		FCTXTRACE("invalid message opcode");
		goto done;
	}

	/*
	 * Update statistics about erroneous responses.
	 */
	if (message->rcode != dns_rcode_noerror) {
		switch (message->rcode) {
		case dns_rcode_nxdomain:
			inc_stats(res, dns_resstatscounter_nxdomain);
			break;
		case dns_rcode_servfail:
			inc_stats(res, dns_resstatscounter_servfail);
			break;
		case dns_rcode_formerr:
			inc_stats(res, dns_resstatscounter_formerr);
			break;
		case dns_rcode_refused:
			inc_stats(res, dns_resstatscounter_refused);
			break;
		case dns_rcode_badvers:
			inc_stats(res, dns_resstatscounter_badvers);
			break;
		default:
			inc_stats(res, dns_resstatscounter_othererror);
			break;
		}
	}

	/*
	 * Is the remote server broken, or does it dislike us?
	 */
	if (message->rcode != dns_rcode_noerror &&
	    message->rcode != dns_rcode_yxdomain &&
	    message->rcode != dns_rcode_nxdomain) {
		isc_buffer_t b;
		char code[64];
#ifdef ISC_PLATFORM_USESIT
		unsigned char cookie[64];

		/*
		 * Some servers do not ignore unknown EDNS options.
		 */
		if (!NOSIT(query->addrinfo) &&
		    (message->rcode == dns_rcode_formerr ||
		     message->rcode == dns_rcode_notimp ||
		     message->rcode == dns_rcode_refused) &&
		     dns_adb_getsit(fctx->adb, query->addrinfo,
				    cookie, sizeof(cookie)) == 0U) {
			dns_adb_changeflags(fctx->adb, query->addrinfo,
					    FCTX_ADDRINFO_NOSIT,
					    FCTX_ADDRINFO_NOSIT);
			resend = ISC_TRUE;
		} else
#endif
		if (((message->rcode == dns_rcode_formerr ||
		      message->rcode == dns_rcode_notimp) ||
		     (message->rcode == dns_rcode_servfail &&
		      dns_message_getopt(message) == NULL)) &&
		    (query->options & DNS_FETCHOPT_NOEDNS0) == 0) {
			/*
			 * It's very likely they don't like EDNS0.
			 * If the response code is SERVFAIL, also check if the
			 * response contains an OPT RR and don't cache the
			 * failure since it can be returned for various other
			 * reasons.
			 *
			 * XXXRTH  We should check if the question
			 *		we're asking requires EDNS0, and
			 *		if so, we should bail out.
			 */
			options |= DNS_FETCHOPT_NOEDNS0;
			resend = ISC_TRUE;
			/*
			 * Remember that they may not like EDNS0.
			 */
			add_bad_edns(fctx, &query->addrinfo->sockaddr);
			inc_stats(res, dns_resstatscounter_edns0fail);
		} else if (message->rcode == dns_rcode_formerr) {
			if (ISFORWARDER(query->addrinfo)) {
				/*
				 * This forwarder doesn't understand us,
				 * but other forwarders might.  Keep trying.
				 */
				broken_server = DNS_R_REMOTEFORMERR;
				keep_trying = ISC_TRUE;
			} else {
				/*
				 * The server doesn't understand us.  Since
				 * all servers for a zone need similar
				 * capabilities, we assume that we will get
				 * FORMERR from all servers, and thus we
				 * cannot make any more progress with this
				 * fetch.
				 */
				log_formerr(fctx, "server sent FORMERR");
				result = DNS_R_FORMERR;
			}
		} else if (message->rcode == dns_rcode_badvers) {
			unsigned int flags, mask;
			unsigned int version;
#ifdef ISC_PLATFORM_USESIT
			isc_boolean_t nosit = ISC_FALSE;

			/*
			 * Some servers return BADVERS to unknown
			 * EDNS options.  This cannot be long term
			 * strategy.  Do not disable COOKIE if we have
			 * already have received a COOKIE from this
			 * server.
			 */
			if (dns_adb_getsit(fctx->adb, query->addrinfo,
					   cookie, sizeof(cookie)) == 0U) {
				if (!NOSIT(query->addrinfo))
					nosit = ISC_TRUE;
				dns_adb_changeflags(fctx->adb, query->addrinfo,
						    FCTX_ADDRINFO_NOSIT,
						    FCTX_ADDRINFO_NOSIT);
			}
#endif

			INSIST(opt != NULL);
			version = (opt->ttl >> 16) & 0xff;
			flags = (version << DNS_FETCHOPT_EDNSVERSIONSHIFT) |
				DNS_FETCHOPT_EDNSVERSIONSET;
			mask = DNS_FETCHOPT_EDNSVERSIONMASK |
			       DNS_FETCHOPT_EDNSVERSIONSET;

			/*
			 * Record that we got a good EDNS response.
			 */
			if (query->ednsversion > (int)version &&
			    !EDNSOK(query->addrinfo)) {
				dns_adb_changeflags(fctx->adb, query->addrinfo,
						    FCTX_ADDRINFO_EDNSOK,
						    FCTX_ADDRINFO_EDNSOK);
			}

			/*
			 * Record the supported EDNS version.
			 */
			switch (version) {
			case 0:
				dns_adb_changeflags(fctx->adb, query->addrinfo,
						    flags, mask);
#ifdef ISC_PLATFORM_USESIT
				if (nosit) {
					resend = ISC_TRUE;
					break;
				}
#endif
				/* FALLTHROUGH */
			default:
				broken_server = DNS_R_BADVERS;
				keep_trying = ISC_TRUE;
				break;
			}
		} else if (message->rcode == dns_rcode_badcookie &&
			   message->sitok) {
			/*
			 * We have recorded the new cookie.
			 */
			if (BADCOOKIE(query->addrinfo))
				query->options |= DNS_FETCHOPT_TCP;
			query->addrinfo->flags |= FCTX_ADDRINFO_BADCOOKIE;
			resend = ISC_TRUE;
		} else {
			/*
			 * XXXRTH log.
			 */
			broken_server = DNS_R_UNEXPECTEDRCODE;
			INSIST(broken_server != ISC_R_SUCCESS);
			keep_trying = ISC_TRUE;
		}

		isc_buffer_init(&b, code, sizeof(code) - 1);
		dns_rcode_totext(fctx->rmessage->rcode, &b);
		code[isc_buffer_usedlength(&b)] = '\0';
		FCTXTRACE2("remote server broken: returned ", code);
		goto done;
	}

	/*
	 * Is the question the same as the one we asked?
	 */
	result = same_question(fctx);
	if (result != ISC_R_SUCCESS) {
		/* XXXRTH Log */
		if (result == DNS_R_FORMERR)
			keep_trying = ISC_TRUE;
		FCTXTRACE3("response did not match question", result);
		goto done;
	}

	/*
	 * Is the server lame?
	 */
	if (res->lame_ttl != 0 && !ISFORWARDER(query->addrinfo) &&
	    is_lame(fctx)) {
		inc_stats(res, dns_resstatscounter_lame);
		log_lame(fctx, query->addrinfo);
		result = dns_adb_marklame(fctx->adb, query->addrinfo,
					  &fctx->name, fctx->type,
					  now + res->lame_ttl);
		if (result != ISC_R_SUCCESS)
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
				      DNS_LOGMODULE_RESOLVER, ISC_LOG_ERROR,
				      "could not mark server as lame: %s",
				      isc_result_totext(result));
		broken_server = DNS_R_LAME;
		keep_trying = ISC_TRUE;
		FCTXTRACE("lame server");
		goto done;
	}

	/*
	 * Enforce delegations only zones like NET and COM.
	 */
	if (!ISFORWARDER(query->addrinfo) &&
	    dns_view_isdelegationonly(res->view, &fctx->domain) &&
	    !dns_name_equal(&fctx->domain, &fctx->name) &&
	    fix_mustbedelegationornxdomain(message, fctx)) {
		char namebuf[DNS_NAME_FORMATSIZE];
		char domainbuf[DNS_NAME_FORMATSIZE];
		char addrbuf[ISC_SOCKADDR_FORMATSIZE];
		char classbuf[64];
		char typebuf[64];

		dns_name_format(&fctx->name, namebuf, sizeof(namebuf));
		dns_name_format(&fctx->domain, domainbuf, sizeof(domainbuf));
		dns_rdatatype_format(fctx->type, typebuf, sizeof(typebuf));
		dns_rdataclass_format(res->rdclass, classbuf,
				      sizeof(classbuf));
		isc_sockaddr_format(&query->addrinfo->sockaddr, addrbuf,
				    sizeof(addrbuf));

		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DELEGATION_ONLY,
			     DNS_LOGMODULE_RESOLVER, ISC_LOG_NOTICE,
			     "enforced delegation-only for '%s' (%s/%s/%s) "
			     "from %s",
			     domainbuf, namebuf, typebuf, classbuf, addrbuf);
	}

	if ((res->options & DNS_RESOLVER_CHECKNAMES) != 0)
		checknames(message);

	/*
	 * Clear cache bits.
	 */
	fctx->attributes &= ~(FCTX_ATTR_WANTNCACHE | FCTX_ATTR_WANTCACHE);

	/*
	 * Did we get any answers?
	 */
	if (message->counts[DNS_SECTION_ANSWER] > 0 &&
	    (message->rcode == dns_rcode_noerror ||
	     message->rcode == dns_rcode_yxdomain ||
	     message->rcode == dns_rcode_nxdomain)) {
		/*
		 * [normal case]
		 * We've got answers.  If it has an authoritative answer or an
		 * answer from a forwarder, we're done.
		 */
		if ((message->flags & DNS_MESSAGEFLAG_AA) != 0 ||
		    ISFORWARDER(query->addrinfo))
		{
			result = answer_response(fctx);
			if (result != ISC_R_SUCCESS)
				FCTXTRACE3("answer_response (AA/fwd)", result);
		} else if (iscname(fctx) &&
			 fctx->type != dns_rdatatype_any &&
			 fctx->type != dns_rdatatype_cname)
		{
			/*
			 * A BIND8 server could return a non-authoritative
			 * answer when a CNAME is followed.  We should treat
			 * it as a valid answer.
			 */
			result = answer_response(fctx);
			if (result != ISC_R_SUCCESS)
				FCTXTRACE3("answer_response (!ANY/!CNAME)",
					   result);
		} else if (fctx->type != dns_rdatatype_ns &&
			   !betterreferral(fctx)) {
			/*
			 * Lame response !!!.
			 */
			result = answer_response(fctx);
			if (result != ISC_R_SUCCESS)
				FCTXTRACE3("answer_response (!NS)", result);
		} else {
			if (fctx->type == dns_rdatatype_ns) {
				/*
				 * A BIND 8 server could incorrectly return a
				 * non-authoritative answer to an NS query
				 * instead of a referral. Since this answer
				 * lacks the SIGs necessary to do DNSSEC
				 * validation, we must invoke the following
				 * special kludge to treat it as a referral.
				 */
				result = noanswer_response(fctx, NULL,
						   LOOK_FOR_NS_IN_ANSWER);
				if (result != ISC_R_SUCCESS)
					FCTXTRACE3("noanswer_response (NS)",
						   result);
			} else {
				/*
				 * Some other servers may still somehow include
				 * an answer when it should return a referral
				 * with an empty answer.  Check to see if we can
				 * treat this as a referral by ignoring the
				 * answer.  Further more, there may be an
				 * implementation that moves A/AAAA glue records
				 * to the answer section for that type of
				 * delegation when the query is for that glue
				 * record.  LOOK_FOR_GLUE_IN_ANSWER will handle
				 * such a corner case.
				 */
				result = noanswer_response(fctx, NULL,
						   LOOK_FOR_GLUE_IN_ANSWER);
				if (result != ISC_R_SUCCESS)
					FCTXTRACE3("noanswer_response", result);
			}
			if (result != DNS_R_DELEGATION) {
				/*
				 * At this point, AA is not set, the response
				 * is not a referral, and the server is not a
				 * forwarder.  It is technically lame and it's
				 * easier to treat it as such than to figure out
				 * some more elaborate course of action.
				 */
				broken_server = DNS_R_LAME;
				keep_trying = ISC_TRUE;
				goto done;
			}
			goto force_referral;
		}
		if (result != ISC_R_SUCCESS) {
			if (result == DNS_R_FORMERR)
				keep_trying = ISC_TRUE;
			goto done;
		}
	} else if (message->counts[DNS_SECTION_AUTHORITY] > 0 ||
		   message->rcode == dns_rcode_noerror ||
		   message->rcode == dns_rcode_nxdomain) {
		/*
		 * NXDOMAIN, NXRDATASET, or referral.
		 */
		result = noanswer_response(fctx, NULL, 0);
		switch (result) {
		case ISC_R_SUCCESS:
		case DNS_R_CHASEDSSERVERS:
			break;
		case DNS_R_DELEGATION:
 force_referral:
			/*
			 * We don't have the answer, but we know a better
			 * place to look.
			 */
			get_nameservers = ISC_TRUE;
			keep_trying = ISC_TRUE;
			/*
			 * We have a new set of name servers, and it
			 * has not experienced any restarts yet.
			 */
			fctx->restarts = 0;

			/*
			 * Update local statistics counters collected for each
			 * new zone.
			 */
			fctx->referrals++;
			fctx->querysent = 0;
			fctx->lamecount = 0;
			fctx->quotacount = 0;
			fctx->neterr = 0;
			fctx->badresp = 0;
			fctx->adberr = 0;

			result = ISC_R_SUCCESS;
			break;
		default:
			/*
			 * Something has gone wrong.
			 */
			if (result == DNS_R_FORMERR)
				keep_trying = ISC_TRUE;
			FCTXTRACE3("noanswer_response", result);
			goto done;
		}
	} else {
		/*
		 * The server is insane.
		 */
		/* XXXRTH Log */
		broken_server = DNS_R_UNEXPECTEDRCODE;
		keep_trying = ISC_TRUE;
		FCTXTRACE("broken server: unexpected rcode");
		goto done;
	}

	/*
	 * Follow additional section data chains.
	 */
	chase_additional(fctx);

	/*
	 * Cache the cacheable parts of the message.  This may also cause
	 * work to be queued to the DNSSEC validator.
	 */
	if (WANTCACHE(fctx)) {
		result = cache_message(fctx, query->addrinfo, now);
		if (result != ISC_R_SUCCESS) {
			FCTXTRACE3("cache_message complete", result);
			goto done;
		}
	}

	/*
	 * Ncache the negatively cacheable parts of the message.  This may
	 * also cause work to be queued to the DNSSEC validator.
	 */
	if (WANTNCACHE(fctx)) {
		dns_rdatatype_t covers;

		/*
		 * Cache DS NXDOMAIN seperately to other types.
		 */
		if (message->rcode == dns_rcode_nxdomain &&
		    fctx->type != dns_rdatatype_ds)
			covers = dns_rdatatype_any;
		else
			covers = fctx->type;

		/*
		 * Cache any negative cache entries in the message.
		 */
		result = ncache_message(fctx, query->addrinfo, covers, now);
		if (result != ISC_R_SUCCESS)
			FCTXTRACE3("ncache_message complete", result);
	}

 done:
	/*
	 * Remember the query's addrinfo, in case we need to mark the
	 * server as broken.
	 */
	addrinfo = query->addrinfo;

	FCTXTRACE4("query canceled in response(); ",
		   no_response ? "no response" : "responding",
		   result);

	/*
	 * Cancel the query.
	 *
	 * XXXRTH  Don't cancel the query if waiting for validation?
	 */
	fctx_cancelquery(&query, &devent, finish, no_response, ISC_FALSE);

	if (keep_trying) {
		if (result == DNS_R_FORMERR)
			broken_server = DNS_R_FORMERR;
		if (broken_server != ISC_R_SUCCESS) {
			/*
			 * Add this server to the list of bad servers for
			 * this fctx.
			 */
			add_bad(fctx, addrinfo, broken_server, broken_type);
		}

		if (get_nameservers) {
			dns_name_t *name;
			dns_fixedname_init(&foundname);
			fname = dns_fixedname_name(&foundname);
			if (result != ISC_R_SUCCESS) {
				fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
				return;
			}
			findoptions = 0;
			if (dns_rdatatype_atparent(fctx->type))
				findoptions |= DNS_DBFIND_NOEXACT;
			if ((options & DNS_FETCHOPT_UNSHARED) == 0)
				name = &fctx->name;
			else
				name = &fctx->domain;
			result = dns_view_findzonecut(res->view,
						      name, fname,
						      now, findoptions,
						      ISC_TRUE,
						      &fctx->nameservers,
						      NULL);
			if (result != ISC_R_SUCCESS) {
				FCTXTRACE("couldn't find a zonecut");
				fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
				return;
			}
			if (!dns_name_issubdomain(fname, &fctx->domain)) {
				/*
				 * The best nameservers are now above our
				 * QDOMAIN.
				 */
				FCTXTRACE("nameservers now above QDOMAIN");
				fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
				return;
			}

#ifdef ENABLE_FETCHLIMIT
			fcount_decr(fctx);
#endif /* ENABLE_FETCHLIMIT */

			dns_name_free(&fctx->domain, fctx->mctx);
			dns_name_init(&fctx->domain, NULL);
			result = dns_name_dup(fname, fctx->mctx, &fctx->domain);
			if (result != ISC_R_SUCCESS) {
				fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
				return;
			}

#ifdef ENABLE_FETCHLIMIT
			result = fcount_incr(fctx, ISC_TRUE);
			if (result != ISC_R_SUCCESS) {
				fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
				return;
			}
#endif /* ENABLE_FETCHLIMIT */

			fctx->ns_ttl = fctx->nameservers.ttl;
			fctx->ns_ttl_ok = ISC_TRUE;
			fctx_cancelqueries(fctx, ISC_TRUE, ISC_FALSE);
			fctx_cleanupfinds(fctx);
			fctx_cleanupaltfinds(fctx);
			fctx_cleanupforwaddrs(fctx);
			fctx_cleanupaltaddrs(fctx);
		}
		/*
		 * Try again.
		 */
		fctx_try(fctx, !get_nameservers, ISC_FALSE);
	} else if (resend) {
		/*
		 * Resend (probably with changed options).
		 */
		FCTXTRACE("resend");
		inc_stats(res, dns_resstatscounter_retry);
		bucketnum = fctx->bucketnum;
		fctx_increference(fctx);
		result = fctx_query(fctx, addrinfo, options);
		if (result != ISC_R_SUCCESS) {
			fctx_done(fctx, result, __LINE__);
			LOCK(&res->buckets[bucketnum].lock);
			bucket_empty = fctx_decreference(fctx);
			UNLOCK(&res->buckets[bucketnum].lock);
			if (bucket_empty)
				empty_bucket(res);
		}
	} else if (result == ISC_R_SUCCESS && !HAVE_ANSWER(fctx)) {
		/*
		 * All has gone well so far, but we are waiting for the
		 * DNSSEC validator to validate the answer.
		 */
		FCTXTRACE("wait for validator");
		fctx_cancelqueries(fctx, ISC_TRUE, ISC_FALSE);
		/*
		 * We must not retransmit while the validator is working;
		 * it has references to the current rmessage.
		 */
		result = fctx_stopidletimer(fctx);
		if (result != ISC_R_SUCCESS)
			fctx_done(fctx, result, __LINE__);
	} else if (result == DNS_R_CHASEDSSERVERS) {
		unsigned int n;
		add_bad(fctx, addrinfo, result, broken_type);
		fctx_cancelqueries(fctx, ISC_TRUE, ISC_FALSE);
		fctx_cleanupfinds(fctx);
		fctx_cleanupforwaddrs(fctx);

		n = dns_name_countlabels(&fctx->name);
		dns_name_getlabelsequence(&fctx->name, 1, n - 1, &fctx->nsname);

		FCTXTRACE("suspending DS lookup to find parent's NS records");

		result = dns_resolver_createfetch(res, &fctx->nsname,
						  dns_rdatatype_ns,
						  NULL, NULL, NULL,
						  fctx->options, task,
						  resume_dslookup, fctx,
						  &fctx->nsrrset, NULL,
						  &fctx->nsfetch);
		if (result != ISC_R_SUCCESS)
			fctx_done(fctx, result, __LINE__);
		else {
			fctx_increference(fctx);
			result = fctx_stopidletimer(fctx);
			if (result != ISC_R_SUCCESS)
				fctx_done(fctx, result, __LINE__);
		}
	} else {
		/*
		 * We're done.
		 */
		fctx_done(fctx, result, __LINE__);
	}
}


/***
 *** Resolver Methods
 ***/
static void
destroy_badcache(dns_resolver_t *res) {
	dns_badcache_t *bad, *next;
	unsigned int i;

	if (res->badcache != NULL) {
		for (i = 0; i < res->badhash; i++)
			for (bad = res->badcache[i]; bad != NULL;
			     bad = next) {
				next = bad->next;
				isc_mem_put(res->mctx, bad, sizeof(*bad) +
					    bad->name.length);
				res->badcount--;
			}
		isc_mem_put(res->mctx, res->badcache,
			    sizeof(*res->badcache) * res->badhash);
		res->badcache = NULL;
		res->badhash = 0;
		INSIST(res->badcount == 0);
	}
}

static void
destroy(dns_resolver_t *res) {
	unsigned int i;
	alternate_t *a;

	REQUIRE(res->references == 0);
	REQUIRE(!res->priming);
	REQUIRE(res->primefetch == NULL);

	RTRACE("destroy");

	INSIST(res->nfctx == 0);

	DESTROYLOCK(&res->primelock);
	DESTROYLOCK(&res->nlock);
	DESTROYLOCK(&res->lock);
	for (i = 0; i < res->nbuckets; i++) {
		INSIST(ISC_LIST_EMPTY(res->buckets[i].fctxs));
		isc_task_shutdown(res->buckets[i].task);
		isc_task_detach(&res->buckets[i].task);
		DESTROYLOCK(&res->buckets[i].lock);
		isc_mem_detach(&res->buckets[i].mctx);
	}
	isc_mem_put(res->mctx, res->buckets,
		    res->nbuckets * sizeof(fctxbucket_t));
#ifdef ENABLE_FETCHLIMIT
	for (i = 0; i < RES_DOMAIN_BUCKETS; i++) {
		INSIST(ISC_LIST_EMPTY(res->dbuckets[i].list));
		isc_mem_detach(&res->dbuckets[i].mctx);
		DESTROYLOCK(&res->dbuckets[i].lock);
	}
	isc_mem_put(res->mctx, res->dbuckets,
		    RES_DOMAIN_BUCKETS * sizeof(zonebucket_t));
#endif /* ENABLE_FETCHLIMIT */
	if (res->dispatches4 != NULL)
		dns_dispatchset_destroy(&res->dispatches4);
	if (res->dispatches6 != NULL)
		dns_dispatchset_destroy(&res->dispatches6);
	while ((a = ISC_LIST_HEAD(res->alternates)) != NULL) {
		ISC_LIST_UNLINK(res->alternates, a, link);
		if (!a->isaddress)
			dns_name_free(&a->_u._n.name, res->mctx);
		isc_mem_put(res->mctx, a, sizeof(*a));
	}
	dns_resolver_reset_algorithms(res);
	dns_resolver_reset_ds_digests(res);
	destroy_badcache(res);
	dns_resolver_resetmustbesecure(res);
#if USE_ALGLOCK
	isc_rwlock_destroy(&res->alglock);
#endif
#if USE_MBSLOCK
	isc_rwlock_destroy(&res->mbslock);
#endif
	isc_timer_detach(&res->spillattimer);
	res->magic = 0;
	isc_mem_put(res->mctx, res, sizeof(*res));
}

static void
send_shutdown_events(dns_resolver_t *res) {
	isc_event_t *event, *next_event;
	isc_task_t *etask;

	/*
	 * Caller must be holding the resolver lock.
	 */

	for (event = ISC_LIST_HEAD(res->whenshutdown);
	     event != NULL;
	     event = next_event) {
		next_event = ISC_LIST_NEXT(event, ev_link);
		ISC_LIST_UNLINK(res->whenshutdown, event, ev_link);
		etask = event->ev_sender;
		event->ev_sender = res;
		isc_task_sendanddetach(&etask, &event);
	}
}

static void
empty_bucket(dns_resolver_t *res) {
	RTRACE("empty_bucket");

	LOCK(&res->lock);

	INSIST(res->activebuckets > 0);
	res->activebuckets--;
	if (res->activebuckets == 0)
		send_shutdown_events(res);

	UNLOCK(&res->lock);
}

static void
spillattimer_countdown(isc_task_t *task, isc_event_t *event) {
	dns_resolver_t *res = event->ev_arg;
	isc_result_t result;
	unsigned int count;
	isc_boolean_t logit = ISC_FALSE;

	REQUIRE(VALID_RESOLVER(res));

	UNUSED(task);

	LOCK(&res->lock);
	INSIST(!res->exiting);
	if (res->spillat > res->spillatmin) {
		res->spillat--;
		logit = ISC_TRUE;
	}
	if (res->spillat <= res->spillatmin) {
		result = isc_timer_reset(res->spillattimer,
					 isc_timertype_inactive, NULL,
					 NULL, ISC_TRUE);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}
	count = res->spillat;
	UNLOCK(&res->lock);
	if (logit)
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
			      DNS_LOGMODULE_RESOLVER, ISC_LOG_NOTICE,
			      "clients-per-query decreased to %u", count);

	isc_event_free(&event);
}

isc_result_t
dns_resolver_create(dns_view_t *view,
		    isc_taskmgr_t *taskmgr,
		    unsigned int ntasks, unsigned int ndisp,
		    isc_socketmgr_t *socketmgr,
		    isc_timermgr_t *timermgr,
		    unsigned int options,
		    dns_dispatchmgr_t *dispatchmgr,
		    dns_dispatch_t *dispatchv4,
		    dns_dispatch_t *dispatchv6,
		    dns_resolver_t **resp)
{
	dns_resolver_t *res;
	isc_result_t result = ISC_R_SUCCESS;
	unsigned int i, buckets_created = 0;
	isc_task_t *task = NULL;
	char name[16];
	unsigned dispattr;
#ifdef ENABLE_FETCHLIMIT
	unsigned int dbuckets_created = 0;
#endif /* ENABLE_FETCHLIMIT */

	/*
	 * Create a resolver.
	 */

	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(ntasks > 0);
	REQUIRE(ndisp > 0);
	REQUIRE(resp != NULL && *resp == NULL);
	REQUIRE(dispatchmgr != NULL);
	REQUIRE(dispatchv4 != NULL || dispatchv6 != NULL);

	res = isc_mem_get(view->mctx, sizeof(*res));
	if (res == NULL)
		return (ISC_R_NOMEMORY);
	RTRACE("create");
	res->mctx = view->mctx;
	res->rdclass = view->rdclass;
	res->socketmgr = socketmgr;
	res->timermgr = timermgr;
	res->taskmgr = taskmgr;
	res->dispatchmgr = dispatchmgr;
	res->view = view;
	res->options = options;
	res->lame_ttl = 0;
	ISC_LIST_INIT(res->alternates);
	res->udpsize = RECV_BUFFER_SIZE;
	res->algorithms = NULL;
	res->digests = NULL;
	res->badcache = NULL;
	res->badcount = 0;
	res->badhash = 0;
	res->badsweep = 0;
	res->mustbesecure = NULL;
	res->spillatmin = res->spillat = 10;
	res->spillatmax = 100;
	res->spillattimer = NULL;
	res->zspill = 0;
	res->zero_no_soa_ttl = ISC_FALSE;
	res->query_timeout = DEFAULT_QUERY_TIMEOUT;
	res->maxdepth = DEFAULT_RECURSION_DEPTH;
	res->maxqueries = DEFAULT_MAX_QUERIES;
	res->quotaresp[dns_quotatype_zone] = DNS_R_DROP;
	res->quotaresp[dns_quotatype_server] = DNS_R_SERVFAIL;
	res->nbuckets = ntasks;
	if (view->resstats != NULL)
		isc_stats_set(view->resstats, ntasks,
			      dns_resstatscounter_buckets);
	res->activebuckets = ntasks;
	res->buckets = isc_mem_get(view->mctx,
				   ntasks * sizeof(fctxbucket_t));
	if (res->buckets == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup_res;
	}
	for (i = 0; i < ntasks; i++) {
		result = isc_mutex_init(&res->buckets[i].lock);
		if (result != ISC_R_SUCCESS)
			goto cleanup_buckets;
		res->buckets[i].task = NULL;
		result = isc_task_create(taskmgr, 0, &res->buckets[i].task);
		if (result != ISC_R_SUCCESS) {
			DESTROYLOCK(&res->buckets[i].lock);
			goto cleanup_buckets;
		}
		res->buckets[i].mctx = NULL;
		snprintf(name, sizeof(name), "res%u", i);
#ifdef ISC_PLATFORM_USETHREADS
		/*
		 * Use a separate memory context for each bucket to reduce
		 * contention among multiple threads.  Do this only when
		 * enabling threads because it will be require more memory.
		 */
		result = isc_mem_create(0, 0, &res->buckets[i].mctx);
		if (result != ISC_R_SUCCESS) {
			isc_task_detach(&res->buckets[i].task);
			DESTROYLOCK(&res->buckets[i].lock);
			goto cleanup_buckets;
		}
		isc_mem_setname(res->buckets[i].mctx, name, NULL);
#else
		isc_mem_attach(view->mctx, &res->buckets[i].mctx);
#endif
		isc_task_setname(res->buckets[i].task, name, res);
		ISC_LIST_INIT(res->buckets[i].fctxs);
		res->buckets[i].exiting = ISC_FALSE;
		buckets_created++;
	}

#ifdef ENABLE_FETCHLIMIT
	res->dbuckets = isc_mem_get(view->mctx,
				    RES_DOMAIN_BUCKETS * sizeof(zonebucket_t));
	if (res->dbuckets == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup_buckets;
	}
	for (i = 0; i < RES_DOMAIN_BUCKETS; i++) {
		ISC_LIST_INIT(res->dbuckets[i].list);
		res->dbuckets[i].mctx = NULL;
		isc_mem_attach(view->mctx, &res->dbuckets[i].mctx);
		result = isc_mutex_init(&res->dbuckets[i].lock);
		if (result != ISC_R_SUCCESS) {
			isc_mem_detach(&res->dbuckets[i].mctx);
			goto cleanup_dbuckets;
		}
		dbuckets_created++;
	}
#endif /* ENABLE_FETCHLIMIT */

	res->dispatches4 = NULL;
	if (dispatchv4 != NULL) {
		dns_dispatchset_create(view->mctx, socketmgr, taskmgr,
				       dispatchv4, &res->dispatches4, ndisp);
		dispattr = dns_dispatch_getattributes(dispatchv4);
		res->exclusivev4 =
			ISC_TF((dispattr & DNS_DISPATCHATTR_EXCLUSIVE) != 0);
	}

	res->dispatches6 = NULL;
	if (dispatchv6 != NULL) {
		dns_dispatchset_create(view->mctx, socketmgr, taskmgr,
				       dispatchv6, &res->dispatches6, ndisp);
		dispattr = dns_dispatch_getattributes(dispatchv6);
		res->exclusivev6 =
			ISC_TF((dispattr & DNS_DISPATCHATTR_EXCLUSIVE) != 0);
	}

	res->querydscp4 = -1;
	res->querydscp6 = -1;
	res->references = 1;
	res->exiting = ISC_FALSE;
	res->frozen = ISC_FALSE;
	ISC_LIST_INIT(res->whenshutdown);
	res->priming = ISC_FALSE;
	res->primefetch = NULL;
	res->nfctx = 0;

	result = isc_mutex_init(&res->lock);
	if (result != ISC_R_SUCCESS)
		goto cleanup_dispatches;

	result = isc_mutex_init(&res->nlock);
	if (result != ISC_R_SUCCESS)
		goto cleanup_lock;

	result = isc_mutex_init(&res->primelock);
	if (result != ISC_R_SUCCESS)
		goto cleanup_nlock;

	task = NULL;
	result = isc_task_create(taskmgr, 0, &task);
	if (result != ISC_R_SUCCESS)
		goto cleanup_primelock;

	result = isc_timer_create(timermgr, isc_timertype_inactive, NULL, NULL,
				  task, spillattimer_countdown, res,
				  &res->spillattimer);
	isc_task_detach(&task);
	if (result != ISC_R_SUCCESS)
		goto cleanup_primelock;

#if USE_ALGLOCK
	result = isc_rwlock_init(&res->alglock, 0, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup_spillattimer;
#endif
#if USE_MBSLOCK
	result = isc_rwlock_init(&res->mbslock, 0, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup_alglock;
#endif

	res->magic = RES_MAGIC;

	*resp = res;

	return (ISC_R_SUCCESS);

#if USE_MBSLOCK
 cleanup_alglock:
#if USE_ALGLOCK
	isc_rwlock_destroy(&res->alglock);
#endif
#endif
#if USE_ALGLOCK || USE_MBSLOCK
 cleanup_spillattimer:
	isc_timer_detach(&res->spillattimer);
#endif

 cleanup_primelock:
	DESTROYLOCK(&res->primelock);

 cleanup_nlock:
	DESTROYLOCK(&res->nlock);

 cleanup_lock:
	DESTROYLOCK(&res->lock);

 cleanup_dispatches:
	if (res->dispatches6 != NULL)
		dns_dispatchset_destroy(&res->dispatches6);
	if (res->dispatches4 != NULL)
		dns_dispatchset_destroy(&res->dispatches4);

#ifdef ENABLE_FETCHLIMIT
 cleanup_dbuckets:
	for (i = 0; i < dbuckets_created; i++) {
		DESTROYLOCK(&res->dbuckets[i].lock);
		isc_mem_detach(&res->dbuckets[i].mctx);
	}
	isc_mem_put(view->mctx, res->dbuckets,
		    RES_DOMAIN_BUCKETS * sizeof(zonebucket_t));
#endif /* ENABLE_FETCHLIMIT*/

 cleanup_buckets:
	for (i = 0; i < buckets_created; i++) {
		isc_mem_detach(&res->buckets[i].mctx);
		DESTROYLOCK(&res->buckets[i].lock);
		isc_task_shutdown(res->buckets[i].task);
		isc_task_detach(&res->buckets[i].task);
	}
	isc_mem_put(view->mctx, res->buckets,
		    res->nbuckets * sizeof(fctxbucket_t));

 cleanup_res:
	isc_mem_put(view->mctx, res, sizeof(*res));

	return (result);
}

static void
prime_done(isc_task_t *task, isc_event_t *event) {
	dns_resolver_t *res;
	dns_fetchevent_t *fevent;
	dns_fetch_t *fetch;
	dns_db_t *db = NULL;

	REQUIRE(event->ev_type == DNS_EVENT_FETCHDONE);
	fevent = (dns_fetchevent_t *)event;
	res = event->ev_arg;
	REQUIRE(VALID_RESOLVER(res));

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
		      DNS_LOGMODULE_RESOLVER, ISC_LOG_INFO,
		      "resolver priming query complete");

	UNUSED(task);

	LOCK(&res->lock);

	INSIST(res->priming);
	res->priming = ISC_FALSE;
	LOCK(&res->primelock);
	fetch = res->primefetch;
	res->primefetch = NULL;
	UNLOCK(&res->primelock);

	UNLOCK(&res->lock);

	if (fevent->result == ISC_R_SUCCESS &&
	    res->view->cache != NULL && res->view->hints != NULL) {
		dns_cache_attachdb(res->view->cache, &db);
		dns_root_checkhints(res->view, res->view->hints, db);
		dns_db_detach(&db);
	}

	if (fevent->node != NULL)
		dns_db_detachnode(fevent->db, &fevent->node);
	if (fevent->db != NULL)
		dns_db_detach(&fevent->db);
	if (dns_rdataset_isassociated(fevent->rdataset))
		dns_rdataset_disassociate(fevent->rdataset);
	INSIST(fevent->sigrdataset == NULL);

	isc_mem_put(res->mctx, fevent->rdataset, sizeof(*fevent->rdataset));

	isc_event_free(&event);
	dns_resolver_destroyfetch(&fetch);
}

void
dns_resolver_prime(dns_resolver_t *res) {
	isc_boolean_t want_priming = ISC_FALSE;
	dns_rdataset_t *rdataset;
	isc_result_t result;

	REQUIRE(VALID_RESOLVER(res));
	REQUIRE(res->frozen);

	RTRACE("dns_resolver_prime");

	LOCK(&res->lock);

	if (!res->exiting && !res->priming) {
		INSIST(res->primefetch == NULL);
		res->priming = ISC_TRUE;
		want_priming = ISC_TRUE;
	}

	UNLOCK(&res->lock);

	if (want_priming) {
		/*
		 * To avoid any possible recursive locking problems, we
		 * start the priming fetch like any other fetch, and holding
		 * no resolver locks.  No one else will try to start it
		 * because we're the ones who set res->priming to true.
		 * Any other callers of dns_resolver_prime() while we're
		 * running will see that res->priming is already true and
		 * do nothing.
		 */
		RTRACE("priming");
		rdataset = isc_mem_get(res->mctx, sizeof(*rdataset));
		if (rdataset == NULL) {
			LOCK(&res->lock);
			INSIST(res->priming);
			INSIST(res->primefetch == NULL);
			res->priming = ISC_FALSE;
			UNLOCK(&res->lock);
			return;
		}
		dns_rdataset_init(rdataset);
		LOCK(&res->primelock);
		result = dns_resolver_createfetch(res, dns_rootname,
						  dns_rdatatype_ns,
						  NULL, NULL, NULL, 0,
						  res->buckets[0].task,
						  prime_done,
						  res, rdataset, NULL,
						  &res->primefetch);
		UNLOCK(&res->primelock);
		if (result != ISC_R_SUCCESS) {
			isc_mem_put(res->mctx, rdataset, sizeof(*rdataset));
			LOCK(&res->lock);
			INSIST(res->priming);
			res->priming = ISC_FALSE;
			UNLOCK(&res->lock);
		}
	}
}

void
dns_resolver_freeze(dns_resolver_t *res) {
	/*
	 * Freeze resolver.
	 */

	REQUIRE(VALID_RESOLVER(res));

	res->frozen = ISC_TRUE;
}

void
dns_resolver_attach(dns_resolver_t *source, dns_resolver_t **targetp) {
	REQUIRE(VALID_RESOLVER(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	RRTRACE(source, "attach");
	LOCK(&source->lock);
	REQUIRE(!source->exiting);

	INSIST(source->references > 0);
	source->references++;
	INSIST(source->references != 0);
	UNLOCK(&source->lock);

	*targetp = source;
}

void
dns_resolver_whenshutdown(dns_resolver_t *res, isc_task_t *task,
			  isc_event_t **eventp)
{
	isc_task_t *tclone;
	isc_event_t *event;

	REQUIRE(VALID_RESOLVER(res));
	REQUIRE(eventp != NULL);

	event = *eventp;
	*eventp = NULL;

	LOCK(&res->lock);

	if (res->exiting && res->activebuckets == 0) {
		/*
		 * We're already shutdown.  Send the event.
		 */
		event->ev_sender = res;
		isc_task_send(task, &event);
	} else {
		tclone = NULL;
		isc_task_attach(task, &tclone);
		event->ev_sender = tclone;
		ISC_LIST_APPEND(res->whenshutdown, event, ev_link);
	}

	UNLOCK(&res->lock);
}

void
dns_resolver_shutdown(dns_resolver_t *res) {
	unsigned int i;
	fetchctx_t *fctx;
	isc_result_t result;

	REQUIRE(VALID_RESOLVER(res));

	RTRACE("shutdown");

	LOCK(&res->lock);

	if (!res->exiting) {
		RTRACE("exiting");
		res->exiting = ISC_TRUE;

		for (i = 0; i < res->nbuckets; i++) {
			LOCK(&res->buckets[i].lock);
			for (fctx = ISC_LIST_HEAD(res->buckets[i].fctxs);
			     fctx != NULL;
			     fctx = ISC_LIST_NEXT(fctx, link))
				fctx_shutdown(fctx);
			if (res->dispatches4 != NULL && !res->exclusivev4) {
				dns_dispatchset_cancelall(res->dispatches4,
							  res->buckets[i].task);
			}
			if (res->dispatches6 != NULL && !res->exclusivev6) {
				dns_dispatchset_cancelall(res->dispatches6,
							  res->buckets[i].task);
			}
			res->buckets[i].exiting = ISC_TRUE;
			if (ISC_LIST_EMPTY(res->buckets[i].fctxs)) {
				INSIST(res->activebuckets > 0);
				res->activebuckets--;
			}
			UNLOCK(&res->buckets[i].lock);
		}
		if (res->activebuckets == 0)
			send_shutdown_events(res);
		result = isc_timer_reset(res->spillattimer,
					 isc_timertype_inactive, NULL,
					 NULL, ISC_TRUE);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}

	UNLOCK(&res->lock);
}

void
dns_resolver_detach(dns_resolver_t **resp) {
	dns_resolver_t *res;
	isc_boolean_t need_destroy = ISC_FALSE;

	REQUIRE(resp != NULL);
	res = *resp;
	REQUIRE(VALID_RESOLVER(res));

	RTRACE("detach");

	LOCK(&res->lock);

	INSIST(res->references > 0);
	res->references--;
	if (res->references == 0) {
		INSIST(res->exiting && res->activebuckets == 0);
		need_destroy = ISC_TRUE;
	}

	UNLOCK(&res->lock);

	if (need_destroy)
		destroy(res);

	*resp = NULL;
}

static inline isc_boolean_t
fctx_match(fetchctx_t *fctx, dns_name_t *name, dns_rdatatype_t type,
	   unsigned int options)
{
	/*
	 * Don't match fetch contexts that are shutting down.
	 */
	if (fctx->cloned || fctx->state == fetchstate_done ||
	    ISC_LIST_EMPTY(fctx->events))
		return (ISC_FALSE);

	if (fctx->type != type || fctx->options != options)
		return (ISC_FALSE);
	return (dns_name_equal(&fctx->name, name));
}

static inline void
log_fetch(dns_name_t *name, dns_rdatatype_t type) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	int level = ISC_LOG_DEBUG(1);

	/*
	 * If there's no chance of logging it, don't render (format) the
	 * name and RDATA type (further below), and return early.
	 */
	if (! isc_log_wouldlog(dns_lctx, level))
		return;

	dns_name_format(name, namebuf, sizeof(namebuf));
	dns_rdatatype_format(type, typebuf, sizeof(typebuf));

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
		      DNS_LOGMODULE_RESOLVER, level,
		      "fetch: %s/%s", namebuf, typebuf);
}

isc_result_t
dns_resolver_createfetch(dns_resolver_t *res, dns_name_t *name,
			 dns_rdatatype_t type,
			 dns_name_t *domain, dns_rdataset_t *nameservers,
			 dns_forwarders_t *forwarders,
			 unsigned int options, isc_task_t *task,
			 isc_taskaction_t action, void *arg,
			 dns_rdataset_t *rdataset,
			 dns_rdataset_t *sigrdataset,
			 dns_fetch_t **fetchp)
{
	return (dns_resolver_createfetch3(res, name, type, domain,
					  nameservers, forwarders, NULL, 0,
					  options, 0, NULL, task, action, arg,
					  rdataset, sigrdataset, fetchp));
}

isc_result_t
dns_resolver_createfetch2(dns_resolver_t *res, dns_name_t *name,
			  dns_rdatatype_t type,
			  dns_name_t *domain, dns_rdataset_t *nameservers,
			  dns_forwarders_t *forwarders,
			  isc_sockaddr_t *client, dns_messageid_t id,
			  unsigned int options, isc_task_t *task,
			  isc_taskaction_t action, void *arg,
			  dns_rdataset_t *rdataset,
			  dns_rdataset_t *sigrdataset,
			  dns_fetch_t **fetchp)
{
	return (dns_resolver_createfetch3(res, name, type, domain,
					  nameservers, forwarders, client, id,
					  options, 0, NULL, task, action, arg,
					  rdataset, sigrdataset, fetchp));
}

isc_result_t
dns_resolver_createfetch3(dns_resolver_t *res, dns_name_t *name,
			  dns_rdatatype_t type,
			  dns_name_t *domain, dns_rdataset_t *nameservers,
			  dns_forwarders_t *forwarders,
			  isc_sockaddr_t *client, dns_messageid_t id,
			  unsigned int options, unsigned int depth,
			  isc_counter_t *qc, isc_task_t *task,
			  isc_taskaction_t action, void *arg,
			  dns_rdataset_t *rdataset,
			  dns_rdataset_t *sigrdataset,
			  dns_fetch_t **fetchp)
{
	dns_fetch_t *fetch;
	fetchctx_t *fctx = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	unsigned int bucketnum;
	isc_boolean_t new_fctx = ISC_FALSE;
	isc_event_t *event;
	unsigned int count = 0;
	unsigned int spillat;
	unsigned int spillatmin;
	isc_boolean_t dodestroy = ISC_FALSE;

	UNUSED(forwarders);

	REQUIRE(VALID_RESOLVER(res));
	REQUIRE(res->frozen);
	/* XXXRTH  Check for meta type */
	if (domain != NULL) {
		REQUIRE(DNS_RDATASET_VALID(nameservers));
		REQUIRE(nameservers->type == dns_rdatatype_ns);
	} else
		REQUIRE(nameservers == NULL);
	REQUIRE(forwarders == NULL);
	REQUIRE(!dns_rdataset_isassociated(rdataset));
	REQUIRE(sigrdataset == NULL ||
		!dns_rdataset_isassociated(sigrdataset));
	REQUIRE(fetchp != NULL && *fetchp == NULL);

	log_fetch(name, type);

	/*
	 * XXXRTH  use a mempool?
	 */
	fetch = isc_mem_get(res->mctx, sizeof(*fetch));
	if (fetch == NULL)
		return (ISC_R_NOMEMORY);
	fetch->mctx = NULL;
	isc_mem_attach(res->mctx, &fetch->mctx);

	bucketnum = dns_name_fullhash(name, ISC_FALSE) % res->nbuckets;

	LOCK(&res->lock);
	spillat = res->spillat;
	spillatmin = res->spillatmin;
	UNLOCK(&res->lock);
	LOCK(&res->buckets[bucketnum].lock);

	if (res->buckets[bucketnum].exiting) {
		result = ISC_R_SHUTTINGDOWN;
		goto unlock;
	}

	if ((options & DNS_FETCHOPT_UNSHARED) == 0) {
		for (fctx = ISC_LIST_HEAD(res->buckets[bucketnum].fctxs);
		     fctx != NULL;
		     fctx = ISC_LIST_NEXT(fctx, link)) {
			if (fctx_match(fctx, name, type, options))
				break;
		}
	}

	/*
	 * Is this a duplicate?
	 */
	if (fctx != NULL && client != NULL) {
		dns_fetchevent_t *fevent;
		for (fevent = ISC_LIST_HEAD(fctx->events);
		     fevent != NULL;
		     fevent = ISC_LIST_NEXT(fevent, ev_link)) {
			if (fevent->client != NULL && fevent->id == id &&
			    isc_sockaddr_equal(fevent->client, client)) {
				result = DNS_R_DUPLICATE;
				goto unlock;
			}
			count++;
		}
	}
	if (count >= spillatmin && spillatmin != 0) {
		INSIST(fctx != NULL);
		if (count >= spillat)
			fctx->spilled = ISC_TRUE;
		if (fctx->spilled) {
			result = DNS_R_DROP;
			goto unlock;
		}
	}

	if (fctx == NULL) {
		result = fctx_create(res, name, type, domain, nameservers,
				     options, bucketnum, depth, qc, &fctx);
		if (result != ISC_R_SUCCESS)
			goto unlock;
		new_fctx = ISC_TRUE;
	} else if (fctx->depth > depth)
		fctx->depth = depth;

	result = fctx_join(fctx, task, client, id, action, arg,
			   rdataset, sigrdataset, fetch);
	if (new_fctx) {
		if (result == ISC_R_SUCCESS) {
			/*
			 * Launch this fctx.
			 */
			event = &fctx->control_event;
			ISC_EVENT_INIT(event, sizeof(*event), 0, NULL,
				       DNS_EVENT_FETCHCONTROL,
				       fctx_start, fctx, NULL,
				       NULL, NULL);
			isc_task_send(res->buckets[bucketnum].task, &event);
		} else {
			/*
			 * We don't care about the result of fctx_unlink()
			 * since we know we're not exiting.
			 */
			(void)fctx_unlink(fctx);
			dodestroy = ISC_TRUE;
		}
	}

 unlock:
	UNLOCK(&res->buckets[bucketnum].lock);

	if (dodestroy)
		fctx_destroy(fctx);

	if (result == ISC_R_SUCCESS) {
		FTRACE("created");
		*fetchp = fetch;
	} else
		isc_mem_putanddetach(&fetch->mctx, fetch, sizeof(*fetch));

	return (result);
}

void
dns_resolver_cancelfetch(dns_fetch_t *fetch) {
	fetchctx_t *fctx;
	dns_resolver_t *res;
	dns_fetchevent_t *event, *next_event;
	isc_task_t *etask;

	REQUIRE(DNS_FETCH_VALID(fetch));
	fctx = fetch->private;
	REQUIRE(VALID_FCTX(fctx));
	res = fctx->res;

	FTRACE("cancelfetch");

	LOCK(&res->buckets[fctx->bucketnum].lock);

	/*
	 * Find the completion event for this fetch (as opposed
	 * to those for other fetches that have joined the same
	 * fctx) and send it with result = ISC_R_CANCELED.
	 */
	event = NULL;
	if (fctx->state != fetchstate_done) {
		for (event = ISC_LIST_HEAD(fctx->events);
		     event != NULL;
		     event = next_event) {
			next_event = ISC_LIST_NEXT(event, ev_link);
			if (event->fetch == fetch) {
				ISC_LIST_UNLINK(fctx->events, event, ev_link);
				break;
			}
		}
	}
	if (event != NULL) {
		etask = event->ev_sender;
		event->ev_sender = fctx;
		event->result = ISC_R_CANCELED;
		isc_task_sendanddetach(&etask, ISC_EVENT_PTR(&event));
	}
	/*
	 * The fctx continues running even if no fetches remain;
	 * the answer is still cached.
	 */

	UNLOCK(&res->buckets[fctx->bucketnum].lock);
}

void
dns_resolver_destroyfetch(dns_fetch_t **fetchp) {
	dns_fetch_t *fetch;
	dns_resolver_t *res;
	dns_fetchevent_t *event, *next_event;
	fetchctx_t *fctx;
	unsigned int bucketnum;
	isc_boolean_t bucket_empty;

	REQUIRE(fetchp != NULL);
	fetch = *fetchp;
	REQUIRE(DNS_FETCH_VALID(fetch));
	fctx = fetch->private;
	REQUIRE(VALID_FCTX(fctx));
	res = fctx->res;

	FTRACE("destroyfetch");

	bucketnum = fctx->bucketnum;
	LOCK(&res->buckets[bucketnum].lock);

	/*
	 * Sanity check: the caller should have gotten its event before
	 * trying to destroy the fetch.
	 */
	event = NULL;
	if (fctx->state != fetchstate_done) {
		for (event = ISC_LIST_HEAD(fctx->events);
		     event != NULL;
		     event = next_event) {
			next_event = ISC_LIST_NEXT(event, ev_link);
			RUNTIME_CHECK(event->fetch != fetch);
		}
	}

	bucket_empty = fctx_decreference(fctx);

	UNLOCK(&res->buckets[bucketnum].lock);

	isc_mem_putanddetach(&fetch->mctx, fetch, sizeof(*fetch));
	*fetchp = NULL;

	if (bucket_empty)
		empty_bucket(res);
}

void
dns_resolver_logfetch(dns_fetch_t *fetch, isc_log_t *lctx,
		      isc_logcategory_t *category, isc_logmodule_t *module,
		      int level, isc_boolean_t duplicateok)
{
	fetchctx_t *fctx;
	dns_resolver_t *res;
	char domainbuf[DNS_NAME_FORMATSIZE];

	REQUIRE(DNS_FETCH_VALID(fetch));
	fctx = fetch->private;
	REQUIRE(VALID_FCTX(fctx));
	res = fctx->res;

	LOCK(&res->buckets[fctx->bucketnum].lock);

	INSIST(fctx->exitline >= 0);
	if (!fctx->logged || duplicateok) {
		dns_name_format(&fctx->domain, domainbuf, sizeof(domainbuf));
		isc_log_write(lctx, category, module, level,
			      "fetch completed at %s:%d for %s in "
			      "%" ISC_PRINT_QUADFORMAT "u."
			      "%06" ISC_PRINT_QUADFORMAT "u: %s/%s "
			      "[domain:%s,referral:%u,restart:%u,qrysent:%u,"
			      "timeout:%u,lame:%u,"
#ifdef ENABLE_FETCHLIMIT
			      "quota:%u,"
#endif /* ENABLE_FETCHLIMIT */
			      "neterr:%u,"
			      "badresp:%u,adberr:%u,findfail:%u,valfail:%u]",
			      __FILE__, fctx->exitline, fctx->info,
			      fctx->duration / US_PER_SEC,
			      fctx->duration % US_PER_SEC,
			      isc_result_totext(fctx->result),
			      isc_result_totext(fctx->vresult), domainbuf,
			      fctx->referrals, fctx->restarts,
			      fctx->querysent, fctx->timeouts, fctx->lamecount,
#ifdef ENABLE_FETCHLIMIT
			      fctx->quotacount,
#endif /* ENABLE_FETCHLIMIT */
			      fctx->neterr, fctx->badresp, fctx->adberr,
			      fctx->findfail, fctx->valfail);
		fctx->logged = ISC_TRUE;
	}

	UNLOCK(&res->buckets[fctx->bucketnum].lock);
}

dns_dispatchmgr_t *
dns_resolver_dispatchmgr(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->dispatchmgr);
}

dns_dispatch_t *
dns_resolver_dispatchv4(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (dns_dispatchset_get(resolver->dispatches4));
}

dns_dispatch_t *
dns_resolver_dispatchv6(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (dns_dispatchset_get(resolver->dispatches6));
}

isc_socketmgr_t *
dns_resolver_socketmgr(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->socketmgr);
}

isc_taskmgr_t *
dns_resolver_taskmgr(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->taskmgr);
}

isc_uint32_t
dns_resolver_getlamettl(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->lame_ttl);
}

void
dns_resolver_setlamettl(dns_resolver_t *resolver, isc_uint32_t lame_ttl) {
	REQUIRE(VALID_RESOLVER(resolver));
	resolver->lame_ttl = lame_ttl;
}

unsigned int
dns_resolver_nrunning(dns_resolver_t *resolver) {
	unsigned int n;
	LOCK(&resolver->nlock);
	n = resolver->nfctx;
	UNLOCK(&resolver->nlock);
	return (n);
}

isc_result_t
dns_resolver_addalternate(dns_resolver_t *resolver, isc_sockaddr_t *alt,
			  dns_name_t *name, in_port_t port) {
	alternate_t *a;
	isc_result_t result;

	REQUIRE(VALID_RESOLVER(resolver));
	REQUIRE(!resolver->frozen);
	REQUIRE((alt == NULL) ^ (name == NULL));

	a = isc_mem_get(resolver->mctx, sizeof(*a));
	if (a == NULL)
		return (ISC_R_NOMEMORY);
	if (alt != NULL) {
		a->isaddress = ISC_TRUE;
		a->_u.addr = *alt;
	} else {
		a->isaddress = ISC_FALSE;
		a->_u._n.port = port;
		dns_name_init(&a->_u._n.name, NULL);
		result = dns_name_dup(name, resolver->mctx, &a->_u._n.name);
		if (result != ISC_R_SUCCESS) {
			isc_mem_put(resolver->mctx, a, sizeof(*a));
			return (result);
		}
	}
	ISC_LINK_INIT(a, link);
	ISC_LIST_APPEND(resolver->alternates, a, link);

	return (ISC_R_SUCCESS);
}

void
dns_resolver_setudpsize(dns_resolver_t *resolver, isc_uint16_t udpsize) {
	REQUIRE(VALID_RESOLVER(resolver));
	resolver->udpsize = udpsize;
}

isc_uint16_t
dns_resolver_getudpsize(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->udpsize);
}

void
dns_resolver_flushbadcache(dns_resolver_t *resolver, dns_name_t *name) {
	unsigned int i;
	dns_badcache_t *bad, *prev, *next;

	/*
	 * Drop all entries that match the name, and also all expired
	 * entries from the badcache.
	 */

	REQUIRE(VALID_RESOLVER(resolver));

	LOCK(&resolver->lock);
	if (resolver->badcache == NULL)
		goto unlock;

	if (name != NULL) {
		isc_time_t now;
		isc_result_t result;
		result = isc_time_now(&now);
		if (result != ISC_R_SUCCESS)
			isc_time_settoepoch(&now);
		i = dns_name_hash(name, ISC_FALSE) % resolver->badhash;
		prev = NULL;
		for (bad = resolver->badcache[i]; bad != NULL; bad = next) {
			int n;
			next = bad->next;
			n = isc_time_compare(&bad->expire, &now);
			if (n < 0 || dns_name_equal(name, &bad->name)) {
				if (prev == NULL)
					resolver->badcache[i] = bad->next;
				else
					prev->next = bad->next;
				isc_mem_put(resolver->mctx, bad, sizeof(*bad) +
					    bad->name.length);
				resolver->badcount--;
			} else
				prev = bad;
		}
	} else
		destroy_badcache(resolver);

 unlock:
	UNLOCK(&resolver->lock);
}

void
dns_resolver_flushbadnames(dns_resolver_t *resolver, dns_name_t *name) {
	dns_badcache_t *bad, *prev, *next;
	unsigned int i;
	int n;
	isc_time_t now;
	isc_result_t result;

	/* Drop all expired entries from the badcache. */

	REQUIRE(VALID_RESOLVER(resolver));
	REQUIRE(name != NULL);

	LOCK(&resolver->lock);
	if (resolver->badcache == NULL)
		goto unlock;

	result = isc_time_now(&now);
	if (result != ISC_R_SUCCESS)
		isc_time_settoepoch(&now);

	for (i = 0; i < resolver->badhash; i++) {
		prev = NULL;
		for (bad = resolver->badcache[i]; bad != NULL; bad = next) {
			next = bad->next;
			n = isc_time_compare(&bad->expire, &now);
			if (n < 0 || dns_name_issubdomain(&bad->name, name)) {
				if (prev == NULL)
					resolver->badcache[i] = bad->next;
				else
					prev->next = bad->next;
				isc_mem_put(resolver->mctx, bad, sizeof(*bad) +
					    bad->name.length);
				resolver->badcount--;
			} else
				prev = bad;
		}
	}

 unlock:
	UNLOCK(&resolver->lock);
}

static void
resizehash(dns_resolver_t *resolver, isc_time_t *now, isc_boolean_t grow) {
	unsigned int newsize;
	dns_badcache_t **new, *bad, *next;
	unsigned int i;

	/*
	 * The number of buckets in the hashtable is modified in this
	 * function. Afterwards, all the entries are remapped into the
	 * corresponding new slot. Rehashing (hash computation) is
	 * unnecessary as the hash values had been saved.
	 */

	if (grow)
		newsize = resolver->badhash * 2 + 1;
	else
		newsize = (resolver->badhash - 1) / 2;

	new = isc_mem_get(resolver->mctx,
			  sizeof(*resolver->badcache) * newsize);
	if (new == NULL)
		return;
	memset(new, 0, sizeof(*resolver->badcache) * newsize);

	/*
	 * Because the hashtable implements a simple modulus mapping
	 * from hash to bucket (no extendible hashing is used), every
	 * name in the hashtable has to be remapped to its new slot.
	 * Entries that have expired (time) are dropped.
	 */
	for (i = 0; i < resolver->badhash; i++) {
		for (bad = resolver->badcache[i]; bad != NULL; bad = next) {
			next = bad->next;
			if (isc_time_compare(&bad->expire, now) < 0) {
				isc_mem_put(resolver->mctx, bad, sizeof(*bad) +
					    bad->name.length);
				resolver->badcount--;
			} else {
				bad->next = new[bad->hashval % newsize];
				new[bad->hashval % newsize] = bad;
			}
		}
	}
	isc_mem_put(resolver->mctx, resolver->badcache,
		    sizeof(*resolver->badcache) * resolver->badhash);
	resolver->badhash = newsize;
	resolver->badcache = new;
}

void
dns_resolver_addbadcache(dns_resolver_t *resolver, dns_name_t *name,
			 dns_rdatatype_t type, isc_time_t *expire)
{
	isc_time_t now;
	isc_result_t result = ISC_R_SUCCESS;
	unsigned int i, hashval;
	dns_badcache_t *bad, *prev, *next;

	/*
	 * The badcache is implemented as a hashtable keyed on the name,
	 * and each bucket slot points to a linked list (separate
	 * chaining).
	 *
	 * To avoid long list chains, if the number of entries in the
	 * hashtable goes over number-of-buckets * 8, the
	 * number-of-buckets is doubled. Similarly, if the number of
	 * entries goes below number-of-buckets * 2, the number-of-buckets
	 * is halved. See resizehash().
	 */

	REQUIRE(VALID_RESOLVER(resolver));

	LOCK(&resolver->lock);
	if (resolver->badcache == NULL) {
		resolver->badcache = isc_mem_get(resolver->mctx,
						 sizeof(*resolver->badcache) *
						 DNS_BADCACHE_SIZE);
		if (resolver->badcache == NULL)
			goto cleanup;
		resolver->badhash = DNS_BADCACHE_SIZE;
		memset(resolver->badcache, 0, sizeof(*resolver->badcache) *
		       resolver->badhash);
	}

	result = isc_time_now(&now);
	if (result != ISC_R_SUCCESS)
		isc_time_settoepoch(&now);
	hashval = dns_name_hash(name, ISC_FALSE);
	i = hashval % resolver->badhash;
	prev = NULL;
	for (bad = resolver->badcache[i]; bad != NULL; bad = next) {
		next = bad->next;
		if (bad->type == type && dns_name_equal(name, &bad->name))
			break;
		/* Drop expired entries when walking the chain. */
		if (isc_time_compare(&bad->expire, &now) < 0) {
			if (prev == NULL)
				resolver->badcache[i] = bad->next;
			else
				prev->next = bad->next;
			isc_mem_put(resolver->mctx, bad, sizeof(*bad) +
				    bad->name.length);
			resolver->badcount--;
		} else
			prev = bad;
	}
	if (bad == NULL) {
		/*
		 * Insert the name into the badcache hashtable at the
		 * head of the linked list at the appropriate slot. The
		 * name data follows right after the allocation for the
		 * linked list node.
		 */
		isc_buffer_t buffer;
		bad = isc_mem_get(resolver->mctx, sizeof(*bad) + name->length);
		if (bad == NULL)
			goto cleanup;
		bad->type = type;
		bad->hashval = hashval;
		bad->expire = *expire;
		isc_buffer_init(&buffer, bad + 1, name->length);
		dns_name_init(&bad->name, NULL);
		dns_name_copy(name, &bad->name, &buffer);
		bad->next = resolver->badcache[i];
		resolver->badcache[i] = bad;
		resolver->badcount++;
		if (resolver->badcount > resolver->badhash * 8)
			resizehash(resolver, &now, ISC_TRUE);
		if (resolver->badcount < resolver->badhash * 2 &&
		    resolver->badhash > DNS_BADCACHE_SIZE)
			resizehash(resolver, &now, ISC_FALSE);
	} else
		bad->expire = *expire;
 cleanup:
	UNLOCK(&resolver->lock);
}

isc_boolean_t
dns_resolver_getbadcache(dns_resolver_t *resolver, dns_name_t *name,
			 dns_rdatatype_t type, isc_time_t *now)
{
	dns_badcache_t *bad, *prev, *next;
	isc_boolean_t answer = ISC_FALSE;
	unsigned int i;

	REQUIRE(VALID_RESOLVER(resolver));

	LOCK(&resolver->lock);
	if (resolver->badcache == NULL)
		goto unlock;

	i = dns_name_hash(name, ISC_FALSE) % resolver->badhash;
	prev = NULL;
	for (bad = resolver->badcache[i]; bad != NULL; bad = next) {
		next = bad->next;
		/*
		 * Search the hash list. Clean out expired records as we go.
		 */
		if (isc_time_compare(&bad->expire, now) < 0) {
			if (prev != NULL)
				prev->next = bad->next;
			else
				resolver->badcache[i] = bad->next;
			isc_mem_put(resolver->mctx, bad, sizeof(*bad) +
				    bad->name.length);
			resolver->badcount--;
			continue;
		}
		if (bad->type == type && dns_name_equal(name, &bad->name)) {
			answer = ISC_TRUE;
			break;
		}
		prev = bad;
	}

	/*
	 * Slow sweep to clean out stale records.
	 */
	i = resolver->badsweep++ % resolver->badhash;
	bad = resolver->badcache[i];
	if (bad != NULL && isc_time_compare(&bad->expire, now) < 0) {
		resolver->badcache[i] = bad->next;
		isc_mem_put(resolver->mctx, bad, sizeof(*bad) +
			    bad->name.length);
		resolver->badcount--;
	}

 unlock:
	UNLOCK(&resolver->lock);
	return (answer);
}

void
dns_resolver_printbadcache(dns_resolver_t *resolver, FILE *fp) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	dns_badcache_t *bad, *next, *prev;
	isc_time_t now;
	unsigned int i;
	isc_uint64_t t;

	LOCK(&resolver->lock);
	fprintf(fp, ";\n; Bad cache\n;\n");

	if (resolver->badcache == NULL)
		goto unlock;

	TIME_NOW(&now);
	for (i = 0; i < resolver->badhash; i++) {
		prev = NULL;
		for (bad = resolver->badcache[i]; bad != NULL; bad = next) {
			next = bad->next;
			if (isc_time_compare(&bad->expire, &now) < 0) {
				if (prev != NULL)
					prev->next = bad->next;
				else
					resolver->badcache[i] = bad->next;
				isc_mem_put(resolver->mctx, bad, sizeof(*bad) +
					    bad->name.length);
				resolver->badcount--;
				continue;
			}
			prev = bad;
			dns_name_format(&bad->name, namebuf, sizeof(namebuf));
			dns_rdatatype_format(bad->type, typebuf,
					     sizeof(typebuf));
			t = isc_time_microdiff(&bad->expire, &now);
			t /= 1000;
			fprintf(fp, "; %s/%s [ttl "
				"%" ISC_PLATFORM_QUADFORMAT "u]\n",
				namebuf, typebuf, t);
		}
	}

 unlock:
	UNLOCK(&resolver->lock);
}

static void
free_algorithm(void *node, void *arg) {
	unsigned char *algorithms = node;
	isc_mem_t *mctx = arg;

	isc_mem_put(mctx, algorithms, *algorithms);
}

void
dns_resolver_reset_algorithms(dns_resolver_t *resolver) {

	REQUIRE(VALID_RESOLVER(resolver));

#if USE_ALGLOCK
	RWLOCK(&resolver->alglock, isc_rwlocktype_write);
#endif
	if (resolver->algorithms != NULL)
		dns_rbt_destroy(&resolver->algorithms);
#if USE_ALGLOCK
	RWUNLOCK(&resolver->alglock, isc_rwlocktype_write);
#endif
}

isc_result_t
dns_resolver_disable_algorithm(dns_resolver_t *resolver, dns_name_t *name,
			       unsigned int alg)
{
	unsigned int len, mask;
	unsigned char *tmp;
	unsigned char *algorithms;
	isc_result_t result;
	dns_rbtnode_t *node = NULL;

	/*
	 * Whether an algorithm is disabled (or not) is stored in a
	 * per-name bitfield that is stored as the node data of an
	 * RBT.
	 */

	REQUIRE(VALID_RESOLVER(resolver));
	if (alg > 255)
		return (ISC_R_RANGE);

#if USE_ALGLOCK
	RWLOCK(&resolver->alglock, isc_rwlocktype_write);
#endif
	if (resolver->algorithms == NULL) {
		result = dns_rbt_create(resolver->mctx, free_algorithm,
					resolver->mctx, &resolver->algorithms);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
	}

	len = alg/8 + 2;
	mask = 1 << (alg%8);

	result = dns_rbt_addnode(resolver->algorithms, name, &node);

	if (result == ISC_R_SUCCESS || result == ISC_R_EXISTS) {
		algorithms = node->data;
		/*
		 * If algorithms is set, algorithms[0] contains its
		 * length.
		 */
		if (algorithms == NULL || len > *algorithms) {
			/*
			 * If no bitfield exists in the node data, or if
			 * it is not long enough, allocate a new
			 * bitfield and copy the old (smaller) bitfield
			 * into it if one exists.
			 */
			tmp = isc_mem_get(resolver->mctx, len);
			if (tmp == NULL) {
				result = ISC_R_NOMEMORY;
				goto cleanup;
			}
			memset(tmp, 0, len);
			if (algorithms != NULL)
				memmove(tmp, algorithms, *algorithms);
			tmp[len-1] |= mask;
			/* 'tmp[0]' should contain the length of 'tmp'. */
			*tmp = len;
			node->data = tmp;
			/* Free the older bitfield. */
			if (algorithms != NULL)
				isc_mem_put(resolver->mctx, algorithms,
					    *algorithms);
		} else
			algorithms[len-1] |= mask;
	}
	result = ISC_R_SUCCESS;
 cleanup:
#if USE_ALGLOCK
	RWUNLOCK(&resolver->alglock, isc_rwlocktype_write);
#endif
	return (result);
}

isc_boolean_t
dns_resolver_algorithm_supported(dns_resolver_t *resolver, dns_name_t *name,
				 unsigned int alg)
{
	unsigned int len, mask;
	unsigned char *algorithms;
	void *data = NULL;
	isc_result_t result;
	isc_boolean_t found = ISC_FALSE;

	REQUIRE(VALID_RESOLVER(resolver));

	/*
	 * DH is unsupported for DNSKEYs, see RFC 4034 sec. A.1.
	 */
	if ((alg == DST_ALG_DH) || (alg == DST_ALG_INDIRECT))
		return (ISC_FALSE);

#if USE_ALGLOCK
	RWLOCK(&resolver->alglock, isc_rwlocktype_read);
#endif
	if (resolver->algorithms == NULL)
		goto unlock;
	result = dns_rbt_findname(resolver->algorithms, name, 0, NULL, &data);
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH) {
		len = alg/8 + 2;
		mask = 1 << (alg%8);
		algorithms = data;
		if (len <= *algorithms && (algorithms[len-1] & mask) != 0)
			found = ISC_TRUE;
	}
 unlock:
#if USE_ALGLOCK
	RWUNLOCK(&resolver->alglock, isc_rwlocktype_read);
#endif
	if (found)
		return (ISC_FALSE);

	return (dst_algorithm_supported(alg));
}

static void
free_digest(void *node, void *arg) {
	unsigned char *digests = node;
	isc_mem_t *mctx = arg;

	isc_mem_put(mctx, digests, *digests);
}

void
dns_resolver_reset_ds_digests(dns_resolver_t *resolver) {

	REQUIRE(VALID_RESOLVER(resolver));

#if USE_ALGLOCK
	RWLOCK(&resolver->alglock, isc_rwlocktype_write);
#endif
	if (resolver->digests != NULL)
		dns_rbt_destroy(&resolver->digests);
#if USE_ALGLOCK
	RWUNLOCK(&resolver->alglock, isc_rwlocktype_write);
#endif
}

isc_result_t
dns_resolver_disable_ds_digest(dns_resolver_t *resolver, dns_name_t *name,
			       unsigned int digest_type)
{
	unsigned int len, mask;
	unsigned char *tmp;
	unsigned char *digests;
	isc_result_t result;
	dns_rbtnode_t *node = NULL;

	/*
	 * Whether a digest is disabled (or not) is stored in a per-name
	 * bitfield that is stored as the node data of an RBT.
	 */

	REQUIRE(VALID_RESOLVER(resolver));
	if (digest_type > 255)
		return (ISC_R_RANGE);

#if USE_ALGLOCK
	RWLOCK(&resolver->alglock, isc_rwlocktype_write);
#endif
	if (resolver->digests == NULL) {
		result = dns_rbt_create(resolver->mctx, free_digest,
					resolver->mctx, &resolver->digests);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
	}

	len = digest_type/8 + 2;
	mask = 1 << (digest_type%8);

	result = dns_rbt_addnode(resolver->digests, name, &node);

	if (result == ISC_R_SUCCESS || result == ISC_R_EXISTS) {
		digests = node->data;
		/* If digests is set, digests[0] contains its length. */
		if (digests == NULL || len > *digests) {
			/*
			 * If no bitfield exists in the node data, or if
			 * it is not long enough, allocate a new
			 * bitfield and copy the old (smaller) bitfield
			 * into it if one exists.
			 */
			tmp = isc_mem_get(resolver->mctx, len);
			if (tmp == NULL) {
				result = ISC_R_NOMEMORY;
				goto cleanup;
			}
			memset(tmp, 0, len);
			if (digests != NULL)
				memmove(tmp, digests, *digests);
			tmp[len-1] |= mask;
			/* tmp[0] should contain the length of 'tmp'. */
			*tmp = len;
			node->data = tmp;
			/* Free the older bitfield. */
			if (digests != NULL)
				isc_mem_put(resolver->mctx, digests,
					    *digests);
		} else
			digests[len-1] |= mask;
	}
	result = ISC_R_SUCCESS;
 cleanup:
#if USE_ALGLOCK
	RWUNLOCK(&resolver->alglock, isc_rwlocktype_write);
#endif
	return (result);
}

isc_boolean_t
dns_resolver_ds_digest_supported(dns_resolver_t *resolver, dns_name_t *name,
				 unsigned int digest_type)
{
	unsigned int len, mask;
	unsigned char *digests;
	void *data = NULL;
	isc_result_t result;
	isc_boolean_t found = ISC_FALSE;

	REQUIRE(VALID_RESOLVER(resolver));

#if USE_ALGLOCK
	RWLOCK(&resolver->alglock, isc_rwlocktype_read);
#endif
	if (resolver->digests == NULL)
		goto unlock;
	result = dns_rbt_findname(resolver->digests, name, 0, NULL, &data);
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH) {
		len = digest_type/8 + 2;
		mask = 1 << (digest_type%8);
		digests = data;
		if (len <= *digests && (digests[len-1] & mask) != 0)
			found = ISC_TRUE;
	}
 unlock:
#if USE_ALGLOCK
	RWUNLOCK(&resolver->alglock, isc_rwlocktype_read);
#endif
	if (found)
		return (ISC_FALSE);
	return (dst_ds_digest_supported(digest_type));
}

void
dns_resolver_resetmustbesecure(dns_resolver_t *resolver) {

	REQUIRE(VALID_RESOLVER(resolver));

#if USE_MBSLOCK
	RWLOCK(&resolver->mbslock, isc_rwlocktype_write);
#endif
	if (resolver->mustbesecure != NULL)
		dns_rbt_destroy(&resolver->mustbesecure);
#if USE_MBSLOCK
	RWUNLOCK(&resolver->mbslock, isc_rwlocktype_write);
#endif
}

static isc_boolean_t yes = ISC_TRUE, no = ISC_FALSE;

isc_result_t
dns_resolver_setmustbesecure(dns_resolver_t *resolver, dns_name_t *name,
			     isc_boolean_t value)
{
	isc_result_t result;

	REQUIRE(VALID_RESOLVER(resolver));

#if USE_MBSLOCK
	RWLOCK(&resolver->mbslock, isc_rwlocktype_write);
#endif
	if (resolver->mustbesecure == NULL) {
		result = dns_rbt_create(resolver->mctx, NULL, NULL,
					&resolver->mustbesecure);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
	}
	result = dns_rbt_addname(resolver->mustbesecure, name,
				 value ? &yes : &no);
 cleanup:
#if USE_MBSLOCK
	RWUNLOCK(&resolver->mbslock, isc_rwlocktype_write);
#endif
	return (result);
}

isc_boolean_t
dns_resolver_getmustbesecure(dns_resolver_t *resolver, dns_name_t *name) {
	void *data = NULL;
	isc_boolean_t value = ISC_FALSE;
	isc_result_t result;

	REQUIRE(VALID_RESOLVER(resolver));

#if USE_MBSLOCK
	RWLOCK(&resolver->mbslock, isc_rwlocktype_read);
#endif
	if (resolver->mustbesecure == NULL)
		goto unlock;
	result = dns_rbt_findname(resolver->mustbesecure, name, 0, NULL, &data);
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH)
		value = *(isc_boolean_t*)data;
 unlock:
#if USE_MBSLOCK
	RWUNLOCK(&resolver->mbslock, isc_rwlocktype_read);
#endif
	return (value);
}

void
dns_resolver_getclientsperquery(dns_resolver_t *resolver, isc_uint32_t *cur,
				isc_uint32_t *min, isc_uint32_t *max)
{
	REQUIRE(VALID_RESOLVER(resolver));

	LOCK(&resolver->lock);
	if (cur != NULL)
		*cur = resolver->spillat;
	if (min != NULL)
		*min = resolver->spillatmin;
	if (max != NULL)
		*max = resolver->spillatmax;
	UNLOCK(&resolver->lock);
}

void
dns_resolver_setclientsperquery(dns_resolver_t *resolver, isc_uint32_t min,
				isc_uint32_t max)
{
	REQUIRE(VALID_RESOLVER(resolver));

	LOCK(&resolver->lock);
	resolver->spillatmin = resolver->spillat = min;
	resolver->spillatmax = max;
	UNLOCK(&resolver->lock);
}

void
dns_resolver_setfetchesperzone(dns_resolver_t *resolver, isc_uint32_t clients)
{
#ifdef ENABLE_FETCHLIMIT
	REQUIRE(VALID_RESOLVER(resolver));

	LOCK(&resolver->lock);
	resolver->zspill = clients;
	UNLOCK(&resolver->lock);
#else
	UNUSED(resolver);
	UNUSED(clients);

	return;
#endif /* !ENABLE_FETCHLIMIT */
}


isc_boolean_t
dns_resolver_getzeronosoattl(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));

	return (resolver->zero_no_soa_ttl);
}

void
dns_resolver_setzeronosoattl(dns_resolver_t *resolver, isc_boolean_t state) {
	REQUIRE(VALID_RESOLVER(resolver));

	resolver->zero_no_soa_ttl = state;
}

unsigned int
dns_resolver_getoptions(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));

	return (resolver->options);
}

unsigned int
dns_resolver_gettimeout(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));

	return (resolver->query_timeout);
}

void
dns_resolver_settimeout(dns_resolver_t *resolver, unsigned int seconds) {
	REQUIRE(VALID_RESOLVER(resolver));

	if (seconds == 0)
		seconds = DEFAULT_QUERY_TIMEOUT;
	if (seconds > MAXIMUM_QUERY_TIMEOUT)
		seconds = MAXIMUM_QUERY_TIMEOUT;
	if (seconds < MINIMUM_QUERY_TIMEOUT)
		seconds =  MINIMUM_QUERY_TIMEOUT;

	resolver->query_timeout = seconds;
}

void
dns_resolver_setquerydscp4(dns_resolver_t *resolver, isc_dscp_t dscp) {
	REQUIRE(VALID_RESOLVER(resolver));

	resolver->querydscp4 = dscp;
}

isc_dscp_t
dns_resolver_getquerydscp4(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->querydscp4);
}

void
dns_resolver_setquerydscp6(dns_resolver_t *resolver, isc_dscp_t dscp) {
	REQUIRE(VALID_RESOLVER(resolver));

	resolver->querydscp6 = dscp;
}

isc_dscp_t
dns_resolver_getquerydscp6(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->querydscp6);
}

void
dns_resolver_setmaxdepth(dns_resolver_t *resolver, unsigned int maxdepth) {
	REQUIRE(VALID_RESOLVER(resolver));
	resolver->maxdepth = maxdepth;
}

unsigned int
dns_resolver_getmaxdepth(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->maxdepth);
}

void
dns_resolver_setmaxqueries(dns_resolver_t *resolver, unsigned int queries) {
	REQUIRE(VALID_RESOLVER(resolver));
	resolver->maxqueries = queries;
}

unsigned int
dns_resolver_getmaxqueries(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->maxqueries);
}

void
dns_resolver_dumpfetches(dns_resolver_t *resolver,
			 isc_statsformat_t format, FILE *fp)
{
#ifdef ENABLE_FETCHLIMIT
	int i;

	REQUIRE(VALID_RESOLVER(resolver));
	REQUIRE(fp != NULL);
	REQUIRE(format == isc_statsformat_file);

	for (i = 0; i < RES_DOMAIN_BUCKETS; i++) {
		fctxcount_t *fc;
		LOCK(&resolver->dbuckets[i].lock);
		for (fc = ISC_LIST_HEAD(resolver->dbuckets[i].list);
		     fc != NULL;
		     fc = ISC_LIST_NEXT(fc, link))
		{
			dns_name_print(fc->domain, fp);
			fprintf(fp, ": %d active (%d spilled, %d allowed)\n",
				fc->count, fc->dropped, fc->allowed);
		}
		UNLOCK(&resolver->dbuckets[i].lock);
	}
#else
	UNUSED(resolver);
	UNUSED(format);
	UNUSED(fp);

	return;
#endif /* !ENABLE_FETCHLIMIT */
}

void
dns_resolver_setquotaresponse(dns_resolver_t *resolver,
			      dns_quotatype_t which, isc_result_t resp)
{
#ifdef ENABLE_FETCHLIMIT
	REQUIRE(VALID_RESOLVER(resolver));
	REQUIRE(which == dns_quotatype_zone || which == dns_quotatype_server);
	REQUIRE(resp == DNS_R_DROP || resp == DNS_R_SERVFAIL);

	resolver->quotaresp[which] = resp;
#else
	UNUSED(resolver);
	UNUSED(which);
	UNUSED(resp);

	return;
#endif /* !ENABLE_FETCHLIMIT */
}

isc_result_t
dns_resolver_getquotaresponse(dns_resolver_t *resolver, dns_quotatype_t which)
{
#ifdef ENABLE_FETCHLIMIT
	REQUIRE(VALID_RESOLVER(resolver));
	REQUIRE(which == dns_quotatype_zone || which == dns_quotatype_server);

	return (resolver->quotaresp[which]);
#else
	UNUSED(resolver);
	UNUSED(which);

	return (ISC_R_NOTIMPLEMENTED);
#endif /* !ENABLE_FETCHLIMIT */
}
