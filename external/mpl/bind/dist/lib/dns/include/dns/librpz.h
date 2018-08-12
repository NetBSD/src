/*	$NetBSD: librpz.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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

/*
 * Define the interface from a DNS resolver to the Response Policy Zone
 * library, librpz.
 *
 * This file should be included only the interface functions between the
 * resolver and librpz to avoid name space pollution.
 *
 * Copyright (c) 2016-2017 Farsight Security, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * version 1.2.12
 */

#ifndef LIBRPZ_H
#define LIBRPZ_H

#include <arpa/nameser.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>


/*
 * Allow either ordinary or dlopen() linking.
 */
#ifdef LIBRPZ_INTERNAL
#define LIBDEF(t,s) extern t s;
#define LIBDEF_F(f) LIBDEF(librpz_##f##_t, librpz_##f)
#else
#define LIBDEF(t,s)
#define LIBDEF_F(f)
#endif

/*
 * Response Policy Zone triggers.
 *	Comparisons of trigger precedences require
 *	LIBRPZ_TRIG_CLIENT_IP < LIBRPZ_TRIG_QNAME < LIBRPZ_TRIG_IP
 *	    < LIBRPZ_TRIG_NSDNAME < LIBRPZ_TRIG_NSIP}
 */
typedef enum {
	LIBRPZ_TRIG_BAD		=0,
	LIBRPZ_TRIG_CLIENT_IP	=1,
	LIBRPZ_TRIG_QNAME	=2,
	LIBRPZ_TRIG_IP		=3,
	LIBRPZ_TRIG_NSDNAME	=4,
	LIBRPZ_TRIG_NSIP	=5
} librpz_trig_t;
#define LIBRPZ_TRIG_SIZE	3	/* sizeof librpz_trig_t in bits */
typedef uint8_t		librpz_tbit_t;  /* one bit for each of the TRIGS_NUM
					 * trigger types */


/*
 * Response Policy Zone Actions or policies
 */
typedef enum {
	LIBRPZ_POLICY_UNDEFINED	=0,	/* an empty entry or no decision yet */
	LIBRPZ_POLICY_DELETED	=1,	/* placeholder for a deleted policy */

	LIBRPZ_POLICY_PASSTHRU	=2,	/* 'passthru': do not rewrite */
	LIBRPZ_POLICY_DROP	=3,	/* 'drop': do not respond */
	LIBRPZ_POLICY_TCP_ONLY	=4,	/* 'tcp-only': answer UDP with TC=1 */
	LIBRPZ_POLICY_NXDOMAIN	=5,	/* 'nxdomain': answer with NXDOMAIN */
	LIBRPZ_POLICY_NODATA	=6,	/* 'nodata': answer with ANCOUNT=0 */
	LIBRPZ_POLICY_RECORD	=7,	/* rewrite with the policy's RR */

	/* only in client configurations to override the zone */
	LIBRPZ_POLICY_GIVEN,		/* 'given': what policy record says */
	LIBRPZ_POLICY_DISABLED,		/* at most log */
	LIBRPZ_POLICY_CNAME,		/* answer with 'cname x' */
} librpz_policy_t;
#define LIBRPZ_POLICY_BITS	4

/*
 * Special policies that appear as targets of CNAMEs
 * NXDOMAIN is signaled by a CNAME with a "." target.
 * NODATA is signaled by a CNAME with a "*." target.
 */
#define LIBRPZ_RPZ_PREFIX	"rpz-"
#define LIBRPZ_RPZ_PASSTHRU	LIBRPZ_RPZ_PREFIX"passthru"
#define LIBRPZ_RPZ_DROP		LIBRPZ_RPZ_PREFIX"drop"
#define LIBRPZ_RPZ_TCP_ONLY	LIBRPZ_RPZ_PREFIX"tcp-only"


typedef	uint16_t    librpz_dznum_t;	/* dnsrpzd zone # in [0,DZNUM_MAX] */
typedef	uint8_t	    librpz_cznum_t;	/* client zone # in [0,CZNUM_MAX] */


/*
 * CIDR block
 */
typedef struct librpz_prefix {
	union {
		struct in_addr	in;
		struct in6_addr	in6;
	} addr;
	uint8_t		    family;
	uint8_t		    len;
} librpz_prefix_t;

/*
 * A domain
 */
typedef uint8_t	librpz_dsize_t;
typedef struct librpz_domain {
	librpz_dsize_t	    size;	/* of only .d */
	uint8_t		    d[0];	/* variable length wire format */
} librpz_domain_t;

/*
 * A maximal domain buffer
 */
typedef struct librpz_domain_buf {
	librpz_dsize_t	    size;
	uint8_t		    d[NS_MAXCDNAME];
} librpz_domain_buf_t;

