/*	$NetBSD: adb.h,v 1.8 2025/01/26 16:25:26 christos Exp $	*/

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

#pragma once

/*****
***** Module Info
*****/

/*! \file dns/adb.h
 *\brief
 * DNS Address Database
 *
 * This module implements an address database (ADB) for mapping a name
 * to an isc_sockaddr_t. It also provides statistical information on
 * how good that address might be.
 *
 * A client will pass in a dns_name_t, and the ADB will walk through
 * the rdataset looking up addresses associated with the name.  If it
 * is found on the internal lists, a structure is filled in with the
 * address information and stats for found addresses.
 *
 * If the name cannot be found on the internal lists, a new entry will
 * be created for a name if all the information needed can be found
 * in the zone table or cache.  This new address will then be returned.
 *
 * If a request must be made to remote servers to satisfy a name lookup,
 * this module will start fetches to try to complete these addresses.  When
 * at least one more completes, an event is sent to the caller.  If none of
 * them resolve before the fetch times out, an event indicating this is
 * sent instead.
 *
 * Records are stored internally until a timer expires. The timer is the
 * smaller of the TTL or signature validity period.
 *
 * MP:
 *
 *\li	The ADB takes care of all necessary locking.
 *
 *\li	Only the task which initiated the name lookup can cancel the lookup.
 *
 *
 * Security:
 *
 *\li	None, since all data stored is required to be pre-filtered.
 *	(Cache needs to be sane, fetches return bounds-checked and sanity-
 *       checked data, caller passes a good dns_name_t for the zone, etc)
 */

/***
 *** Imports
 ***/

/* Add -DDNS_ADB_TRACE=1 to CFLAGS for detailed reference tracing */

#include <inttypes.h>
#include <stdbool.h>

#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/sockaddr.h>

#include <dns/types.h>
#include <dns/view.h>

ISC_LANG_BEGINDECLS

/***
 *** Magic number checks
 ***/

#define DNS_ADBFIND_MAGIC	 ISC_MAGIC('a', 'd', 'b', 'H')
#define DNS_ADBFIND_VALID(x)	 ISC_MAGIC_VALID(x, DNS_ADBFIND_MAGIC)
#define DNS_ADBADDRINFO_MAGIC	 ISC_MAGIC('a', 'd', 'A', 'I')
#define DNS_ADBADDRINFO_VALID(x) ISC_MAGIC_VALID(x, DNS_ADBADDRINFO_MAGIC)

/***
 *** TYPES
 ***/

typedef struct dns_adbname dns_adbname_t;

typedef enum {
	DNS_ADB_UNSET = 0,
	DNS_ADB_MOREADDRESSES,
	DNS_ADB_NOMOREADDRESSES,
	DNS_ADB_EXPIRED,
	DNS_ADB_CANCELED,
	DNS_ADB_SHUTTINGDOWN
} dns_adbstatus_t;

/*!
 *\brief
 * Represents a lookup for a single name.
 *
 * On return, the client can safely use "list", and can reorder the list.
 * Items may not be _deleted_ from this list, however, or added to it
 * other than by using the dns_adb_*() API.
 */
struct dns_adbfind {
	/* Public */
	unsigned int	      magic;	      /*%< RO: magic */
	dns_adbaddrinfolist_t list;	      /*%< RO: list of addrs */
	unsigned int	      query_pending;  /*%< RO: partial list */
	unsigned int	      partial_result; /*%< RO: addrs missing */
	unsigned int	      options;	      /*%< RO: options */
	isc_result_t	      result_v4;      /*%< RO: v4 result */
	isc_result_t	      result_v6;      /*%< RO: v6 result */
	ISC_LINK(dns_adbfind_t) publink;      /*%< RW: client use */

	/* Private */
	isc_mutex_t		 lock; /* locks all below */
	in_port_t		 port;
	unsigned int		 flags;
	dns_adbname_t		*adbname;
	dns_adb_t		*adb;
	isc_loop_t		*loop;
	_Atomic(dns_adbstatus_t) status;
	isc_job_cb		 cb;
	void			*cbarg;
	ISC_LINK(dns_adbfind_t) plink;
};

