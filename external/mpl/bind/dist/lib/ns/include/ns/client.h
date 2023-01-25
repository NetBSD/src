/*	$NetBSD: client.h,v 1.14 2023/01/25 21:43:33 christos Exp $	*/

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

#ifndef NS_CLIENT_H
#define NS_CLIENT_H 1

/*****
***** Module Info
*****/

/*! \file
 * \brief
 * This module defines two objects, ns_client_t and ns_clientmgr_t.
 *
 * An ns_client_t object handles incoming DNS requests from clients
 * on a given network interface.
 *
 * Each ns_client_t object can handle only one TCP connection or UDP
 * request at a time.  Therefore, several ns_client_t objects are
 * typically created to serve each network interface, e.g., one
 * for handling TCP requests and a few (one per CPU) for handling
 * UDP requests.
 *
 * Incoming requests are classified as queries, zone transfer
 * requests, update requests, notify requests, etc, and handed off
 * to the appropriate request handler.  When the request has been
 * fully handled (which can be much later), the ns_client_t must be
 * notified of this by calling one of the following functions
 * exactly once in the context of its task:
 * \code
 *   ns_client_send()	 (sending a non-error response)
 *   ns_client_sendraw() (sending a raw response)
 *   ns_client_error()	 (sending an error response)
 *   ns_client_drop() (sending no response, logging the reason)
 *\endcode
 * This will release any resources used by the request and
 * and allow the ns_client_t to listen for the next request.
 *
 * A ns_clientmgr_t manages a number of ns_client_t objects.
 * New ns_client_t objects are created by calling
 * ns_clientmgr_createclients(). They are destroyed by
 * destroying their manager.
 */

/***
 *** Imports
 ***/

#include <inttypes.h>
#include <stdbool.h>

#include <isc/atomic.h>
#include <isc/buffer.h>
#include <isc/magic.h>
#include <isc/netmgr.h>
#include <isc/platform.h>
#include <isc/quota.h>
#include <isc/stdtime.h>

#include <dns/db.h>
#include <dns/ecs.h>
#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/rdataclass.h>
#include <dns/rdatatype.h>
#include <dns/tcpmsg.h>
#include <dns/types.h>

#include <ns/query.h>
#include <ns/types.h>

/***
 *** Types
 ***/

#define NS_CLIENT_TCP_BUFFER_SIZE  65535
#define NS_CLIENT_SEND_BUFFER_SIZE 4096

/*!
 * Client object states.  Ordering is significant: higher-numbered
 * states are generally "more active", meaning that the client can
 * have more dynamically allocated data, outstanding events, etc.
 * In the list below, any such properties listed for state N
 * also apply to any state > N.
 */

typedef enum {
	NS_CLIENTSTATE_FREED = 0,
	/*%<
	 * The client object no longer exists.
	 */

	NS_CLIENTSTATE_INACTIVE = 1,
	/*%<
	 * The client object exists and has a task and timer.
	 * Its "query" struct and sendbuf are initialized.
	 * It has a message and OPT, both in the reset state.
	 */

	NS_CLIENTSTATE_READY = 2,
	/*%<
	 * The client object is either a TCP or a UDP one, and
	 * it is associated with a network interface.  It is on the
	 * client manager's list of active clients.
	 *
	 * If it is a TCP client object, it has a TCP listener socket
	 * and an outstanding TCP listen request.
	 *
	 * If it is a UDP client object, it has a UDP listener socket
	 * and an outstanding UDP receive request.
	 */

	NS_CLIENTSTATE_WORKING = 3,
	/*%<
	 * The client object has received a request and is working
	 * on it.  It has a view, and it may have any of a non-reset OPT,
	 * recursion quota, and an outstanding write request.
	 */

	NS_CLIENTSTATE_RECURSING = 4,
	/*%<
	 * The client object is recursing.  It will be on the
	 * 'recursing' list.
	 */

	NS_CLIENTSTATE_MAX = 5
	/*%<
	 * Sentinel value used to indicate "no state".
	 */
} ns_clientstate_t;