/*
 * A resource record without the owner name.
 * C compilers say that sizeof(librpz_rr_t)=12 instead of 10.
 */
typedef struct {
	uint16_t	    type;	/* network byte order */
	uint16_t	    class;	/* network byte order */
	uint32_t	    ttl;	/* network byte order */
	uint16_t	    rdlength;	/* network byte order */
	uint8_t		    rdata[0];	/* variable length */
} librpz_rr_t;

/*
 * The database file might be mapped with different starting addresses
 * by concurrent clients (resolvers), and so all pointers are offsets.
 */
typedef uint32_t	librpz_idx_t;
#define LIBRPZ_IDX_NULL	0
#define LIBRPZ_IDX_MIN	1
#define LIBRPZ_IDX_BAD  ((librpz_idx_t)-1)
/**
 * Partial decoded results of a set of RPZ queries for a single DNS response
 * or iteration through the mapped file.
 */
typedef int16_t librpz_result_id_t;
typedef struct librpz_result {
	librpz_idx_t	    next_rr;
	librpz_result_id_t  hit_id;		/* trigger ID from resolver */
	librpz_policy_t	    zpolicy;	/* policy from zone */
	librpz_policy_t	    policy;	/* adjusted by client configuration */
	librpz_dznum_t	    dznum;	/* dnsrpzd zone number */
	librpz_cznum_t	    cznum;	/* librpz client zone number */
	librpz_trig_t	    trig:LIBRPZ_TRIG_SIZE;
	bool		    log:1;	/* log rewrite given librpz_log_level */
} librpz_result_t;


/**
 * librpz trace or log levels.
 */
typedef enum {
	LIBRPZ_LOG_FATAL    =0,		/* always print fatal errors */
	LIBRPZ_LOG_ERROR    =1,		/* errors have this level */
	LIBRPZ_LOG_TRACE1   =2,		/* big events such as dnsrpzd starts */
	LIBRPZ_LOG_TRACE2   =3,		/* smaller dnsrpzd zone transfers */
	LIBRPZ_LOG_TRACE3   =4,		/* librpz hits */
	LIBRPZ_LOG_TRACE4   =5,		/* librpz lookups */
	LIBRPZ_LOG_INVALID   =999,
} librpz_log_level_t;
typedef librpz_log_level_t (librpz_log_level_val_t)(librpz_log_level_t level);
LIBDEF_F(log_level_val)

/**
 * Logging function that can be supplied by the resolver.
 * @param level is one of librpz_log_level_t
 * @param ctx is for use by the resolver's logging system.
 *	NULL mean a context-free message.
 */
typedef void(librpz_log_fnc_t)(librpz_log_level_t level, void *ctx,
			       const char *buf);

/**
 * Point librpz logging functions to the resolver's choice.
 */
typedef void (librpz_set_log_t)(librpz_log_fnc_t *new_log, const char *prog_nm);
LIBDEF_F(set_log)


/**
 * librpz error messages are put in these buffers.
 * Use a structure instead of naked char* to let the compiler check the length.
 * A function defined with "foo(char buf[120])" can be called with
 * "char sbuf[2]; foo(sbuf)" and suffer a buffer overrun.
 */
typedef struct {
	char	c[120];
} librpz_emsg_t;


#ifdef LIBRPZ_HAVE_ATTR
#define LIBRPZ_UNUSED	__attribute__((unused))
#define LIBRPZ_PF(f,l)	__attribute__((format(printf,f,l)))
#define	LIBRPZ_NORET	__attribute__((__noreturn__))
#else
#define LIBRPZ_UNUSED
#define LIBRPZ_PF(f,l)
#define	LIBRPZ_NORET
#endif

#ifdef HAVE_BUILTIN_EXPECT
#define LIBRPZ_LIKELY(c) __builtin_expect(!!(c), 1)
#define LIBRPZ_UNLIKELY(c) __builtin_expect(!!(c), 0)
#else
#define LIBRPZ_LIKELY(c) (c)
#define LIBRPZ_UNLIKELY(c) (c)
#endif

typedef bool (librpz_parse_log_opt_t)(librpz_emsg_t *emsg, const char *arg);
LIBDEF_F(parse_log_opt)

typedef void (librpz_vpemsg_t)(librpz_emsg_t *emsg,
			       const char *p, va_list args);
LIBDEF_F(vpemsg)
typedef void (librpz_pemsg_t)(librpz_emsg_t *emsg,
			      const char *p, ...) LIBRPZ_PF(2,3);
LIBDEF_F(pemsg)

typedef void (librpz_vlog_t)(librpz_log_level_t level, void *ctx,
			     const char *p, va_list args);