/*
 * _INET:
 * _INET6:
 *	return addresses of that type.
 *
 * _EMPTYEVENT:
 *	Only schedule an event if no addresses are known.
 *	Must set _WANTEVENT for this to be meaningful.
 *
 * _WANTEVENT:
 *	An event is desired.  Check this bit in the returned find to see
 *	if one will actually be generated.
 *
 * _AVOIDFETCHES:
 *	If set, fetches will not be generated unless no addresses are
 *	available in any of the address families requested.
 *
 * _STARTATZONE:
 *	Fetches will start using the closest zone data or use the root servers.
 *	This is useful for reestablishing glue that has expired.
 */
/*% Return addresses of type INET. */
#define DNS_ADBFIND_INET 0x00000001
/*% Return addresses of type INET6. */
#define DNS_ADBFIND_INET6	0x00000002
#define DNS_ADBFIND_ADDRESSMASK 0x00000003
/*%
 *      Only schedule an event if no addresses are known.
 *      Must set _WANTEVENT for this to be meaningful.
 */
#define DNS_ADBFIND_EMPTYEVENT 0x00000004
/*%
 *	An event is desired.  Check this bit in the returned find to see
 *	if one will actually be generated.
 */
#define DNS_ADBFIND_WANTEVENT 0x00000008
/*%
 *	If set, fetches will not be generated unless no addresses are
 *	available in any of the address families requested.
 */
#define DNS_ADBFIND_AVOIDFETCHES 0x00000010
/*%
 *	Fetches will start using the closest zone data or use the root servers.
 *	This is useful for reestablishing glue that has expired.
 */
#define DNS_ADBFIND_STARTATZONE 0x00000020
/*%
 *	Fetches will be exempted from the quota.
 */
#define DNS_ADBFIND_QUOTAEXEMPT 0x00000040
/*%
 *      The server's fetch quota is exceeded; it will be treated as
 *      lame for this query.
 */
#define DNS_ADBFIND_OVERQUOTA 0x00000400
/*%
 *	Don't perform a fetch even if there are no address records available.
 */
#define DNS_ADBFIND_NOFETCH 0x00000800
/*%
 *	Only look for glue record for static stub.
 */
#define DNS_ADBFIND_STATICSTUB 0x00001000

/*%
 * The answers to queries come back as a list of these.
 */
struct dns_adbaddrinfo {
	unsigned int magic; /*%< private */

	isc_sockaddr_t	 sockaddr; /*%< [rw] */
	unsigned int	 srtt;	   /*%< [rw] microsecs */
	dns_transport_t *transport;

	unsigned int	flags; /*%< [rw] */
	dns_adbentry_t *entry; /*%< private */
	ISC_LINK(dns_adbaddrinfo_t) publink;
};

/*!<
 * When the caller recieves a callback from dns_adb_createfind(), the
 * argument will a pointer to the dns_adbfind_t structure, which includes
 * this includes a copy of the callback function and argument passed to
 * dns_adb_createfind(), and a dns_adbstatus_t in the 'status' field,
 * which indicates one of the following:
 *
 *\li	#DNS_ADB_MOREADDRESSES   -- another address resolved.
 *\li	#DNS_ADB_NOMOREADDRESSES -- all pending addresses failed,
 *				    were canceled, or otherwise will
 *				    not be usable.
 *\li	#DNS_ADB_CANCELED	 -- The request was canceled by a
 *				    3rd party.
 *\li	#DNS_ADB_EXPIRED	 -- The name was expired, so this request
 *				    was canceled.
 *
 * In each of these cases, the addresses returned by the initial call
 * to dns_adb_createfind() can still be used until they are no longer needed.
 */

/****
 **** FUNCTIONS
 ****/