typedef ISC_LIST(ns_client_t) client_list_t;

/*% nameserver client manager structure */
struct ns_clientmgr {
	/* Unlocked. */
	unsigned int magic;

	isc_mem_t      *mctx;
	ns_server_t    *sctx;
	isc_taskmgr_t  *taskmgr;
	isc_timermgr_t *timermgr;
	isc_task_t     *excl;
	isc_refcount_t	references;
	int		ncpus;

	/* Attached by clients, needed for e.g. recursion */
	isc_task_t **taskpool;

	ns_interface_t *interface;

	/* Lock covers manager state. */
	isc_mutex_t lock;
	bool	    exiting;

	/* Lock covers the recursing list */
	isc_mutex_t   reclock;
	client_list_t recursing; /*%< Recursing clients */

	/*%< mctx pool for clients. */
	isc_mem_t **mctxpool;
};

/*% nameserver client structure */
struct ns_client {
	unsigned int	 magic;
	isc_mem_t	*mctx;
	bool		 allocated; /* Do we need to free it? */
	ns_server_t	*sctx;
	ns_clientmgr_t	*manager;
	ns_clientstate_t state;
	int		 nupdates;
	bool		 nodetach;
	bool		 shuttingdown;
	unsigned int	 attributes;
	isc_task_t	*task;
	dns_view_t	*view;
	dns_dispatch_t	*dispatch;
	isc_nmhandle_t	*handle;	/* Permanent pointer to handle */
	isc_nmhandle_t	*sendhandle;	/* Waiting for send callback */
	isc_nmhandle_t	*reqhandle;	/* Waiting for request callback
					   (query, update, notify) */
	isc_nmhandle_t *fetchhandle;	/* Waiting for recursive fetch */
	isc_nmhandle_t *prefetchhandle; /* Waiting for prefetch / rpzfetch */
	isc_nmhandle_t *updatehandle;	/* Waiting for update callback */
	unsigned char  *tcpbuf;
	dns_message_t  *message;
	unsigned char  *sendbuf;
	dns_rdataset_t *opt;
	uint16_t	udpsize;
	uint16_t	extflags;
	int16_t		ednsversion; /* -1 noedns */
	void (*cleanup)(ns_client_t *);
	ns_query_t    query;
	isc_time_t    requesttime;
	isc_stdtime_t now;
	isc_time_t    tnow;
	dns_name_t    signername; /*%< [T]SIG key name */
	dns_name_t   *signer;	  /*%< NULL if not valid sig */
	bool	      mortal;	  /*%< Die after handling request */
	isc_quota_t  *recursionquota;

	isc_sockaddr_t peeraddr;
	bool	       peeraddr_valid;
	isc_netaddr_t  destaddr;
	isc_sockaddr_t destsockaddr;

	dns_ecs_t ecs; /*%< EDNS client subnet sent by client */

	struct in6_pktinfo pktinfo;
	isc_dscp_t	   dscp;
	/*%
	 * Information about recent FORMERR response(s), for
	 * FORMERR loop avoidance.  This is separate for each
	 * client object rather than global only to avoid
	 * the need for locking.
	 */
	struct {
		isc_sockaddr_t	addr;
		isc_stdtime_t	time;
		dns_messageid_t id;
	} formerrcache;

	/*% Callback function to send a response when unit testing */
	void (*sendcb)(isc_buffer_t *buf);

	ISC_LINK(ns_client_t) rlink;
	unsigned char  cookie[8];
	uint32_t       expire;
	unsigned char *keytag;
	uint16_t       keytag_len;

	/*%
	 * Used to override the DNS response code in ns_client_error().
	 * If set to -1, the rcode is determined from the result code,
	 * but if set to any other value, the least significant 12
	 * bits will be used as the rcode in the response message.
	 */
	int32_t rcode_override;
};