LIBDEF_F(vlog)
typedef void (librpz_log_t)(librpz_log_level_t level, void *ctx,
			    const char *p, ...) LIBRPZ_PF(3,4);
LIBDEF_F(log)

typedef void (librpz_fatal_t)(int ex_code,
			      const char *p, ...) LIBRPZ_PF(2,3);
extern void librpz_fatal(int ex_code,
			 const char *p, ...) LIBRPZ_PF(2,3) LIBRPZ_NORET;

typedef void (librpz_rpz_assert_t)(const char *file, unsigned line,
				   const char *p, ...) LIBRPZ_PF(3,4);
extern void librpz_rpz_assert(const char *file, unsigned line,
			      const char *p, ...) LIBRPZ_PF(3,4) LIBRPZ_NORET;

typedef void (librpz_rpz_vassert_t)(const char *file, uint line,
				    const char *p, va_list args);
extern void librpz_rpz_vassert(const char *file, uint line,
			       const char *p, va_list args) LIBRPZ_NORET;


/*
 * As far as clients are concerned, all relative pointers or indexes in a
 * version of the mapped file except trie node parent pointers remain valid
 * forever.  A client must release a version so that it can be garbage
 * collected by the file system.  When dnsrpzd needs to expand the file,
 * it copies the old file to a new, larger file.  Clients can continue
 * using the old file.
 *
 * Versions can also appear in a single file.  Old nodes and trie values
 * within the file are not destroyed until all clients using the version
 * that contained the old values release the version.
 *
 * A client is marked as using version by connecting to the daemon.  It is
 * marked as using all subsequent versions.  A client releases all versions
 * by closing the connection or a range of versions by updating is slot
 * in the shared memory version table.
 *
 * As far as clients are concerned, there are the following possible librpz
 * failures:
 *	- malloc() or other fatal internal librpz problems indicated by
 *	    a failing return from a librpz function
 *	    All operations will fail until client handle is destroyed and
 *	    recreated with librpz_client_detach() and librpz_client_create().
 *	- corrupt database detected by librpz code, corrupt database detected
 *	    by dnsrpzd, or disconnection from the daemon.
 *	    Current operations will fail.
 *
 * Clients assume that the file has already been unlinked before
 *	the corrupt flag is set so that they do not race with the server
 *	over the corruption of a single file.  A client that finds the
 *	corrupt set knows that dnsrpzd has already crashed with
 *	abort() and is restarting.  The client can re-connect to dnsrpzd
 *	and retransmit its configuration, backing off as usual if anything
 *	goes wrong.
 *
 * Searches of the database by a client do not need locks against dnsrpzd or
 *	other clients, but a lock is used to protect changes to the connection
 *	by competing threads in the client.  The client provides functions
 *	to serialize the concurrent use of any single client handle.
 *	Functions that do nothing are appropriate for applications that are
 *	not "threaded" or that do not share client handles among threads.
 *	Otherwise, functions must be provided to librpz_clientcreate().
 *	Something like the following works with pthreads:
 *
 * static void
 * lock(void *mutex) { assert(pthread_mutex_lock(mutex) == 0); }
 *
 * static void
 * unlock(void *mutex) { assert(pthread_mutex_unlock(mutex) == 0); }
 *
 * static void
 * mutex_destroy(void *mutex) { assert(pthread_mutex_destroy(mutex) == 0); }
 *
 *
 *
 * At every instant, all of the data and pointers in the mapped file are valid.
 *	Changes to trie node or other data are always made so that it and
 *	all pointers in and to it remain valid for a time.  Old versions are
 *	eventually discarded.
 *
 * Dnsrpzd periodically defines a new version by setting aside all changes
 *	made since the previous version was defined.  Subsequent changes
 *	made (only!) by dnsrpzd will be part of the next version.
 *
 * To discard an old version, dnsrpzd must know that all clients have stopped
 *	using that version.  Clients do that by using part of the mapped file
 *	to tell dnsrpzd the oldest version that each client is using.
 *	Dnsrpzd assigns each connecting client an entry in the cversions array
 *	in the mapped file.  The client puts version numbers into that entry
 *	to signal to dnsrpzd which versions that can be discarded.
 *	Dnsrpzd is free, as far as that client is concerned, to discard all
 *	numerically smaller versions.  A client can disclaim all versions with
 *	the version number VERSIONS_ALL or 0.
 *
 * The race between a client changing its entry and dnsrpzd discarding a
 *	version is resolved by allowing dnsrpzd to discard all versions
 *	smaller or equal to the client's version number.  If dnsrpzd is in
 *	the midst of discarding or about to discard version N when the
 *	client asserts N, no harm is done.  The client depends only on
 *	the consistency of version N+1.
 *
 * This version mechanism depends in part on not being exercised too frequently
 *	Version numbers are 32 bits long and dnsrpzd creates new versions
 *	at most once every 30 seconds.
 */