void
dns_adb_create(isc_mem_t *mem, dns_view_t *view, dns_adb_t **newadb);
/*%<
 * Create a new ADB.
 *
 * Notes:
 *
 *\li	Generally, applications should not create an ADB directly, but
 *	should instead call dns_view_createresolver().
 *
 * Requires:
 *
 *\li	'mem' must be a valid memory context.
 *
 *\li	'view' be a pointer to a valid view.
 *
 *\li	'newadb' != NULL && '*newadb' == NULL.
 */

#if DNS_ADB_TRACE
#define dns_adb_ref(ptr)   dns_adb__ref(ptr, __func__, __FILE__, __LINE__)
#define dns_adb_unref(ptr) dns_adb__unref(ptr, __func__, __FILE__, __LINE__)
#define dns_adb_attach(ptr, ptrp) \
	dns_adb__attach(ptr, ptrp, __func__, __FILE__, __LINE__)
#define dns_adb_detach(ptrp) dns_adb__detach(ptrp, __func__, __FILE__, __LINE__)
ISC_REFCOUNT_TRACE_DECL(dns_adb);
#else
ISC_REFCOUNT_DECL(dns_adb);
#endif

void
dns_adb_shutdown(dns_adb_t *adb);
/*%<
 * Shutdown 'adb'.
 *
 * Requires:
 *
 * \li	'*adb' is a valid dns_adb_t.
 */

isc_result_t
dns_adb_createfind(dns_adb_t *adb, isc_loop_t *loop, isc_job_cb cb, void *cbarg,
		   const dns_name_t *name, const dns_name_t *qname,
		   dns_rdatatype_t qtype, unsigned int options,
		   isc_stdtime_t now, dns_name_t *target, in_port_t port,
		   unsigned int depth, isc_counter_t *qc, dns_adbfind_t **find);
/*%<
 * Main interface for clients. The adb will look up the name given in
 * "name" and will build up a list of found addresses, and perhaps start
 * internal fetches to resolve names that are unknown currently.
 *
 * If other addresses resolve after this call completes, the 'cb' callback
 * will be called with a pointer to the dns_adbfind_t returned by this
 * structure, which in turn has a pointer to the callback argument passed
 * in as 'cbarg'. The caller is responsible for freeing the find object.
 *
 * If no events will be generated, the *find->result_v4 and/or result_v6
 * members may be examined for address lookup status.  The usual #ISC_R_SUCCESS,
 * #ISC_R_FAILURE, #DNS_R_NXDOMAIN, and #DNS_R_NXRRSET are returned, along with
 * #ISC_R_NOTFOUND meaning the ADB has not _yet_ found the values.  In this
 * latter case, retrying may produce more addresses.
 *
 * If events will be returned, the result_v[46] members are only valid
 * when that event is actually returned.
 *
 * The list of addresses returned is unordered.  The caller must impose
 * any ordering required.  The list will not contain "known bad" addresses,
 * however.
 *
 * The caller cannot (directly) modify the contents of the address list's
 * fields other than the "link" field.  All values can be read at any
 * time, however.
 *
 * The "now" parameter is used only for determining which entries that
 * have a specific time to live or expire time should be removed from
 * the running database.  If specified as zero, the current time will
 * be retrieved and used.
 *
 * If 'target' is not NULL and 'name' is an alias (i.e. the name is
 * CNAME'd or DNAME'd to another name), then 'target' will be updated with
 * the domain name that 'name' is aliased to.
 *
 * All addresses returned will have the sockaddr's port set to 'port.'
 * The caller may change them directly in the dns_adbaddrinfo_t since
 * they are copies of the internal address only.
 *
 * Requires:
 *
 *\li	*adb be a valid isc_adb_t object.
 *
 *\li	If events are to be sent, *loop be a valid loop,
 *	and cb != NULL.
 *
 *\li	*name is a valid dns_name_t.
 *
 *\li	qname != NULL and *qname be a valid dns_name_t.
 *
 *\li	target == NULL or target is a valid name with a buffer.
 *
 *\li	find != NULL && *find == NULL.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS	Addresses might have been returned, and events will be
 *			delivered for unresolved addresses.
 *\li	#ISC_R_NOMORE	Addresses might have been returned, but no events
 *			will ever be posted for this context.  This is only
 *			returned if task != NULL.
 *\li	#ISC_R_NOMEMORY	insufficient resources
 *\li	#DNS_R_ALIAS	'name' is an alias for another name.
 *
 * Notes:
 *
 *\li	No internal reference to "name" exists after this function
 *	returns.
 */