#define NS_CLIENT_MAGIC	   ISC_MAGIC('N', 'S', 'C', 'c')
#define NS_CLIENT_VALID(c) ISC_MAGIC_VALID(c, NS_CLIENT_MAGIC)

#define NS_CLIENTATTR_TCP	 0x00001
#define NS_CLIENTATTR_RA	 0x00002 /*%< Client gets recursive service */
#define NS_CLIENTATTR_PKTINFO	 0x00004 /*%< pktinfo is valid */
#define NS_CLIENTATTR_MULTICAST	 0x00008 /*%< recv'd from multicast */
#define NS_CLIENTATTR_WANTDNSSEC 0x00010 /*%< include dnssec records */
#define NS_CLIENTATTR_WANTNSID	 0x00020 /*%< include nameserver ID */
/* Obsolete: NS_CLIENTATTR_FILTER_AAAA	0x00040 */
/* Obsolete: NS_CLIENTATTR_FILTER_AAAA_RC 0x00080 */
#define NS_CLIENTATTR_WANTAD	   0x00100 /*%< want AD in response if possible */
#define NS_CLIENTATTR_WANTCOOKIE   0x00200 /*%< return a COOKIE */
#define NS_CLIENTATTR_HAVECOOKIE   0x00400 /*%< has a valid COOKIE */
#define NS_CLIENTATTR_WANTEXPIRE   0x00800 /*%< return seconds to expire */
#define NS_CLIENTATTR_HAVEEXPIRE   0x01000 /*%< return seconds to expire */
#define NS_CLIENTATTR_WANTOPT	   0x02000 /*%< add opt to reply */
#define NS_CLIENTATTR_HAVEECS	   0x04000 /*%< received an ECS option */
#define NS_CLIENTATTR_WANTPAD	   0x08000 /*%< pad reply */
#define NS_CLIENTATTR_USEKEEPALIVE 0x10000 /*%< use TCP keepalive */

#define NS_CLIENTATTR_NOSETFC 0x20000 /*%< don't set servfail cache */

/*
 * Flag to use with the SERVFAIL cache to indicate
 * that a query had the CD bit set.
 */
#define NS_FAILCACHE_CD 0x01

#if (defined(_WIN32) && !defined(_WIN64)) || !defined(_LP64)
LIBNS_EXTERNAL_DATA extern atomic_uint_fast32_t ns_client_requests;
#else  /* if defined(_WIN32) && !defined(_WIN64) */
LIBNS_EXTERNAL_DATA extern atomic_uint_fast64_t ns_client_requests;
#endif /* if defined(_WIN32) && !defined(_WIN64) */

/***
 *** Functions
 ***/

/*
 * Note!  These ns_client_ routines MUST be called ONLY from the client's
 * task in order to ensure synchronization.
 */

void
ns_client_send(ns_client_t *client);
/*%<
 * Finish processing the current client request and
 * send client->message as a response.
 * \brief
 * Note!  These ns_client_ routines MUST be called ONLY from the client's
 * task in order to ensure synchronization.
 */

void
ns_client_sendraw(ns_client_t *client, dns_message_t *msg);
/*%<
 * Finish processing the current client request and
 * send msg as a response using client->message->id for the id.
 */

void
ns_client_error(ns_client_t *client, isc_result_t result);
/*%<
 * Finish processing the current client request and return
 * an error response to the client.  The error response
 * will have an RCODE determined by 'result'.
 */

void
ns_client_drop(ns_client_t *client, isc_result_t result);
/*%<
 * Log the reason the current client request has failed; no response
 * will be sent.
 */

bool
ns_client_shuttingdown(ns_client_t *client);
/*%<
 * Return true iff the client is currently shutting down.
 */