/*
 * Lock functions for concurrent use of a single librpz_client_t client handle.
 */
typedef void(librpz_mutex_t)(void *mutex);

/*
 * List of connections to dnsrpzd daemons.
 */
typedef struct librpz_clist librpz_clist_t;

/*
 * Client's handle on dnsrpzd.
 */
typedef struct librpz_client librpz_client_t;

/**
 * Create the list of connections to the dnsrpzd daemon.
 * @param[out] emsg: error message
 * @param lock: start exclusive access to the client handle
 * @param unlock: end exclusive access to the client handle
 * @param mutex_destroy: release the lock
 * @param mutex: pointer to the lock for the client handle
 * @param log_ctx: NULL or resolver's context log messages
 */
typedef librpz_clist_t *(librpz_clist_create_t)(librpz_emsg_t *emsg,
						librpz_mutex_t *lock,
						librpz_mutex_t *unlock,
						librpz_mutex_t *mutex_destroy,
						void *mutex, void *log_ctx);
LIBDEF_F(clist_create)


/**
 * Release the list of dnsrpzd connections.
 */
typedef void (librpz_clist_detach_t)(librpz_clist_t **clistp);
LIBDEF_F(clist_detach)

/**
 * Create a librpz client handle.
 * @param[out] emsg: error message
 * @param clist: of dnsrpzd connections
 * @param cstr: string of configuration settings separated by ';' or '\n'
 * @param use_expired: true to not ignore expired zones
 * @return client handle or NULL if the handle could not be created
 */
typedef librpz_client_t *(librpz_client_create_t)(librpz_emsg_t *emsg,
						  librpz_clist_t *clist,
						  const char *cstr,
						  bool use_expired);
LIBDEF_F(client_create)

/**
 * Start (if necessary) dnsrpzd and connect to it.
 * @param[out] emsg: error message
 * @param client handle
 * @param optional: true if it is ok if starting the daemon is not allowed
 */
typedef bool (librpz_connect_t)(librpz_emsg_t *emsg, librpz_client_t *client,
				bool optional);
LIBDEF_F(connect)

/**
 * Start to destroy a librpz client handle.
 * It will not be destroyed until the last set of RPZ queries represented
 * by a librpz_rsp_t ends.
 * @param client handle to be released
 * @return false on error
 */
typedef void (librpz_client_detach_t)(librpz_client_t **clientp);
LIBDEF_F(client_detach)

/**
 * State for a set of RPZ queries for a single DNS response
 * or for listing the database.
 */
typedef struct librpz_rsp librpz_rsp_t;

/**
 * Start a set of RPZ queries for a single DNS response.
 * @param[out] emsg: error message for false return or *rspp=NULL
 * @param[out] rspp created context or NULL
 * @param[out] min_ns_dotsp: NULL or pointer to configured MIN-NS-DOTS value
 * @param client state
 * @param have_rd: RD=1 in the DNS request
 * @param have_do: DO=1 in the DNS request
 * @return false on error
 */
typedef bool (librpz_rsp_create_t)(librpz_emsg_t *emsg, librpz_rsp_t **rspp,
				   int *min_ns_dotsp, librpz_client_t *client,
				   bool have_rd, bool have_do);
LIBDEF_F(rsp_create)

/**
 * Finish RPZ work for a DNS response.
 */
typedef void (librpz_rsp_detach_t)(librpz_rsp_t **rspp);
LIBDEF_F(rsp_detach)

/**
 * Get the final, accumulated result of a set of RPZ queries.
 * Yield LIBRPZ_POLICY_UNDEFINED if
 *  - there were no hits,
 *  - there was a dispositive hit, be we have not recursed and are required
 *	to recurse so that evil DNS authorities will not know we are using RPZ
 *  - we have a hit and have recursed, but later data such as NSIP could
 *	override
 * @param[out] emsg
 * @param[out] result describes the hit
 *	or result->policy=LIBRPZ_POLICY_UNDEFINED without a hit
 * @param[out] result: current policy rewrite values
 * @param recursed: recursion has now been done even if it was not done
 *	when the hit was found
 * @param[in,out] rsp state from librpz_itr_start()
 * @return false on error
 */
typedef bool (librpz_rsp_result_t)(librpz_emsg_t *emsg, librpz_result_t *result,
				   bool recursed, const librpz_rsp_t *rsp);
LIBDEF_F(rsp_result)

/**
 * Might looking for a trigger be worthwhile?
 * @param trig: look for this type of trigger
 * @param ipv6: true if trig is LIBRPZ_TRIG_CLIENT_IP, LIBRPZ_TRIG_IP,
 *	or LIBRPZ_TRIG_NSIP and the IP address is IPv6
 * @return: true if looking could be worthwhile
 */