void
dns_adb_cancelfind(dns_adbfind_t *find);
/*%<
 * Cancels the find, and sends the event off to the caller.
 *
 * It is an error to call dns_adb_cancelfind() on a find where
 * no event is wanted, or will ever be sent.
 *
 * Note:
 *
 *\li	It is possible that the real completion event was posted just
 *	before the dns_adb_cancelfind() call was made.  In this case,
 *	dns_adb_cancelfind() will do nothing.  The event callback needs
 *	to be prepared to find this situation (i.e. result is valid but
 *	the caller expects it to be canceled).
 *
 * Requires:
 *
 *\li	'find' be a valid dns_adbfind_t pointer.
 *
 *\li	events would have been posted to the task.  This can be checked
 *	with (find->options & DNS_ADBFIND_WANTEVENT).
 *
 * Ensures:
 *
 *\li	The event was posted to the task.
 */

void
dns_adbfind_done(dns_adbfind_t find);
/*%<
 * Marks a find as ready to free.
 *
 * Requires:
 *
 *\li	'find' != NULL and *find be valid dns_adbfind_t pointer.
 */

unsigned int
dns_adb_findstatus(dns_adbfind_t *);
/*%<
 * Returns the status field of the find.
 *
 * Requires:
 *
 *\li	'find' be a valid dns_adbfind_t pointer.
 */

void
dns_adb_destroyfind(dns_adbfind_t **find);
/*%<
 * Destroys the find reference.
 *
 * Note:
 *
 *\li	This can only be called after the event was delivered for a
 *	find.
 *
 * Requires:
 *
 *\li	'find' != NULL and *find be valid dns_adbfind_t pointer.
 *
 * Ensures:
 *
 *\li	No "address found" events will be posted to the originating task
 *	after this function returns.
 */

void
dns_adb_dump(dns_adb_t *adb, FILE *f);
/*%<
 * Used by "rndc dumpdb": Dump the state of the running ADB.
 *
 * Requires:
 *
 *\li	adb is valid.
 *
 *\li	f != NULL, and is a file open for writing.
 */

/*
 * Reasonable defaults for RTT adjustments
 *
 * (Note: these values function both as scaling factors and as
 * indicators of the type of RTT adjustment operation taking place.
 * Adjusting the scaling factors is fine, as long as they all remain
 * unique values.)
 */
#define DNS_ADB_RTTADJDEFAULT 7	 /*%< default scale */
#define DNS_ADB_RTTADJREPLACE 0	 /*%< replace with our rtt */
#define DNS_ADB_RTTADJAGE     10 /*%< age this rtt */

void
dns_adb_adjustsrtt(dns_adb_t *adb, dns_adbaddrinfo_t *addr, unsigned int rtt,
		   unsigned int factor);
/*%<
 * Mix the round trip time into the existing smoothed rtt.
 *
 * Requires:
 *
 *\li	adb be valid.
 *
 *\li	addr be valid.
 *
 *\li	0 <= factor <= 10
 *
 * Note:
 *
 *\li	The srtt in addr will be updated to reflect the new global
 *	srtt value.  This may include changes made by others.
 */

void
dns_adb_agesrtt(dns_adb_t *adb, dns_adbaddrinfo_t *addr, isc_stdtime_t now);
/*
 * dns_adb_agesrtt is equivalent to dns_adb_adjustsrtt with factor
 * equal to DNS_ADB_RTTADJAGE and the current time passed in.
 *
 * Requires:
 *
 *\li	adb be valid.
 *
 *\li	addr be valid.
 *
 * Note:
 *
 *\li	The srtt in addr will be updated to reflect the new global
 *	srtt value.  This may include changes made by others.
 */