isc_result_t
ns_client_replace(ns_client_t *client);
/*%<
 * Try to replace the current client with a new one, so that the
 * current one can go off and do some lengthy work without
 * leaving the dispatch/socket without service.
 */

void
ns_client_settimeout(ns_client_t *client, unsigned int seconds);
/*%<
 * Set a timer in the client to go off in the specified amount of time.
 */

isc_result_t
ns_clientmgr_create(isc_mem_t *mctx, ns_server_t *sctx, isc_taskmgr_t *taskmgr,
		    isc_timermgr_t *timermgr, ns_interface_t *ifp, int ncpus,
		    ns_clientmgr_t **managerp);
/*%<
 * Create a client manager.
 */

void
ns_clientmgr_shutdown(ns_clientmgr_t *manager);
/*%<
 * Shutdown a client manager and all ns_client_t objects
 * managed by it.
 */

void
ns_clientmgr_destroy(ns_clientmgr_t **managerp);
/*%<
 * Destroy a client manager.
 */

isc_sockaddr_t *
ns_client_getsockaddr(ns_client_t *client);
/*%<
 * Get the socket address of the client whose request is
 * currently being processed.
 */

isc_sockaddr_t *
ns_client_getdestaddr(ns_client_t *client);
/*%<
 * Get the destination address (server) for the request that is
 * currently being processed.
 */

isc_result_t
ns_client_checkaclsilent(ns_client_t *client, isc_netaddr_t *netaddr,
			 dns_acl_t *acl, bool default_allow);

/*%<
 * Convenience function for client request ACL checking.
 *
 * Check the current client request against 'acl'.  If 'acl'
 * is NULL, allow the request iff 'default_allow' is true.
 * If netaddr is NULL, check the ACL against client->peeraddr;
 * otherwise check it against netaddr.
 *
 * Notes:
 *\li	This is appropriate for checking allow-update,
 * 	allow-query, allow-transfer, etc.  It is not appropriate
 * 	for checking the blackhole list because we treat positive
 * 	matches as "allow" and negative matches as "deny"; in
 *	the case of the blackhole list this would be backwards.
 *
 * Requires:
 *\li	'client' points to a valid client.
 *\li	'netaddr' points to a valid address, or is NULL.
 *\li	'acl' points to a valid ACL, or is NULL.
 *
 * Returns:
 *\li	ISC_R_SUCCESS	if the request should be allowed
 * \li	DNS_R_REFUSED	if the request should be denied
 *\li	No other return values are possible.
 */

isc_result_t
ns_client_checkacl(ns_client_t *client, isc_sockaddr_t *sockaddr,
		   const char *opname, dns_acl_t *acl, bool default_allow,
		   int log_level);
/*%<
 * Like ns_client_checkaclsilent, except the outcome of the check is
 * logged at log level 'log_level' if denied, and at debug 3 if approved.
 * Log messages will refer to the request as an 'opname' request.
 *
 * Requires:
 *\li	'client' points to a valid client.
 *\li	'sockaddr' points to a valid address, or is NULL.
 *\li	'acl' points to a valid ACL, or is NULL.
 *\li	'opname' points to a null-terminated string.
 */

void
ns_client_log(ns_client_t *client, isc_logcategory_t *category,
	      isc_logmodule_t *module, int level, const char *fmt, ...)
	ISC_FORMAT_PRINTF(5, 6);

void
ns_client_logv(ns_client_t *client, isc_logcategory_t *category,
	       isc_logmodule_t *module, int level, const char *fmt, va_list ap)
	ISC_FORMAT_PRINTF(5, 0);

void
ns_client_aclmsg(const char *msg, const dns_name_t *name, dns_rdatatype_t type,
		 dns_rdataclass_t rdclass, char *buf, size_t len);

#define NS_CLIENT_ACLMSGSIZE(x)                           \
	(DNS_NAME_FORMATSIZE + DNS_RDATATYPE_FORMATSIZE + \
	 DNS_RDATACLASS_FORMATSIZE + sizeof(x) + sizeof("'/'"))