typedef bool (librpz_have_trig_t)(librpz_trig_t trig, bool ipv6,
				  const librpz_rsp_t *rsp);
LIBDEF_F(have_trig)

/**
 * Might looking for NSDNAME and NSIP triggers be worthwhile?
 * @return: true if looking could be worthwhile
 */
typedef bool (librpz_have_ns_trig_t)(const librpz_rsp_t *rsp);
LIBDEF_F(have_ns_trig)

/**
 * Convert the found client IP trie key to a CIDR block
 * @param[out] emsg
 * @param[out] prefix trigger
 * @param[in,out] rsp state from librpz_itr_start()
 * @return false on error
 */
typedef bool (librpz_rsp_clientip_prefix_t)(librpz_emsg_t *emsg,
					    librpz_prefix_t *prefix,
					    librpz_rsp_t *rsp);
LIBDEF_F(rsp_clientip_prefix)

/**
 * Compute the owner name of the found or result trie key, usually to log it.
 * An IP address key might be returned as 8.0.0.0.127.rpz-client-ip.
 * example.com. might be a qname trigger.  example.com.rpz-nsdname. could
 * be an NSDNAME trigger.
 * @param[out] emsg
 * @param[out] owner domain
 * @param[in,out] rsp state from librpz_itr_start()
 * @return false on error
 */
typedef bool (librpz_rsp_domain_t)(librpz_emsg_t *emsg,
				   librpz_domain_buf_t *owner,
				   librpz_rsp_t *rsp);
LIBDEF_F(rsp_domain)

/**
 * Get the next RR of the LIBRPZ_POLICY_RECORD result after an initial use of
 * librpz_rsp_result() or librpz_itr_node() or after a previous use of
 * librpz_rsp_rr().  The RR is in uncompressed wire format including type,
 * class, ttl and length in network byte order.
 * @param[out] emsg
 * @param[out] typep: optional host byte order record type or ns_t_invalid (0)
 * @param[out] classp: class such as ns_c_in
 * @param[out] ttlp: TTL
 * @param[out] rrp: optional malloc() buffer containing the next RR or
 *	NULL after the last RR
 * @param[out] result: current policy rewrite values
 * @param qname: used construct a wildcard CNAME
 * @param qname_size
 * @param[in,out] rsp state from librpz_itr_start()
 * @return false on error
 */
typedef bool (librpz_rsp_rr_t)(librpz_emsg_t *emsg, uint16_t *typep,
			       uint16_t *classp, uint32_t *ttlp,
			       librpz_rr_t **rrp, librpz_result_t *result,
			       const uint8_t *qname, size_t qname_size,
			       librpz_rsp_t *rsp);
LIBDEF_F(rsp_rr)

/**
 * Get the next RR of the LIBRPZ_POLICY_RECORD result.
 * @param[out] emsg
 * @param[out] ttlp: TTL
 * @param[out] rrp: malloc() buffer with SOA RR without owner name
 * @param[out] result: current policy rewrite values
 * @param[out] origin: SOA owner name
 * @param[out] origin_size
 * @param[in,out] rsp state from librpz_itr_start()
 * @return false on error
 */
typedef bool (librpz_rsp_soa_t)(librpz_emsg_t *emsg, uint32_t *ttlp,
				librpz_rr_t **rrp, librpz_domain_buf_t *origin,
				librpz_result_t *result, librpz_rsp_t *rsp);
LIBDEF_F(rsp_soa)

/**
 * Get the SOA serial number for a policy zone to compare with a known value
 * to check whether a zone transfer is complete.
 */
typedef bool (librpz_soa_serial_t)(librpz_emsg_t *emsg, uint32_t *serialp,
				   const char *domain_nm, librpz_rsp_t *rsp);
LIBDEF_F(soa_serial)

/**
 * Save the current policy checking state.
 * @param[out] emsg
 * @param[in,out] rsp state from librpz_itr_start()
 * @return false on error
 */
typedef bool (librpz_rsp_push_t)(librpz_emsg_t *emsg, librpz_rsp_t *rsp);
LIBDEF_F(rsp_push)
#define LIBRPZ_RSP_STACK_DEPTH	3

/**
 * Restore the previous policy checking state.
 * @param[out] emsg
 * @param[out] result: NULL or restored policy rewrite values
 * @param[in,out] rsp state from librpz_itr_start()
 * @return false on error
 */
typedef bool (librpz_rsp_pop_t)(librpz_emsg_t *emsg, librpz_result_t *result,
				librpz_rsp_t *rsp);
LIBDEF_F(rsp_pop)

/**
 * Discard the most recently save policy checking state.
 * @param[out] emsg
 * @param[out] result: NULL or restored policy rewrite values
 * @return false on error
 */