void
dns_adb_changeflags(dns_adb_t *adb, dns_adbaddrinfo_t *addr, unsigned int bits,
		    unsigned int mask);
/*%
 * Change Flags.
 *
 * Set the flags as given by:
 *
 *\li	newflags = (oldflags & ~mask) | (bits & mask);
 *
 * Requires:
 *
 *\li	adb be valid.
 *
 *\li	addr be valid.
 */

void
dns_adb_setudpsize(dns_adb_t *adb, dns_adbaddrinfo_t *addr, unsigned int size);
/*%
 * Update seen UDP response size.  The largest seen will be returned by
 * dns_adb_getudpsize().
 *
 * Requires:
 *
 *\li	adb be valid.
 *
 *\li	addr be valid.
 */

unsigned int
dns_adb_getudpsize(dns_adb_t *adb, dns_adbaddrinfo_t *addr);
/*%
 * Return the largest seen UDP response size.
 *
 * Requires:
 *
 *\li	adb be valid.
 *
 *\li	addr be valid.
 */

void
dns_adb_plainresponse(dns_adb_t *adb, dns_adbaddrinfo_t *addr);
/*%
 * Record a successful plain DNS response.
 *
 * Requires:
 *
 *\li	adb be valid.
 *
 *\li	addr be valid.
 */

void
dns_adb_timeout(dns_adb_t *adb, dns_adbaddrinfo_t *addr);
/*%
 * Record a plain DNS UDP query failed.
 *
 * Requires:
 *
 *\li	adb be valid.
 *
 *\li	addr be valid.
 */

void
dns_adb_ednsto(dns_adb_t *adb, dns_adbaddrinfo_t *addr);
/*%
 * Record a EDNS UDP query failed.
 *
 * Requires:
 *
 *\li	adb be valid.
 *
 *\li	addr be valid.
 */

isc_result_t
dns_adb_findaddrinfo(dns_adb_t *adb, const isc_sockaddr_t *sa,
		     dns_adbaddrinfo_t **addrp, isc_stdtime_t now);
/*%<
 * Return a dns_adbaddrinfo_t that is associated with address 'sa'.
 *
 * Requires:
 *
 *\li	adb is valid.
 *
 *\li	sa is valid.
 *
 *\li	addrp != NULL && *addrp == NULL
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_SHUTTINGDOWN
 */

void
dns_adb_freeaddrinfo(dns_adb_t *adb, dns_adbaddrinfo_t **addrp);
/*%<
 * Free a dns_adbaddrinfo_t allocated by dns_adb_findaddrinfo().
 *
 * Requires:
 *
 *\li	adb is valid.
 *
 *\li	*addrp is a valid dns_adbaddrinfo_t *.
 */

void
dns_adb_flush(dns_adb_t *adb);
/*%<
 * Flushes all cached data from the adb.
 *
 * Requires:
 *\li 	adb is valid.
 */

void
dns_adb_setadbsize(dns_adb_t *adb, size_t size);
/*%<
 * Set a target memory size.  If memory usage exceeds the target
 * size entries will be removed before they would have expired on
 * a random basis.
 *
 * If 'size' is 0 then memory usage is unlimited.
 *
 * Requires:
 *\li	'adb' is valid.
 */

void
dns_adb_flushname(dns_adb_t *adb, const dns_name_t *name);
/*%<
 * Flush 'name' from the adb cache.
 *
 * Requires:
 *\li	'adb' is valid.
 *\li	'name' is valid.
 */

void
dns_adb_flushnames(dns_adb_t *adb, const dns_name_t *name);
/*%<
 * Flush 'name' and all subdomains from the adb cache.
 *
 * Requires:
 *\li	'adb' is valid.
 *\li	'name' is valid.
 */