void
ns_client_recursing(ns_client_t *client);
/*%<
 * Add client to end of th recursing list.
 */

void
ns_client_killoldestquery(ns_client_t *client);
/*%<
 * Kill the oldest recursive query (recursing list head).
 */

void
ns_client_dumprecursing(FILE *f, ns_clientmgr_t *manager);
/*%<
 * Dump the outstanding recursive queries to 'f'.
 */

void
ns_client_qnamereplace(ns_client_t *client, dns_name_t *name);
/*%<
 * Replace the qname.
 */

isc_result_t
ns_client_sourceip(dns_clientinfo_t *ci, isc_sockaddr_t **addrp);

isc_result_t
ns_client_addopt(ns_client_t *client, dns_message_t *message,
		 dns_rdataset_t **opt);

/*%<
 * Get a client object from the inactive queue, or create one, as needed.
 * (Not intended for use outside this module and associated tests.)
 */

void
ns__client_request(isc_nmhandle_t *handle, isc_result_t eresult,
		   isc_region_t *region, void *arg);

/*%<
 * Handle client requests.
 * (Not intended for use outside this module and associated tests.)
 */

isc_result_t
ns__client_tcpconn(isc_nmhandle_t *handle, isc_result_t result, void *arg);

/*%<
 * Called every time a TCP connection is establish.  This is used for
 * updating TCP statistics.
 */

dns_rdataset_t *
ns_client_newrdataset(ns_client_t *client);

void
ns_client_putrdataset(ns_client_t *client, dns_rdataset_t **rdatasetp);
/*%<
 * Get and release temporary rdatasets in the client message;
 * used in query.c and in plugins.
 */

isc_result_t
ns_client_newnamebuf(ns_client_t *client);
/*%<
 * Allocate a name buffer for the client message.
 */

dns_name_t *
ns_client_newname(ns_client_t *client, isc_buffer_t *dbuf, isc_buffer_t *nbuf);
/*%<
 * Get a temporary name for the client message.
 */

isc_buffer_t *
ns_client_getnamebuf(ns_client_t *client);
/*%<
 * Get a name buffer from the pool, or allocate a new one if needed.
 */

void
ns_client_keepname(ns_client_t *client, dns_name_t *name, isc_buffer_t *dbuf);
/*%<
 * Adjust buffer 'dbuf' to reflect that 'name' is using space in it,
 * and set client attributes appropriately.
 */

void
ns_client_releasename(ns_client_t *client, dns_name_t **namep);
/*%<
 * Release 'name' back to the pool of temporary names for the client
 * message. If it is using a name buffer, relinquish its exclusive
 * rights on the buffer.
 */

isc_result_t
ns_client_newdbversion(ns_client_t *client, unsigned int n);
/*%<
 * Allocate 'n' new database versions for use by client queries.
 */

ns_dbversion_t *
ns_client_getdbversion(ns_client_t *client);
/*%<
 * Get a free database version for use by a client query, allocating
 * a new one if necessary.
 */

ns_dbversion_t *
ns_client_findversion(ns_client_t *client, dns_db_t *db);
/*%<
 * Find the correct database version to use with a client query.
 * If we have already done a query related to the database 'db',
 * make sure subsequent queries are from the same version;
 * otherwise, take a database version from the list of dbversions
 * allocated by ns_client_newdbversion().
 */

isc_result_t
ns__client_setup(ns_client_t *client, ns_clientmgr_t *manager, bool new);
/*%<
 * Perform initial setup of an allocated client.
 */

void
ns__client_reset_cb(void *client0);
/*%<
 * Reset the client object so that it can be reused.
 */

void
ns__client_put_cb(void *client0);
/*%<
 * Free all resources allocated to this client object, so that
 * it can be freed.
 */

#endif /* NS_CLIENT_H */