typedef bool (librpz_rsp_pop_discard_t)(librpz_emsg_t *emsg, librpz_rsp_t *rsp);
LIBDEF_F(rsp_pop_discard)

/**
 * Disable a zone.
 * @param[out] emsg
 * @param znum
 * @param[in,out] rsp state from librpz_itr_start()
 * @return false on error
 */
typedef bool (librpz_rsp_forget_zone_t)(librpz_emsg_t *emsg,
					librpz_cznum_t znum, librpz_rsp_t *rsp);
LIBDEF_F(rsp_forget_zone)

/**
 * Apply RPZ to an IP address.
 * @param[out] emsg
 * @param addr: address to check
 * @param ipv6: true for 16 byte IPv6 instead of 4 byte IPv4
 * @param trig LIBRPZ_TRIG_CLIENT_IP, LIBRPZ_TRIG_IP, or LIBRPZ_TRIG_NSIP
 * @param hit_id: caller chosen
 * @param recursed: recursion has been done
 * @param[in,out] rsp state from librpz_itr_start()
 * @return false on error
 */
typedef bool (librpz_ck_ip_t)(librpz_emsg_t *emsg,
			      const void *addr, uint family,
			      librpz_trig_t trig, librpz_result_id_t hit_id,
			      bool recursed, librpz_rsp_t *rsp);
LIBDEF_F(ck_ip)

/**
 * Apply RPZ to a wire-format domain.
 * @param[out] emsg
 * @param domain in wire format
 * @param domain_size
 * @param trig LIBRPZ_TRIG_QNAME or LIBRPZ_TRIG_NSDNAME
 * @param hit_id: caller chosen
 * @param recursed: recursion has been done
 * @param[in,out] rsp state from librpz_itr_start()
 * @return false on error
 */
typedef bool (librpz_ck_domain_t)(librpz_emsg_t *emsg,
				  const uint8_t *domain, size_t domain_size,
				  librpz_trig_t trig, librpz_result_id_t hit_id,
				  bool recursed, librpz_rsp_t *rsp);
LIBDEF_F(ck_domain)

/**
 * Ask dnsrpzd to refresh a zone.
 * @param[out] emsg error message
 * @param librpz_domain_t domain to refresh
 * @param client context
 * @return false after error
 */
typedef bool (librpz_zone_refresh_t)(librpz_emsg_t *emsg, const char *domain,
				       librpz_rsp_t *rsp);
LIBDEF_F(zone_refresh)

/**
 * Get a string describing the database
 * @param license: include the license
 * @param cfiles: include the configuration file names
 * @param listens: include the local notify IP addresses
 * @param[out] emsg error message if the result is null
 * @param client context
 * @return malloc'ed string or NULL after error
 */
typedef char *(librpz_db_info_t)(librpz_emsg_t *emsg,
				 bool license, bool cfiles, bool listens,
				 librpz_rsp_t *rsp);
LIBDEF_F(db_info)

/**
 * Start a context for listing the nodes and/or zones in the mapped file
 * @param[out] emsg: error message for false return or *rspp=NULL
 * @param[out] rspp: created context or NULL
 * @param client context
 * @return false after error
 */
typedef bool (librpz_itr_start_t)(librpz_emsg_t *emsg, librpz_rsp_t **rspp,
				  librpz_client_t *client);
LIBDEF_F(itr_start)

/**
 * Get mapped file memory allocation statistics.
 * @param[out] emsg: error message
 * @param rsp state from librpz_itr_start()
 * @return malloc'ed string or NULL after error
 */
typedef char *(librpz_mf_stats_t)(librpz_emsg_t *emsg, librpz_rsp_t *rsp);
LIBDEF_F(mf_stats)

/**
 * Get versions currently used by clients.
 * @param[out] emsg: error message
 * @param[in,out] rsp: state from librpz_itr_start()
 * @return malloc'ed string or NULL after error
 */
typedef char *(librpz_vers_stats_t)(librpz_emsg_t *emsg, librpz_rsp_t *rsp);
LIBDEF_F(vers_stats)

/**
 * Allocate a string describing the next zone or "" after the last zone.
 * @param[out] emsg
 * @param all_zones to list all instead of only requested zones
 * @param[in,out] rsp state from librpz_rsp_start()
 * @return malloc'ed string or NULL after error
 */
typedef char *(librpz_itr_zone_t)(librpz_emsg_t *emsg, bool all_zones,
				  librpz_rsp_t *rsp);
LIBDEF_F(itr_zone)

/**
 * Describe the next trie node while dumping the database.
 * @param[out] emsg
 * @param[out] result describes node
 *	or result->policy=LIBRPZ_POLICY_UNDEFINED after the last node.
 * @param all_zones to list all instead of only requested zones
 * @param[in,out] rsp state from librpz_itr_start()
 * @return: false on error
 */