void
dns_adb_setcookie(dns_adb_t *adb, dns_adbaddrinfo_t *addr,
		  const unsigned char *cookie, size_t len);
/*%<
 * Record the COOKIE associated with this address.  If
 * cookie is NULL or len is zero the recorded COOKIE is cleared.
 *
 * Requires:
 *\li	'adb' is valid.
 *\li	'addr' is valid.
 */

size_t
dns_adb_getcookie(dns_adbaddrinfo_t *addr, unsigned char *cookie, size_t len);
/*
 * If 'cookie' is not NULL, then retrieve the saved COOKIE value and store it
 * in 'cookie' which has size 'len'.
 *
 * Requires:
 *\li	'addr' is valid.
 *
 * Returns:
 *	The size of the cookie or zero if it doesn't exist, or when 'cookie' is
 *      not NULL and it doesn't fit in the buffer.
 */

void
dns_adb_setquota(dns_adb_t *adb, uint32_t quota, uint32_t freq, double low,
		 double high, double discount);
/*%<
 * Set the baseline ADB quota, and configure parameters for the
 * quota adjustment algorithm.
 *
 * If the number of fetches currently waiting for responses from this
 * address exceeds the current quota, then additional fetches are spilled.
 *
 * 'quota' is the highest permissible quota; it will adjust itself
 * downward in response to detected congestion.
 *
 * After every 'freq' fetches have either completed or timed out, an
 * exponentially weighted moving average of the ratio of timeouts
 * to responses is calculated.  If the EWMA goes above a 'high'
 * threshold, then the quota is adjusted down one step; if it drops
 * below a 'low' threshold, then the quota is adjusted back up one
 * step.
 *
 * The quota adjustment is based on the function (1 / 1 + (n/10)^(3/2)),
 * for values of n from 0 to 99.  It starts at 100% of the baseline
 * quota, and descends after 100 steps to 2%.
 *
 * 'discount' represents the discount rate of the moving average. Higher
 * values cause older values to be discounted sooner, providing a faster
 * response to changes in the timeout ratio.
 *
 * Requires:
 *\li	'adb' is valid.
 */

void
dns_adb_getquota(dns_adb_t *adb, uint32_t *quotap, uint32_t *freqp,
		 double *lowp, double *highp, double *discountp);
/*%<
 * Get the quota values set by dns_adb_setquota().
 * If any of the 'quotap', 'freqp', 'lowp', 'highp', and
 * 'discountp' parameters are non-NULL, then the memory they
 * point to will be updated to hold the corresponding quota
 * or parameter value.
 *
 * Requires:
 *\li	'adb' is valid.
 */

bool
dns_adb_overquota(dns_adb_t *adb, dns_adbaddrinfo_t *addr);
/*%<
 * Returns true if the specified ADB has too many active fetches.
 *
 * Requires:
 *\li	'entry' is valid.
 */

void
dns_adb_beginudpfetch(dns_adb_t *adb, dns_adbaddrinfo_t *addr);
void
dns_adb_endudpfetch(dns_adb_t *adb, dns_adbaddrinfo_t *addr);
/*%
 * Begin/end a UDP fetch on a particular address.
 *
 * These functions increment or decrement the fetch counter for
 * the ADB entry so that the fetch quota can be enforced.
 *
 * Requires:
 *
 *\li	adb be valid.
 *
 *\li	addr be valid.
 */

isc_stats_t *
dns_adb_getstats(dns_adb_t *adb);
/*%<
 * Get the adb statistics counter set for 'adb'.
 *
 * Requires:
 * \li 'adb' is valid.
 */

isc_result_t
dns_adb_dumpquota(dns_adb_t *adb, isc_buffer_t **buf);
/*%
 * Dump the addresses, current quota values, and current ATR values
 * for all servers that are currently being fetchlimited. Servers
 * for which the quota is still equal to the default and the ATR
 * is zero are not printed.
 *
 * Requires:
 * \li 'adb' is valid.
 */
ISC_LANG_ENDDECLS