typedef bool (librpz_itr_node_t)(librpz_emsg_t *emsg, librpz_result_t *result,
				 bool all_zones, librpz_rsp_t *rsp);
LIBDEF_F(itr_node)

/**
 * RPZ policy to string with a backup buffer of POLICY2STR_SIZE size
 */
typedef const char *(librpz_policy2str_t)(librpz_policy_t policy,
					  char *buf, size_t buf_size);
#define POLICY2STR_SIZE sizeof("policy xxxxxx")
LIBDEF_F(policy2str)

/**
 * Trigger type to string.
 */
typedef const char *(librpz_trig2str_t)(librpz_trig_t trig);
LIBDEF_F(trig2str)

/**
 * Convert a number of seconds to a zone file duration string
 */
typedef const char *(librpz_secs2str_t)(time_t secs,
					char *buf, size_t buf_size);
#define SECS2STR_SIZE sizeof("1234567w7d24h59m59s")
LIBDEF_F(secs2str)

/**
 * Parse a duration with 's', 'm', 'h', 'd', and 'w' units.
 */
typedef bool (librpz_str2secs_t)(librpz_emsg_t *emsg, time_t *val,
				 const char *str0);
LIBDEF_F(str2secs)

/**
 * Translate selected rtypes to strings
 */
typedef const char *(librpz_rtype2str_t)(uint type, char *buf, size_t buf_size);
#define RTYPE2STR_SIZE sizeof("type xxxxx")
LIBDEF_F(rtype2str)

/**
 * Local version of ns_name_ntop() for portability.
 */
typedef int (librpz_domain_ntop_t)(const u_char *src, char *dst, size_t dstsiz);
LIBDEF_F(domain_ntop)

/**
 * Local version of ns_name_pton().
 */
typedef int (librpz_domain_pton2_t)(const char *src, u_char *dst, size_t dstsiz,
				    size_t *dstlen, bool lower);
LIBDEF_F(domain_pton2)

typedef union socku socku_t;
typedef socku_t *(librpz_mk_inet_su_t)(socku_t *su, const struct in_addr *addrp,
				     in_port_t port);
LIBDEF_F(mk_inet_su)

typedef socku_t *(librpz_mk_inet6_su_t)(socku_t *su, const
					struct in6_addr *addrp,
					uint32_t scope_id, in_port_t port);
LIBDEF_F(mk_inet6_su)

typedef bool (librpz_str2su_t)(socku_t *sup, const char *str);
LIBDEF_F(str2su)

typedef char *(librpz_su2str_t)(char *str, size_t str_len, const socku_t *su);
LIBDEF_F(su2str)
#define SU2STR_SIZE (INET6_ADDRSTRLEN+1+6+1)


/**
 * default path to dnsrpzd
 */
const char *librpz_dnsrpzd_path;


#undef LIBDEF

/*
 * This is the dlopen() interface to librpz.
 */
typedef const struct {
	const char			*dnsrpzd_path;
	const char			*version;
	librpz_parse_log_opt_t		*parse_log_opt;
	librpz_log_level_val_t		*log_level_val;
	librpz_set_log_t		*set_log;
	librpz_vpemsg_t			*vpemsg;
	librpz_pemsg_t			*pemsg;
	librpz_vlog_t			*vlog;
	librpz_log_t			*log;
	librpz_fatal_t			*fatal LIBRPZ_NORET;
	librpz_rpz_assert_t		*rpz_assert LIBRPZ_NORET;
	librpz_rpz_vassert_t		*rpz_vassert LIBRPZ_NORET;
	librpz_clist_create_t		*clist_create;
	librpz_clist_detach_t		*clist_detach;
	librpz_client_create_t		*client_create;
	librpz_connect_t		*connect;
	librpz_client_detach_t		*client_detach;
	librpz_rsp_create_t		*rsp_create;
	librpz_rsp_detach_t		*rsp_detach;
	librpz_rsp_result_t		*rsp_result;
	librpz_have_trig_t		*have_trig;
	librpz_have_ns_trig_t		*have_ns_trig;
	librpz_rsp_clientip_prefix_t	*rsp_clientip_prefix;
	librpz_rsp_domain_t		*rsp_domain;
	librpz_rsp_rr_t			*rsp_rr;
	librpz_rsp_soa_t		*rsp_soa;
	librpz_soa_serial_t		*soa_serial;
	librpz_rsp_push_t		*rsp_push;
	librpz_rsp_pop_t		*rsp_pop;
	librpz_rsp_pop_discard_t	*rsp_pop_discard;
	librpz_rsp_forget_zone_t	*rsp_forget_zone;
	librpz_ck_ip_t			*ck_ip;
	librpz_ck_domain_t		*ck_domain;
	librpz_zone_refresh_t		*zone_refresh;
	librpz_db_info_t		*db_info;
	librpz_itr_start_t		*itr_start;
	librpz_mf_stats_t		*mf_stats;
	librpz_vers_stats_t		*vers_stats;
	librpz_itr_zone_t		*itr_zone;
	librpz_itr_node_t		*itr_node;
	librpz_policy2str_t		*policy2str;
	librpz_trig2str_t		*trig2str;
	librpz_secs2str_t		*secs2str;
	librpz_str2secs_t		*str2secs;
	librpz_rtype2str_t		*rtype2str;
	librpz_domain_ntop_t		*domain_ntop;
	librpz_domain_pton2_t		*domain_pton2;
	librpz_mk_inet_su_t		*mk_inet_su;
	librpz_mk_inet6_su_t		*mk_inet6_su;
	librpz_str2su_t			*str2su;
	librpz_su2str_t			*su2str;
} librpz_0_t;
extern librpz_0_t librpz_def_0;

/*
 * Future versions can be upward compatible by defining LIBRPZ_DEF as
 * librpz_X_t.
 */
#define LIBRPZ_DEF	librpz_def_0
#define LIBRPZ_DEF_STR	"librpz_def_0"

typedef librpz_0_t librpz_t;
extern librpz_t *librpz;


#if LIBRPZ_LIB_OPEN == 2
#include <dlfcn.h>

/**
 * link-load librpz
 * @param[out] emsg: error message
 * @param[in,out] dl_handle: NULL or pointer to new dlopen handle
 * @param[in] path: librpz.so path
 * @return address of interface structure or NULL on failure
 */
static inline librpz_t *
librpz_lib_open(librpz_emsg_t *emsg, void **dl_handle, const char *path)
{
	void *handle;
	librpz_t *new_librpz;

	emsg->c[0] = '\0';

	/*
	 * Close a previously opened handle on librpz.so.
	 */
	if (dl_handle != NULL && *dl_handle != NULL) {
		if (dlclose(*dl_handle) != 0) {
			snprintf(emsg->c, sizeof(librpz_emsg_t),
				 "dlopen(NULL): %s", dlerror());
			return (NULL);
		}
		*dl_handle = NULL;
	}

	/*
	 * First try the main executable of the process in case it was
	 * linked to librpz.
	 * Do not worry if we cannot search the main executable of the process.
	 */
	handle = dlopen(NULL, RTLD_NOW | RTLD_LOCAL);
	if (handle != NULL) {
		new_librpz = dlsym(handle, LIBRPZ_DEF_STR);
		if (new_librpz != NULL) {
			if (dl_handle != NULL)
				*dl_handle = handle;
			return (new_librpz);
		}
		if (dlclose(handle) != 0) {
			snprintf(emsg->c, sizeof(librpz_emsg_t),
				 "dlsym(NULL, "LIBRPZ_DEF_STR"): %s",
				 dlerror());
			return (NULL);
		}
	}

	if (path == NULL || path[0] == '\0') {
		snprintf(emsg->c, sizeof(librpz_emsg_t),
			 "librpz not linked and no dlopen() path provided");
		return (NULL);
	}

	handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	if (handle == NULL) {
		snprintf(emsg->c, sizeof(librpz_emsg_t), "dlopen(%s): %s",
			 path, dlerror());
		return (NULL);
	}
	new_librpz = dlsym(handle, LIBRPZ_DEF_STR);
	if (new_librpz != NULL) {
		if (dl_handle != NULL)
			*dl_handle = handle;
		return (new_librpz);
	}
	snprintf(emsg->c, sizeof(librpz_emsg_t),
		 "dlsym(%s, "LIBRPZ_DEF_STR"): %s",
		 path, dlerror());
	dlclose(handle);
	return (NULL);
}
#elif defined(LIBRPZ_LIB_OPEN)
/*
 * Statically link to the librpz.so DSO on systems without dlopen()
 */
static inline librpz_t *
librpz_lib_open(librpz_emsg_t *emsg, void **dl_handle, const char *path)
{
	(void)(path);

	if (dl_handle != NULL)
		*dl_handle = NULL;

#if LIBRPZ_LIB_OPEN == 1
	emsg->c[0] = '\0';
	return (&LIBRPZ_DEF);
#else
	snprintf(emsg->c, sizeof(librpz_emsg_t),
		 "librpz not available via ./configure");
	return (NULL);
#endif /* LIBRPZ_LIB_OPEN */
}
#endif /* LIBRPZ_LIB_OPEN */

#endif /* LIBRPZ_H */
