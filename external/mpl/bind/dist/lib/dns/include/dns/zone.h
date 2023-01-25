/*	$NetBSD: zone.h,v 1.10 2023/01/25 21:43:30 christos Exp $	*/

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

#ifndef DNS_ZONE_H
#define DNS_ZONE_H 1

/*! \file dns/zone.h */

/***
 ***	Imports
 ***/

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include <isc/formatcheck.h>
#include <isc/lang.h>
#include <isc/rwlock.h>

#include <dns/catz.h>
#include <dns/master.h>
#include <dns/masterdump.h>
#include <dns/rdatastruct.h>
#include <dns/rpz.h>
#include <dns/types.h>
#include <dns/zt.h>

typedef enum {
	dns_zone_none,
	dns_zone_primary,
	dns_zone_secondary,
	dns_zone_mirror,
	dns_zone_stub,
	dns_zone_staticstub,
	dns_zone_key,
	dns_zone_dlz,
	dns_zone_redirect
} dns_zonetype_t;

#ifndef dns_zone_master
#define dns_zone_master dns_zone_primary
#endif /* dns_zone_master */

#ifndef dns_zone_slave
#define dns_zone_slave dns_zone_secondary
#endif /* dns_zone_slave */

typedef enum {
	dns_zonestat_none = 0,
	dns_zonestat_terse,
	dns_zonestat_full
} dns_zonestat_level_t;

typedef enum {
	DNS_ZONEOPT_MANYERRORS = 1 << 0,    /*%< return many errors on load */
	DNS_ZONEOPT_IXFRFROMDIFFS = 1 << 1, /*%< calculate differences */
	DNS_ZONEOPT_NOMERGE = 1 << 2,	    /*%< don't merge journal */
	DNS_ZONEOPT_CHECKNS = 1 << 3,	    /*%< check if NS's are addresses */
	DNS_ZONEOPT_FATALNS = 1 << 4,	    /*%< DNS_ZONEOPT_CHECKNS is fatal */
	DNS_ZONEOPT_MULTIMASTER = 1 << 5,   /*%< this zone has multiple
						 primaries */
	DNS_ZONEOPT_USEALTXFRSRC = 1 << 6,  /*%< use alternate transfer sources
					     */
	DNS_ZONEOPT_CHECKNAMES = 1 << 7,    /*%< check-names */
	DNS_ZONEOPT_CHECKNAMESFAIL = 1 << 8, /*%< fatal check-name failures */
	DNS_ZONEOPT_CHECKWILDCARD = 1 << 9, /*%< check for internal wildcards */
	DNS_ZONEOPT_CHECKMX = 1 << 10,	    /*%< check-mx */
	DNS_ZONEOPT_CHECKMXFAIL = 1 << 11,  /*%< fatal check-mx failures */
	DNS_ZONEOPT_CHECKINTEGRITY = 1 << 12, /*%< perform integrity checks */
	DNS_ZONEOPT_CHECKSIBLING = 1 << 13, /*%< perform sibling glue checks */
	DNS_ZONEOPT_NOCHECKNS = 1 << 14,    /*%< disable IN NS address checks */
	DNS_ZONEOPT_WARNMXCNAME = 1 << 15,  /*%< warn on MX CNAME check */
	DNS_ZONEOPT_IGNOREMXCNAME = 1 << 16,  /*%< ignore MX CNAME check */
	DNS_ZONEOPT_WARNSRVCNAME = 1 << 17,   /*%< warn on SRV CNAME check */
	DNS_ZONEOPT_IGNORESRVCNAME = 1 << 18, /*%< ignore SRV CNAME check */
	DNS_ZONEOPT_UPDATECHECKKSK = 1 << 19, /*%< check dnskey KSK flag */
	DNS_ZONEOPT_TRYTCPREFRESH = 1 << 20, /*%< try tcp refresh on udp failure
					      */
	DNS_ZONEOPT_NOTIFYTOSOA = 1 << 21,   /*%< Notify the SOA MNAME */
	DNS_ZONEOPT_NSEC3TESTZONE = 1 << 22, /*%< nsec3-test-zone */
	DNS_ZONEOPT_SECURETOINSECURE = 1 << 23, /*%< dnssec-secure-to-insecure
						 */
	DNS_ZONEOPT_DNSKEYKSKONLY = 1 << 24,	/*%< dnssec-dnskey-kskonly */
	DNS_ZONEOPT_CHECKDUPRR = 1 << 25,	/*%< check-dup-records */
	DNS_ZONEOPT_CHECKDUPRRFAIL = 1 << 26,	/*%< fatal check-dup-records
						 * failures */
	DNS_ZONEOPT_CHECKSPF = 1 << 27,		/*%< check SPF records */
	DNS_ZONEOPT_CHECKTTL = 1 << 28,		/*%< check max-zone-ttl */
	DNS_ZONEOPT_AUTOEMPTY = 1 << 29,	/*%< automatic empty zone */
#ifndef __NetBSD__
	DNS_ZONEOPT___MAX = UINT64_MAX, /* trick to make the ENUM 64-bit wide */
#endif
} dns_zoneopt_t;

/*
 * Zone key maintenance options
 */
typedef enum {
	DNS_ZONEKEY_ALLOW = 0x00000001U,    /*%< fetch keys on command */
	DNS_ZONEKEY_MAINTAIN = 0x00000002U, /*%< publish/sign on schedule */
	DNS_ZONEKEY_CREATE = 0x00000004U,   /*%< make keys when needed */
	DNS_ZONEKEY_FULLSIGN = 0x00000008U, /*%< roll to new keys immediately */
	DNS_ZONEKEY_NORESIGN = 0x00000010U, /*%< no automatic resigning */
#ifndef __NetBSD__
	DNS_ZONEKEY___MAX = UINT64_MAX, /* trick to make the ENUM 64-bit wide */
#endif
} dns_zonekey_t;

#ifndef DNS_ZONE_MINREFRESH
#define DNS_ZONE_MINREFRESH 300 /*%< 5 minutes */
#endif				/* ifndef DNS_ZONE_MINREFRESH */
#ifndef DNS_ZONE_MAXREFRESH
#define DNS_ZONE_MAXREFRESH 2419200 /*%< 4 weeks */
#endif				    /* ifndef DNS_ZONE_MAXREFRESH */
#ifndef DNS_ZONE_DEFAULTREFRESH
#define DNS_ZONE_DEFAULTREFRESH 3600 /*%< 1 hour */
#endif				     /* ifndef DNS_ZONE_DEFAULTREFRESH */
#ifndef DNS_ZONE_MINRETRY
#define DNS_ZONE_MINRETRY 300 /*%< 5 minutes */
#endif			      /* ifndef DNS_ZONE_MINRETRY */
#ifndef DNS_ZONE_MAXRETRY
#define DNS_ZONE_MAXRETRY 1209600 /*%< 2 weeks */
#endif				  /* ifndef DNS_ZONE_MAXRETRY */
#ifndef DNS_ZONE_DEFAULTRETRY
#define DNS_ZONE_DEFAULTRETRY        \
	60 /*%< 1 minute, subject to \
	    * exponential backoff */
#endif	   /* ifndef DNS_ZONE_DEFAULTRETRY */

#define DNS_ZONESTATE_XFERRUNNING  1
#define DNS_ZONESTATE_XFERDEFERRED 2
#define DNS_ZONESTATE_SOAQUERY	   3
#define DNS_ZONESTATE_ANY	   4
#define DNS_ZONESTATE_AUTOMATIC	   5

ISC_LANG_BEGINDECLS

/***
 ***	Functions
 ***/

isc_result_t
dns_zone_create(dns_zone_t **zonep, isc_mem_t *mctx);
/*%<
 *	Creates a new empty zone and attach '*zonep' to it.
 *
 * Requires:
 *\li	'zonep' to point to a NULL pointer.
 *\li	'mctx' to be a valid memory context.
 *
 * Ensures:
 *\li	'*zonep' refers to a valid zone.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_UNEXPECTED
 */

void
dns_zone_setclass(dns_zone_t *zone, dns_rdataclass_t rdclass);
/*%<
 *	Sets the class of a zone.  This operation can only be performed
 *	once on a zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	dns_zone_setclass() not to have been called since the zone was
 *	created.
 *\li	'rdclass' != dns_rdataclass_none.
 */

dns_rdataclass_t
dns_zone_getclass(dns_zone_t *zone);
/*%<
 *	Returns the current zone class.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_getserial(dns_zone_t *zone, uint32_t *serialp);
/*%<
 *	Returns the current serial number of the zone.  On success, the SOA
 *	serial of the zone will be copied into '*serialp'.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 *\li	'serialp' to be non NULL
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#DNS_R_NOTLOADED	zone DB is not loaded
 */

void
dns_zone_settype(dns_zone_t *zone, dns_zonetype_t type);
/*%<
 *	Sets the zone type. This operation can only be performed once on
 *	a zone.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 *\li	dns_zone_settype() not to have been called since the zone was
 *	created.
 *\li	'type' != dns_zone_none
 */

void
dns_zone_setview(dns_zone_t *zone, dns_view_t *view);
/*%<
 *	Associate the zone with a view.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

dns_view_t *
dns_zone_getview(dns_zone_t *zone);
/*%<
 *	Returns the zone's associated view.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_setviewcommit(dns_zone_t *zone);
/*%<
 *	Commit the previous view saved internally via dns_zone_setview().
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_setviewrevert(dns_zone_t *zone);
/*%<
 *	Revert the most recent dns_zone_setview() on this zone,
 *	restoring the previous view.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setorigin(dns_zone_t *zone, const dns_name_t *origin);
/*%<
 *	Sets the zones origin to 'origin'.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'origin' to be non NULL.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li 	#ISC_R_NOMEMORY
 */

dns_name_t *
dns_zone_getorigin(dns_zone_t *zone);
/*%<
 *	Returns the value of the origin.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setfile(dns_zone_t *zone, const char *file, dns_masterformat_t format,
		 const dns_master_style_t *style);
/*%<
 *    Sets the name of the master file in the format of 'format' from which
 *    the zone loads its database to 'file'.
 *
 *    For zones that have no associated master file, 'file' will be NULL.
 *
 *	For zones with persistent databases, the file name
 *	setting is ignored.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_SUCCESS
 */

const char *
dns_zone_getfile(dns_zone_t *zone);
/*%<
 * 	Gets the name of the zone's master file, if any.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 *
 * Returns:
 *\li	Pointer to null-terminated file name, or NULL.
 */

void
dns_zone_setmaxrecords(dns_zone_t *zone, uint32_t records);
/*%<
 * 	Sets the maximum number of records permitted in a zone.
 *	0 implies unlimited.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 *
 * Returns:
 *\li	void
 */

uint32_t
dns_zone_getmaxrecords(dns_zone_t *zone);
/*%<
 * 	Gets the maximum number of records permitted in a zone.
 *	0 implies unlimited.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 *
 * Returns:
 *\li	uint32_t maxrecords.
 */

void
dns_zone_setmaxttl(dns_zone_t *zone, uint32_t maxttl);
/*%<
 * 	Sets the max ttl of the zone.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 *
 * Returns:
 *\li	void
 */

dns_ttl_t
dns_zone_getmaxttl(dns_zone_t *zone);
/*%<
 * 	Gets the max ttl of the zone.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 *
 * Returns:
 *\li	dns_ttl_t maxttl.
 */

void
dns_zone_lock_keyfiles(dns_zone_t *zone);
/*%<
 *	Lock associated keyfiles for this zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_unlock_keyfiles(dns_zone_t *zone);
/*%<
 *	Unlock associated keyfiles for this zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_load(dns_zone_t *zone, bool newonly);

isc_result_t
dns_zone_loadandthaw(dns_zone_t *zone);

/*%<
 *	Cause the database to be loaded from its backing store.
 *	Confirm that the minimum requirements for the zone type are
 *	met, otherwise DNS_R_BADZONE is returned.
 *
 *	If newonly is set dns_zone_load() only loads new zones.
 *	dns_zone_loadandthaw() is similar to dns_zone_load() but will
 *	also re-enable DNS UPDATEs when the load completes.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	#ISC_R_UNEXPECTED
 *\li	#ISC_R_SUCCESS
 *\li	DNS_R_CONTINUE	  Incremental load has been queued.
 *\li	DNS_R_UPTODATE	  The zone has already been loaded based on
 *			  file system timestamps.
 *\li	DNS_R_BADZONE
 *\li	Any result value from dns_db_load().
 */

isc_result_t
dns_zone_asyncload(dns_zone_t *zone, bool newonly, dns_zt_zoneloaded_t done,
		   void *arg);
/*%<
 * Cause the database to be loaded from its backing store asynchronously.
 * Other zone maintenance functions are suspended until this is complete.
 * When finished, 'done' is called to inform the caller, with 'arg' as
 * its first argument and 'zone' as its second.  (Normally, 'arg' is
 * expected to point to the zone table but is left undefined for testing
 * purposes.)
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	#ISC_R_ALREADYRUNNING
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_FAILURE
 *\li	#ISC_R_NOMEMORY
 */

bool
dns__zone_loadpending(dns_zone_t *zone);
/*%<
 * Indicates whether the zone is waiting to be loaded asynchronously.
 * (Not currently intended for use outside of this module and associated
 * tests.)
 */

void
dns_zone_attach(dns_zone_t *source, dns_zone_t **target);
/*%<
 *	Attach '*target' to 'source' incrementing its external
 * 	reference count.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'target' to be non NULL and '*target' to be NULL.
 */

void
dns_zone_detach(dns_zone_t **zonep);
/*%<
 *	Detach from a zone decrementing its external reference count.
 *	If this was the last external reference to the zone it will be
 * 	shut down and eventually freed.
 *
 * Require:
 *\li	'zonep' to point to a valid zone.
 */

void
dns_zone_iattach(dns_zone_t *source, dns_zone_t **target);
/*%<
 *	Attach '*target' to 'source' incrementing its internal
 * 	reference count.  This is intended for use by operations
 * 	such as zone transfers that need to prevent the zone
 * 	object from being freed but not from shutting down.
 *
 * Require:
 *\li	The caller is running in the context of the zone's task.
 *\li	'zone' to be a valid zone.
 *\li	'target' to be non NULL and '*target' to be NULL.
 */

void
dns_zone_idetach(dns_zone_t **zonep);
/*%<
 *	Detach from a zone decrementing its internal reference count.
 *	If there are no more internal or external references to the
 * 	zone, it will be freed.
 *
 * Require:
 *\li	The caller is running in the context of the zone's task.
 *\li	'zonep' to point to a valid zone.
 */

isc_result_t
dns_zone_getdb(dns_zone_t *zone, dns_db_t **dbp);
/*%<
 * 	Attach '*dbp' to the database to if it exists otherwise
 *	return DNS_R_NOTLOADED.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'dbp' to be != NULL && '*dbp' == NULL.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	DNS_R_NOTLOADED
 */

void
dns_zone_setdb(dns_zone_t *zone, dns_db_t *db);
/*%<
 *	Sets the zone database to 'db'.
 *
 *	This function is expected to be used to configure a zone with a
 *	database which is not loaded from a file or zone transfer.
 *	It can be used for a general purpose zone, but right now its use
 *	is limited to static-stub zones to avoid possible undiscovered
 *	problems in the general cases.
 *
 * Require:
 *\li	'zone' to be a valid zone of static-stub.
 *\li	zone doesn't have a database.
 */

void
dns_zone_setdbtype(dns_zone_t *zone, unsigned int dbargc,
		   const char *const *dbargv);
/*%<
 *	Sets the database type to dbargv[0] and database arguments
 *	to subsequent dbargv elements.
 *	'db_type' is not checked to see if it is a valid database type.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'database' to be non NULL.
 *\li	'dbargc' to be >= 1
 *\li	'dbargv' to point to dbargc NULL-terminated strings
 */

isc_result_t
dns_zone_getdbtype(dns_zone_t *zone, char ***argv, isc_mem_t *mctx);
/*%<
 *	Returns the current dbtype.  isc_mem_free() should be used
 * 	to free 'argv' after use.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'argv' to be non NULL and *argv to be NULL.
 *\li	'mctx' to be valid.
 *
 * Returns:
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_SUCCESS
 */

void
dns_zone_markdirty(dns_zone_t *zone);
/*%<
 *	Mark a zone as 'dirty'.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_expire(dns_zone_t *zone);
/*%<
 *	Mark the zone as expired.  If the zone requires dumping cause it to
 *	be initiated.  Set the refresh and retry intervals to there default
 *	values and unload the zone.
 *
 * Require
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_refresh(dns_zone_t *zone);
/*%<
 *	Initiate zone up to date checks.  The zone must already be being
 *	managed.
 *
 * Require
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_flush(dns_zone_t *zone);
/*%<
 *	Write the zone to database if there are uncommitted changes.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_dump(dns_zone_t *zone);
/*%<
 *	Write the zone to database.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_dumptostream(dns_zone_t *zone, FILE *fd, dns_masterformat_t format,
		      const dns_master_style_t *style,
		      const uint32_t		rawversion);
/*%<
 *    Write the zone to stream 'fd' in the specified 'format'.
 *    If the 'format' is dns_masterformat_text (RFC1035), 'style' also
 *    specifies the file style (e.g., &dns_master_style_default).
 *
 *    dns_zone_dumptostream() is a backward-compatible form of
 *    dns_zone_dumptostream2(), which always uses the dns_masterformat_text
 *    format and the dns_master_style_default style.
 *
 *    dns_zone_dumptostream2() is a backward-compatible form of
 *    dns_zone_dumptostream3(), which always uses the current
 *    default raw file format version.
 *
 *    Note that dns_zone_dumptostream3() is the most flexible form.  It
 *    can also provide the functionality of dns_zone_fulldumptostream().
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'fd' to be a stream open for writing.
 */

void
dns_zone_maintenance(dns_zone_t *zone);
/*%<
 *	Perform regular maintenance on the zone.  This is called as a
 *	result of a zone being managed.
 *
 * Require
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setprimaries(dns_zone_t *zone, const isc_sockaddr_t *primaries,
		      uint32_t count);
isc_result_t
dns_zone_setprimarieswithkeys(dns_zone_t *zone, const isc_sockaddr_t *primaries,
			      dns_name_t **keynames, uint32_t count);
/*%<
 *	Set the list of master servers for the zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'primaries' array of isc_sockaddr_t with port set or NULL.
 *\li	'count' the number of primaries.
 *\li	'keynames' array of dns_name_t's for tsig keys or NULL.
 *
 *\li	If 'primaries' is NULL then 'count' must be zero.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li      Any result dns_name_dup() can return, if keynames!=NULL
 */

isc_result_t
dns_zone_setparentals(dns_zone_t *zone, const isc_sockaddr_t *parentals,
		      dns_name_t **keynames, uint32_t count);
/*%<
 *	Set the list of parental agents for the zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'parentals' array of isc_sockaddr_t with port set or NULL.
 *\li	'count' the number of primaries.
 *\li	'keynames' array of dns_name_t's for tsig keys or NULL.
 *
 *\li	If 'parentals' is NULL then 'count' must be zero.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li      Any result dns_name_dup() can return, if keynames!=NULL
 */

isc_result_t
dns_zone_setalsonotify(dns_zone_t *zone, const isc_sockaddr_t *notify,
		       uint32_t count);
isc_result_t
dns_zone_setalsonotifywithkeys(dns_zone_t *zone, const isc_sockaddr_t *notify,
			       dns_name_t **keynames, uint32_t count);
isc_result_t
dns_zone_setalsonotifydscpkeys(dns_zone_t *zone, const isc_sockaddr_t *notify,
			       const isc_dscp_t *dscps, dns_name_t **keynames,
			       uint32_t count);
/*%<
 *	Set the list of additional servers to be notified when
 *	a zone changes.	 To clear the list use 'count = 0'.
 *
 *	dns_zone_alsonotifywithkeys() allows each notify address to
 *	be associated with a TSIG key.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'notify' to be non-NULL if count != 0.
 *\li	'count' to be the number of notifiees.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 */

void
dns_zone_unload(dns_zone_t *zone);
/*%<
 *	detach the database from the zone structure.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

dns_kasp_t *
dns_zone_getkasp(dns_zone_t *zone);
/*%<
 *	Returns the current kasp.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_setkasp(dns_zone_t *zone, dns_kasp_t *kasp);
/*%<
 *	Set kasp for zone.  If a kasp is already set, it will be detached.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_setoption(dns_zone_t *zone, dns_zoneopt_t option, bool value);
/*%<
 *	Set the given options on ('value' == true) or off
 *	('value' == #false).
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

dns_zoneopt_t
dns_zone_getoptions(dns_zone_t *zone);
/*%<
 *	Returns the current zone options.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_setkeyopt(dns_zone_t *zone, unsigned int option, bool value);
/*%<
 *	Set key options on ('value' == true) or off ('value' ==
 *	#false).
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

unsigned int
dns_zone_getkeyopts(dns_zone_t *zone);
/*%<
 *	Returns the current zone key options.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_setminrefreshtime(dns_zone_t *zone, uint32_t val);
/*%<
 *	Set the minimum refresh time.
 *
 * Requires:
 *\li	'zone' is valid.
 *\li	val > 0.
 */

void
dns_zone_setmaxrefreshtime(dns_zone_t *zone, uint32_t val);
/*%<
 *	Set the maximum refresh time.
 *
 * Requires:
 *\li	'zone' is valid.
 *\li	val > 0.
 */

void
dns_zone_setminretrytime(dns_zone_t *zone, uint32_t val);
/*%<
 *	Set the minimum retry time.
 *
 * Requires:
 *\li	'zone' is valid.
 *\li	val > 0.
 */

void
dns_zone_setmaxretrytime(dns_zone_t *zone, uint32_t val);
/*%<
 *	Set the maximum retry time.
 *
 * Requires:
 *\li	'zone' is valid.
 *	val > 0.
 */

isc_result_t
dns_zone_setxfrsource4(dns_zone_t *zone, const isc_sockaddr_t *xfrsource);
isc_result_t
dns_zone_setaltxfrsource4(dns_zone_t *zone, const isc_sockaddr_t *xfrsource);
/*%<
 * 	Set the source address to be used in IPv4 zone transfers.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'xfrsource' to contain the address.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_sockaddr_t *
dns_zone_getxfrsource4(dns_zone_t *zone);
isc_sockaddr_t *
dns_zone_getaltxfrsource4(dns_zone_t *zone);
/*%<
 *	Returns the source address set by a previous dns_zone_setxfrsource4
 *	call, or the default of inaddr_any, port 0.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setxfrsource4dscp(dns_zone_t *zone, isc_dscp_t dscp);
isc_result_t
dns_zone_setaltxfrsource4dscp(dns_zone_t *zone, isc_dscp_t dscp);
/*%<
 * Set the DSCP value associated with the transfer/alt-transfer source.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_dscp_t
dns_zone_getxfrsource4dscp(dns_zone_t *zone);
isc_dscp_t
dns_zone_getaltxfrsource4dscp(dns_zone_t *zone);
/*%/
 * Get the DSCP value associated with the transfer/alt-transfer source.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setxfrsource6(dns_zone_t *zone, const isc_sockaddr_t *xfrsource);
isc_result_t
dns_zone_setaltxfrsource6(dns_zone_t *zone, const isc_sockaddr_t *xfrsource);
/*%<
 * 	Set the source address to be used in IPv6 zone transfers.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'xfrsource' to contain the address.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_sockaddr_t *
dns_zone_getxfrsource6(dns_zone_t *zone);
isc_sockaddr_t *
dns_zone_getaltxfrsource6(dns_zone_t *zone);
/*%<
 *	Returns the source address set by a previous dns_zone_setxfrsource6
 *	call, or the default of in6addr_any, port 0.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_dscp_t
dns_zone_getxfrsource6dscp(dns_zone_t *zone);
isc_dscp_t
dns_zone_getaltxfrsource6dscp(dns_zone_t *zone);
/*%/
 * Get the DSCP value associated with the transfer/alt-transfer source.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setxfrsource6dscp(dns_zone_t *zone, isc_dscp_t dscp);
isc_result_t
dns_zone_setaltxfrsource6dscp(dns_zone_t *zone, isc_dscp_t dscp);
/*%<
 * Set the DSCP value associated with the transfer/alt-transfer source.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_result_t
dns_zone_setparentalsrc4(dns_zone_t *zone, const isc_sockaddr_t *parentalsrc);
/*%<
 * 	Set the source address to be used with IPv4 parental DS queries.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'parentalsrc' to contain the address.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_sockaddr_t *
dns_zone_getparentalsrc4(dns_zone_t *zone);
/*%<
 *	Returns the source address set by a previous dns_zone_setparentalsrc4
 *	call, or the default of inaddr_any, port 0.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_dscp_t
dns_zone_getparentalsrc4dscp(dns_zone_t *zone);
/*%/
 * Get the DSCP value associated with the IPv4 parental source.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setparentalsrc4dscp(dns_zone_t *zone, isc_dscp_t dscp);
/*%<
 * Set the DSCP value associated with the IPv4 parental source.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_result_t
dns_zone_setparentalsrc6(dns_zone_t *zone, const isc_sockaddr_t *parentalsrc);
/*%<
 * 	Set the source address to be used with IPv6 parental DS queries.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'parentalsrc' to contain the address.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_sockaddr_t *
dns_zone_getparentalsrc6(dns_zone_t *zone);
/*%<
 *	Returns the source address set by a previous dns_zone_setparentalsrc6
 *	call, or the default of in6addr_any, port 0.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_dscp_t
dns_zone_getparentalsrc6dscp(dns_zone_t *zone);
/*%/
 * Get the DSCP value associated with the IPv6 parental source.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setparentalsrc6dscp(dns_zone_t *zone, isc_dscp_t dscp);
/*%<
 * Set the DSCP value associated with the IPv6 parental source.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_result_t
dns_zone_setnotifysrc4(dns_zone_t *zone, const isc_sockaddr_t *notifysrc);
/*%<
 * 	Set the source address to be used with IPv4 NOTIFY messages.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'notifysrc' to contain the address.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_sockaddr_t *
dns_zone_getnotifysrc4(dns_zone_t *zone);
/*%<
 *	Returns the source address set by a previous dns_zone_setnotifysrc4
 *	call, or the default of inaddr_any, port 0.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_dscp_t
dns_zone_getnotifysrc4dscp(dns_zone_t *zone);
/*%/
 * Get the DSCP value associated with the IPv4 notify source.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setnotifysrc4dscp(dns_zone_t *zone, isc_dscp_t dscp);
/*%<
 * Set the DSCP value associated with the IPv4 notify source.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_result_t
dns_zone_setnotifysrc6(dns_zone_t *zone, const isc_sockaddr_t *notifysrc);
/*%<
 * 	Set the source address to be used with IPv6 NOTIFY messages.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'notifysrc' to contain the address.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_sockaddr_t *
dns_zone_getnotifysrc6(dns_zone_t *zone);
/*%<
 *	Returns the source address set by a previous dns_zone_setnotifysrc6
 *	call, or the default of in6addr_any, port 0.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_dscp_t
dns_zone_getnotifysrc6dscp(dns_zone_t *zone);
/*%/
 * Get the DSCP value associated with the IPv6 notify source.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setnotifysrc6dscp(dns_zone_t *zone, isc_dscp_t dscp);
/*%<
 * Set the DSCP value associated with the IPv6 notify source.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

void
dns_zone_setnotifyacl(dns_zone_t *zone, dns_acl_t *acl);
/*%<
 *	Sets the notify acl list for the zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'acl' to be a valid acl.
 */

void
dns_zone_setqueryacl(dns_zone_t *zone, dns_acl_t *acl);
/*%<
 *	Sets the query acl list for the zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'acl' to be a valid acl.
 */

void
dns_zone_setqueryonacl(dns_zone_t *zone, dns_acl_t *acl);
/*%<
 *	Sets the query-on acl list for the zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'acl' to be a valid acl.
 */

void
dns_zone_setupdateacl(dns_zone_t *zone, dns_acl_t *acl);
/*%<
 *	Sets the update acl list for the zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'acl' to be valid acl.
 */

void
dns_zone_setforwardacl(dns_zone_t *zone, dns_acl_t *acl);
/*%<
 *	Sets the forward unsigned updates acl list for the zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'acl' to be valid acl.
 */

void
dns_zone_setxfracl(dns_zone_t *zone, dns_acl_t *acl);
/*%<
 *	Sets the transfer acl list for the zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'acl' to be valid acl.
 */

dns_acl_t *
dns_zone_getnotifyacl(dns_zone_t *zone);
/*%<
 * 	Returns the current notify acl or NULL.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	acl a pointer to the acl.
 *\li	NULL
 */

dns_acl_t *
dns_zone_getqueryacl(dns_zone_t *zone);
/*%<
 * 	Returns the current query acl or NULL.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	acl a pointer to the acl.
 *\li	NULL
 */

dns_acl_t *
dns_zone_getqueryonacl(dns_zone_t *zone);
/*%<
 * 	Returns the current query-on acl or NULL.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	acl a pointer to the acl.
 *\li	NULL
 */

dns_acl_t *
dns_zone_getupdateacl(dns_zone_t *zone);
/*%<
 * 	Returns the current update acl or NULL.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	acl a pointer to the acl.
 *\li	NULL
 */

dns_acl_t *
dns_zone_getforwardacl(dns_zone_t *zone);
/*%<
 * 	Returns the current forward unsigned updates acl or NULL.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	acl a pointer to the acl.
 *\li	NULL
 */

dns_acl_t *
dns_zone_getxfracl(dns_zone_t *zone);
/*%<
 * 	Returns the current transfer acl or NULL.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	acl a pointer to the acl.
 *\li	NULL
 */

void
dns_zone_clearupdateacl(dns_zone_t *zone);
/*%<
 *	Clear the current update acl.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_clearforwardacl(dns_zone_t *zone);
/*%<
 *	Clear the current forward unsigned updates acl.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_clearnotifyacl(dns_zone_t *zone);
/*%<
 *	Clear the current notify acl.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_clearqueryacl(dns_zone_t *zone);
/*%<
 *	Clear the current query acl.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_clearqueryonacl(dns_zone_t *zone);
/*%<
 *	Clear the current query-on acl.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_clearxfracl(dns_zone_t *zone);
/*%<
 *	Clear the current transfer acl.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

bool
dns_zone_getupdatedisabled(dns_zone_t *zone);
/*%<
 * Return update disabled.
 * Transient unless called when running in isc_task_exclusive() mode.
 */

void
dns_zone_setupdatedisabled(dns_zone_t *zone, bool state);
/*%<
 * Set update disabled.
 * Should only be called only when running in isc_task_exclusive() mode.
 * Failure to do so may result in updates being committed after the
 * call has been made.
 */

bool
dns_zone_getzeronosoattl(dns_zone_t *zone);
/*%<
 * Return zero-no-soa-ttl status.
 */

void
dns_zone_setzeronosoattl(dns_zone_t *zone, bool state);
/*%<
 * Set zero-no-soa-ttl status.
 */

void
dns_zone_setchecknames(dns_zone_t *zone, dns_severity_t severity);
/*%<
 * 	Set the severity of name checking when loading a zone.
 *
 * Require:
 * \li     'zone' to be a valid zone.
 */

dns_severity_t
dns_zone_getchecknames(dns_zone_t *zone);
/*%<
 *	Return the current severity of name checking.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_setjournalsize(dns_zone_t *zone, int32_t size);
/*%<
 *	Sets the journal size for the zone.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 */

int32_t
dns_zone_getjournalsize(dns_zone_t *zone);
/*%<
 *	Return the journal size as set with a previous call to
 *	dns_zone_setjournalsize().
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_notifyreceive(dns_zone_t *zone, isc_sockaddr_t *from,
		       isc_sockaddr_t *to, dns_message_t *msg);
/*%<
 *	Tell the zone that it has received a NOTIFY message from another
 *	server.  This may cause some zone maintenance activity to occur.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 *\li	'*from' to contain the address of the server from which 'msg'
 *		was received.
 *\li	'msg' a message with opcode NOTIFY and qr clear.
 *
 * Returns:
 *\li	DNS_R_REFUSED
 *\li	DNS_R_NOTIMP
 *\li	DNS_R_FORMERR
 *\li	DNS_R_SUCCESS
 */

void
dns_zone_setmaxxfrin(dns_zone_t *zone, uint32_t maxxfrin);
/*%<
 * Set the maximum time (in seconds) that a zone transfer in (AXFR/IXFR)
 * of this zone will use before being aborted.
 *
 * Requires:
 * \li	'zone' to be valid initialised zone.
 */

uint32_t
dns_zone_getmaxxfrin(dns_zone_t *zone);
/*%<
 * Returns the maximum transfer time for this zone.  This will be
 * either the value set by the last call to dns_zone_setmaxxfrin() or
 * the default value of 1 hour.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 */

void
dns_zone_setmaxxfrout(dns_zone_t *zone, uint32_t maxxfrout);
/*%<
 * Set the maximum time (in seconds) that a zone transfer out (AXFR/IXFR)
 * of this zone will use before being aborted.
 *
 * Requires:
 * \li	'zone' to be valid initialised zone.
 */

uint32_t
dns_zone_getmaxxfrout(dns_zone_t *zone);
/*%<
 * Returns the maximum transfer time for this zone.  This will be
 * either the value set by the last call to dns_zone_setmaxxfrout() or
 * the default value of 1 hour.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 */

isc_result_t
dns_zone_setjournal(dns_zone_t *zone, const char *myjournal);
/*%<
 * Sets the filename used for journaling updates / IXFR transfers.
 * The default journal name is set by dns_zone_setfile() to be
 * "file.jnl".  If 'myjournal' is NULL, the zone will have no
 * journal name.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 */

char *
dns_zone_getjournal(dns_zone_t *zone);
/*%<
 * Returns the journal name associated with this zone.
 * If no journal has been set this will be NULL.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 */

dns_zonetype_t
dns_zone_gettype(dns_zone_t *zone);
/*%<
 * Returns the type of the zone (master/slave/etc.)
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 */

dns_zonetype_t
dns_zone_getredirecttype(dns_zone_t *zone);
/*%<
 * Returns whether the redirect zone is configured as a master or a
 * slave zone.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 *\li	'zone' to be a redirect zone.
 *
 * Returns:
 *\li	'dns_zone_primary'
 *\li	'dns_zone_secondary'
 */

void
dns_zone_settask(dns_zone_t *zone, isc_task_t *task);
/*%<
 * Give a zone a task to work with.  Any current task will be detached.
 *
 * Requires:
 *\li	'zone' to be valid.
 *\li	'task' to be valid.
 */

void
dns_zone_gettask(dns_zone_t *zone, isc_task_t **target);
/*%<
 * Attach '*target' to the zone's task.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 *\li	'zone' to have a task.
 *\li	'target' to be != NULL && '*target' == NULL.
 */

void
dns_zone_notify(dns_zone_t *zone);
/*%<
 * Generate notify events for this zone.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_replacedb(dns_zone_t *zone, dns_db_t *db, bool dump);
/*%<
 * Replace the database of "zone" with a new database "db".
 *
 * If "dump" is true, then the new zone contents are dumped
 * into to the zone's master file for persistence.  When replacing
 * a zone database by one just loaded from a master file, set
 * "dump" to false to avoid a redundant redump of the data just
 * loaded.  Otherwise, it should be set to true.
 *
 * If the "diff-on-reload" option is enabled in the configuration file,
 * the differences between the old and the new database are added to the
 * journal file, and the master file dump is postponed.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 *
 * Returns:
 * \li	DNS_R_SUCCESS
 * \li	DNS_R_BADZONE	zone failed basic consistency checks:
 *			* a single SOA must exist
 *			* some NS records must exist.
 *	Others
 */

uint32_t
dns_zone_getidlein(dns_zone_t *zone);
/*%<
 * Requires:
 * \li	'zone' to be a valid zone.
 *
 * Returns:
 * \li	number of seconds of idle time before we abort the transfer in.
 */

void
dns_zone_setidlein(dns_zone_t *zone, uint32_t idlein);
/*%<
 * \li	Set the idle timeout for transfer the.
 * \li	Zero set the default value, 1 hour.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

uint32_t
dns_zone_getidleout(dns_zone_t *zone);
/*%<
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 *
 * Returns:
 * \li	number of seconds of idle time before we abort a transfer out.
 */

void
dns_zone_setidleout(dns_zone_t *zone, uint32_t idleout);
/*%<
 * \li	Set the idle timeout for transfers out.
 * \li	Zero set the default value, 1 hour.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

void
dns_zone_getssutable(dns_zone_t *zone, dns_ssutable_t **table);
/*%<
 * Get the simple-secure-update policy table.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

void
dns_zone_setssutable(dns_zone_t *zone, dns_ssutable_t *table);
/*%<
 * Set / clear the simple-secure-update policy table.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

isc_mem_t *
dns_zone_getmctx(dns_zone_t *zone);
/*%<
 * Get the memory context of a zone.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

dns_zonemgr_t *
dns_zone_getmgr(dns_zone_t *zone);
/*%<
 *	If 'zone' is managed return the zone manager otherwise NULL.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

void
dns_zone_setsigvalidityinterval(dns_zone_t *zone, uint32_t interval);
/*%<
 * Set the zone's general signature validity interval.  This is the length
 * of time for which DNSSEC signatures created as a result of dynamic
 * updates to secure zones will remain valid, in seconds.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

uint32_t
dns_zone_getsigvalidityinterval(dns_zone_t *zone);
/*%<
 * Get the zone's general signature validity interval.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

void
dns_zone_setkeyvalidityinterval(dns_zone_t *zone, uint32_t interval);
/*%<
 * Set the zone's DNSKEY signature validity interval.  This is the length
 * of time for which DNSSEC signatures created for DNSKEY records
 * will remain valid, in seconds.
 *
 * If this value is set to zero, then the regular signature validity
 * interval (see dns_zone_setsigvalidityinterval(), above) is used
 * for all RRSIGs. However, if this value is nonzero, then it is used
 * as the validity interval for RRSIGs covering DNSKEY and CDNSKEY
 * RRsets.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

uint32_t
dns_zone_getkeyvalidityinterval(dns_zone_t *zone);
/*%<
 * Get the zone's DNSKEY signature validity interval.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

void
dns_zone_setsigresigninginterval(dns_zone_t *zone, uint32_t interval);
/*%<
 * Set the zone's RRSIG re-signing interval.  A dynamic zone's RRSIG's
 * will be re-signed 'interval' amount of time before they expire.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

uint32_t
dns_zone_getsigresigninginterval(dns_zone_t *zone);
/*%<
 * Get the zone's RRSIG re-signing interval.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

void
dns_zone_setnotifytype(dns_zone_t *zone, dns_notifytype_t notifytype);
/*%<
 * Sets zone notify method to "notifytype"
 */

isc_result_t
dns_zone_forwardupdate(dns_zone_t *zone, dns_message_t *msg,
		       dns_updatecallback_t callback, void *callback_arg);
/*%<
 * Forward 'msg' to each master in turn until we get an answer or we
 * have exhausted the list of primaries. 'callback' will be called with
 * ISC_R_SUCCESS if we get an answer and the returned message will be
 * passed as 'answer_message', otherwise a non ISC_R_SUCCESS result code
 * will be passed and answer_message will be NULL.  The callback function
 * is responsible for destroying 'answer_message'.
 *		(callback)(callback_arg, result, answer_message);
 *
 * Require:
 *\li	'zone' to be valid
 *\li	'msg' to be valid.
 *\li	'callback' to be non NULL.
 * Returns:
 *\li	#ISC_R_SUCCESS if the message has been forwarded,
 *\li	#ISC_R_NOMEMORY
 *\li	Others
 */

isc_result_t
dns_zone_next(dns_zone_t *zone, dns_zone_t **next);
/*%<
 * Find the next zone in the list of managed zones.
 *
 * Requires:
 *\li	'zone' to be valid
 *\li	The zone manager for the indicated zone MUST be locked
 *	by the caller.  This is not checked.
 *\li	'next' be non-NULL, and '*next' be NULL.
 *
 * Ensures:
 *\li	'next' points to a valid zone (result ISC_R_SUCCESS) or to NULL
 *	(result ISC_R_NOMORE).
 */

isc_result_t
dns_zone_first(dns_zonemgr_t *zmgr, dns_zone_t **first);
/*%<
 * Find the first zone in the list of managed zones.
 *
 * Requires:
 *\li	'zonemgr' to be valid
 *\li	The zone manager for the indicated zone MUST be locked
 *	by the caller.  This is not checked.
 *\li	'first' be non-NULL, and '*first' be NULL
 *
 * Ensures:
 *\li	'first' points to a valid zone (result ISC_R_SUCCESS) or to NULL
 *	(result ISC_R_NOMORE).
 */

isc_result_t
dns_zone_setkeydirectory(dns_zone_t *zone, const char *directory);
/*%<
 *	Sets the name of the directory where private keys used for
 *	online signing of dynamic zones are found.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_SUCCESS
 */

const char *
dns_zone_getkeydirectory(dns_zone_t *zone);
/*%<
 * 	Gets the name of the directory where private keys used for
 *	online signing of dynamic zones are found.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 *
 * Returns:
 *	Pointer to null-terminated file name, or NULL.
 */

isc_result_t
dns_zone_getdnsseckeys(dns_zone_t *zone, dns_db_t *db, dns_dbversion_t *ver,
		       isc_stdtime_t now, dns_dnsseckeylist_t *keys);
/*%
 * Find DNSSEC keys used for signing with dnssec-policy. Load these keys
 * into 'keys'.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 *\li	'keys' to be an initialised DNSSEC keylist.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	Error
 */

isc_result_t
dns_zonemgr_create(isc_mem_t *mctx, isc_taskmgr_t *taskmgr,
		   isc_timermgr_t *timermgr, isc_socketmgr_t *socketmgr,
		   dns_zonemgr_t **zmgrp);
/*%<
 * Create a zone manager.  Note: the zone manager will not be able to
 * manage any zones until dns_zonemgr_setsize() has been run.
 *
 * Requires:
 *\li	'mctx' to be a valid memory context.
 *\li	'taskmgr' to be a valid task manager.
 *\li	'timermgr' to be a valid timer manager.
 *\li	'zmgrp'	to point to a NULL pointer.
 */

isc_result_t
dns_zonemgr_setsize(dns_zonemgr_t *zmgr, int num_zones);
/*%<
 *	Set the size of the zone manager task pool.  This must be run
 *	before zmgr can be used for managing zones.  Currently, it can only
 *	be run once; the task pool cannot be resized.
 *
 * Requires:
 *\li	zmgr is a valid zone manager.
 *\li	zmgr->zonetasks has been initialized.
 */

isc_result_t
dns_zonemgr_createzone(dns_zonemgr_t *zmgr, dns_zone_t **zonep);
/*%<
 *	Allocate a new zone using a memory context from the
 *	zone manager's memory context pool.
 *
 * Require:
 *\li	'zmgr' to be a valid zone manager.
 *\li	'zonep' != NULL and '*zonep' == NULL.
 */

isc_result_t
dns_zonemgr_managezone(dns_zonemgr_t *zmgr, dns_zone_t *zone);
/*%<
 *	Bring the zone under control of a zone manager.
 *
 * Require:
 *\li	'zmgr' to be a valid zone manager.
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zonemgr_forcemaint(dns_zonemgr_t *zmgr);
/*%<
 * Force zone maintenance of all loaded zones managed by 'zmgr'
 * to take place at the system's earliest convenience.
 */

void
dns__zonemgr_run(isc_task_t *task, isc_event_t *event);
/*%<
 * Event handler to call dns_zonemgr_forcemaint(); used to start
 * zone operations from a unit test.  Not intended for use outside
 * libdns or related tests.
 */

void
dns_zonemgr_resumexfrs(dns_zonemgr_t *zmgr);
/*%<
 * Attempt to start any stalled zone transfers.
 */

void
dns_zonemgr_shutdown(dns_zonemgr_t *zmgr);
/*%<
 *	Shut down the zone manager.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 */

void
dns_zonemgr_attach(dns_zonemgr_t *source, dns_zonemgr_t **target);
/*%<
 *	Attach '*target' to 'source' incrementing its external
 * 	reference count.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'target' to be non NULL and '*target' to be NULL.
 */

void
dns_zonemgr_detach(dns_zonemgr_t **zmgrp);
/*%<
 *	 Detach from a zone manager.
 *
 * Requires:
 *\li	'*zmgrp' is a valid, non-NULL zone manager pointer.
 *
 * Ensures:
 *\li	'*zmgrp' is NULL.
 */

void
dns_zonemgr_releasezone(dns_zonemgr_t *zmgr, dns_zone_t *zone);
/*%<
 *	Release 'zone' from the managed by 'zmgr'.  'zmgr' is implicitly
 *	detached from 'zone'.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 *\li	'zone' to be a valid zone.
 *\li	'zmgr' == 'zone->zmgr'
 *
 * Ensures:
 *\li	'zone->zmgr' == NULL;
 */

isc_taskmgr_t *
dns_zonemgr_gettaskmgr(dns_zonemgr_t *zmgr);
/*%
 * Get the tasmkgr object attached to 'zmgr'.
 */

void
dns_zonemgr_settransfersin(dns_zonemgr_t *zmgr, uint32_t value);
/*%<
 *	Set the maximum number of simultaneous transfers in allowed by
 *	the zone manager.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 */

uint32_t
dns_zonemgr_getttransfersin(dns_zonemgr_t *zmgr);
/*%<
 *	Return the maximum number of simultaneous transfers in allowed.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 */

void
dns_zonemgr_settransfersperns(dns_zonemgr_t *zmgr, uint32_t value);
/*%<
 *	Set the number of zone transfers allowed per nameserver.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager
 */

uint32_t
dns_zonemgr_getttransfersperns(dns_zonemgr_t *zmgr);
/*%<
 *	Return the number of transfers allowed per nameserver.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 */

void
dns_zonemgr_setiolimit(dns_zonemgr_t *zmgr, uint32_t iolimit);
/*%<
 *	Set the number of simultaneous file descriptors available for
 *	reading and writing masterfiles.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 *\li	'iolimit' to be positive.
 */

uint32_t
dns_zonemgr_getiolimit(dns_zonemgr_t *zmgr);
/*%<
 *	Get the number of simultaneous file descriptors available for
 *	reading and writing masterfiles.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 */

void
dns_zonemgr_setcheckdsrate(dns_zonemgr_t *zmgr, unsigned int value);
/*%<
 *	Set the number of parental DS queries sent per second.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager
 */

void
dns_zonemgr_setnotifyrate(dns_zonemgr_t *zmgr, unsigned int value);
/*%<
 *	Set the number of NOTIFY requests sent per second.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager
 */

void
dns_zonemgr_setstartupnotifyrate(dns_zonemgr_t *zmgr, unsigned int value);
/*%<
 *	Set the number of startup NOTIFY requests sent per second.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager
 */

void
dns_zonemgr_setserialqueryrate(dns_zonemgr_t *zmgr, unsigned int value);
/*%<
 *	Set the number of SOA queries sent per second.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager
 */

unsigned int
dns_zonemgr_getnotifyrate(dns_zonemgr_t *zmgr);
/*%<
 *	Return the number of NOTIFY requests sent per second.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 */

unsigned int
dns_zonemgr_getstartupnotifyrate(dns_zonemgr_t *zmgr);
/*%<
 *	Return the number of startup NOTIFY requests sent per second.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 */

unsigned int
dns_zonemgr_getserialqueryrate(dns_zonemgr_t *zmgr);
/*%<
 *	Return the number of SOA queries sent per second.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 */

unsigned int
dns_zonemgr_getcount(dns_zonemgr_t *zmgr, int state);
/*%<
 *	Returns the number of zones in the specified state.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 *\li	'state' to be a valid DNS_ZONESTATE_ constant.
 */

void
dns_zonemgr_unreachableadd(dns_zonemgr_t *zmgr, isc_sockaddr_t *remote,
			   isc_sockaddr_t *local, isc_time_t *now);
/*%<
 *	Add the pair of addresses to the unreachable cache.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 *\li	'remote' to be a valid sockaddr.
 *\li	'local' to be a valid sockaddr.
 */

bool
dns_zonemgr_unreachable(dns_zonemgr_t *zmgr, isc_sockaddr_t *remote,
			isc_sockaddr_t *local, isc_time_t *now);
/*%<
 *	Returns true if the given local/remote address pair
 *	is found in the zone maanger's unreachable cache.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 *\li	'remote' to be a valid sockaddr.
 *\li	'local' to be a valid sockaddr.
 *\li	'now' != NULL
 */

void
dns_zonemgr_unreachabledel(dns_zonemgr_t *zmgr, isc_sockaddr_t *remote,
			   isc_sockaddr_t *local);
/*%<
 *	Remove the pair of addresses from the unreachable cache.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 *\li	'remote' to be a valid sockaddr.
 *\li	'local' to be a valid sockaddr.
 */

void
dns_zone_forcereload(dns_zone_t *zone);
/*%<
 *      Force a reload of specified zone.
 *
 * Requires:
 *\li      'zone' to be a valid zone.
 */

bool
dns_zone_isforced(dns_zone_t *zone);
/*%<
 *      Check if the zone is waiting a forced reload.
 *
 * Requires:
 * \li     'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setstatistics(dns_zone_t *zone, bool on);
/*%<
 * This function is obsoleted by dns_zone_setrequeststats().
 */

uint64_t *
dns_zone_getstatscounters(dns_zone_t *zone);
/*%<
 * This function is obsoleted by dns_zone_getrequeststats().
 */

void
dns_zone_setstats(dns_zone_t *zone, isc_stats_t *stats);
/*%<
 * Set a general zone-maintenance statistics set 'stats' for 'zone'.  This
 * function is expected to be called only on zone creation (when necessary).
 * Once installed, it cannot be removed or replaced.  Also, there is no
 * interface to get the installed stats from the zone; the caller must keep the
 * stats to reference (e.g. dump) it later.
 *
 * Requires:
 * \li	'zone' to be a valid zone and does not have a statistics set already
 *	installed.
 *
 *\li	stats is a valid statistics supporting zone statistics counters
 *	(see dns/stats.h).
 */

void
dns_zone_setrequeststats(dns_zone_t *zone, isc_stats_t *stats);

void
dns_zone_setrcvquerystats(dns_zone_t *zone, dns_stats_t *stats);

void
dns_zone_setdnssecsignstats(dns_zone_t *zone, dns_stats_t *stats);
/*%<
 * Set additional statistics sets to zone.  These are attached to the zone
 * but are not counted in the zone module; only the caller updates the
 * counters.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 *
 *\li	stats is a valid statistics.
 */

isc_stats_t *
dns_zone_getrequeststats(dns_zone_t *zone);

dns_stats_t *
dns_zone_getrcvquerystats(dns_zone_t *zone);

dns_stats_t *
dns_zone_getdnssecsignstats(dns_zone_t *zone);
/*%<
 * Get the additional statistics for zone, if one is installed.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 *
 * Returns:
 * \li	when available, a pointer to the statistics set installed in zone;
 *	otherwise NULL.
 */

void
dns_zone_dialup(dns_zone_t *zone);
/*%<
 * Perform dialup-time maintenance on 'zone'.
 */

void
dns_zone_setdialup(dns_zone_t *zone, dns_dialuptype_t dialup);
/*%<
 * Set the dialup type of 'zone' to 'dialup'.
 *
 * Requires:
 * \li	'zone' to be valid initialised zone.
 *\li	'dialup' to be a valid dialup type.
 */

void
dns_zone_logv(dns_zone_t *zone, isc_logcategory_t *category, int level,
	      const char *prefix, const char *msg, va_list ap);
/*%<
 * Log the message 'msg...' at 'level' using log category 'category', including
 * text that identifies the message as applying to 'zone'.  If the (optional)
 * 'prefix' is not NULL, it will be placed at the start of the entire log line.
 */

void
dns_zone_log(dns_zone_t *zone, int level, const char *msg, ...)
	ISC_FORMAT_PRINTF(3, 4);
/*%<
 * Log the message 'msg...' at 'level', including text that identifies
 * the message as applying to 'zone'.
 */

void
dns_zone_logc(dns_zone_t *zone, isc_logcategory_t *category, int level,
	      const char *msg, ...) ISC_FORMAT_PRINTF(4, 5);
/*%<
 * Log the message 'msg...' at 'level', including text that identifies
 * the message as applying to 'zone'.
 */

void
dns_zone_name(dns_zone_t *zone, char *buf, size_t len);
/*%<
 * Return the name of the zone with class and view.
 *
 * Requires:
 *\li	'zone' to be valid.
 *\li	'buf' to be non NULL.
 */

void
dns_zone_nameonly(dns_zone_t *zone, char *buf, size_t len);
/*%<
 * Return the name of the zone only.
 *
 * Requires:
 *\li	'zone' to be valid.
 *\li	'buf' to be non NULL.
 */

isc_result_t
dns_zone_checknames(dns_zone_t *zone, const dns_name_t *name,
		    dns_rdata_t *rdata);
/*%<
 * Check if this record meets the check-names policy.
 *
 * Requires:
 *	'zone' to be valid.
 *	'name' to be valid.
 *	'rdata' to be valid.
 *
 * Returns:
 *	DNS_R_SUCCESS		passed checks.
 *	DNS_R_BADOWNERNAME	failed ownername checks.
 *	DNS_R_BADNAME		failed rdata checks.
 */

void
dns_zone_setcheckmx(dns_zone_t *zone, dns_checkmxfunc_t checkmx);
/*%<
 *	Set the post load integrity callback function 'checkmx'.
 *	'checkmx' will be called if the MX TARGET is not within the zone.
 *
 * Require:
 *	'zone' to be a valid zone.
 */

void
dns_zone_setchecksrv(dns_zone_t *zone, dns_checkmxfunc_t checksrv);
/*%<
 *	Set the post load integrity callback function 'checksrv'.
 *	'checksrv' will be called if the SRV TARGET is not within the zone.
 *
 * Require:
 *	'zone' to be a valid zone.
 */

void
dns_zone_setcheckns(dns_zone_t *zone, dns_checknsfunc_t checkns);
/*%<
 *	Set the post load integrity callback function 'checkns'.
 *	'checkns' will be called if the NS TARGET is not within the zone.
 *
 * Require:
 *	'zone' to be a valid zone.
 */

void
dns_zone_setnotifydelay(dns_zone_t *zone, uint32_t delay);
/*%<
 * Set the minimum delay between sets of notify messages.
 *
 * Requires:
 *	'zone' to be valid.
 */

uint32_t
dns_zone_getnotifydelay(dns_zone_t *zone);
/*%<
 * Get the minimum delay between sets of notify messages.
 *
 * Requires:
 *	'zone' to be valid.
 */

void
dns_zone_setisself(dns_zone_t *zone, dns_isselffunc_t isself, void *arg);
/*%<
 * Set the isself callback function and argument.
 *
 * bool
 * isself(dns_view_t *myview, dns_tsigkey_t *mykey,
 *	  const isc_netaddr_t *srcaddr, const isc_netaddr_t *destaddr,
 *	  dns_rdataclass_t rdclass, void *arg);
 *
 * 'isself' returns true if a non-recursive query from 'srcaddr' to
 * 'destaddr' with optional key 'mykey' for class 'rdclass' would be
 * delivered to 'myview'.
 */

void
dns_zone_setnodes(dns_zone_t *zone, uint32_t nodes);
/*%<
 * Set the number of nodes that will be checked per quantum.
 */

void
dns_zone_setsignatures(dns_zone_t *zone, uint32_t signatures);
/*%<
 * Set the number of signatures that will be generated per quantum.
 */

uint32_t
dns_zone_getsignatures(dns_zone_t *zone);
/*%<
 * Get the number of signatures that will be generated per quantum.
 */

isc_result_t
dns_zone_signwithkey(dns_zone_t *zone, dns_secalg_t algorithm, uint16_t keyid,
		     bool deleteit);
/*%<
 * Initiate/resume signing of the entire zone with the zone DNSKEY(s)
 * that match the given algorithm and keyid.
 */

isc_result_t
dns_zone_addnsec3chain(dns_zone_t *zone, dns_rdata_nsec3param_t *nsec3param);
/*%<
 * Incrementally add a NSEC3 chain that corresponds to 'nsec3param'.
 */

void
dns_zone_setprivatetype(dns_zone_t *zone, dns_rdatatype_t type);
dns_rdatatype_t
dns_zone_getprivatetype(dns_zone_t *zone);
/*
 * Get/Set the private record type.  It is expected that these interfaces
 * will not be permanent.
 */

void
dns_zone_rekey(dns_zone_t *zone, bool fullsign);
/*%<
 * Update the zone's DNSKEY set from the key repository.
 *
 * If 'fullsign' is true, trigger an immediate full signing of
 * the zone with the new key.  Otherwise, if there are no keys or
 * if the new keys are for algorithms that have already signed the
 * zone, then the zone can be re-signed incrementally.
 */

isc_result_t
dns_zone_nscheck(dns_zone_t *zone, dns_db_t *db, dns_dbversion_t *version,
		 unsigned int *errors);
/*%
 * Check if the name servers for the zone are sane (have address, don't
 * refer to CNAMEs/DNAMEs.  The number of constiancy errors detected in
 * returned in '*errors'
 *
 * Requires:
 * \li	'zone' to be valid.
 * \li	'db' to be valid.
 * \li	'version' to be valid or NULL.
 * \li	'errors' to be non NULL.
 *
 * Returns:
 * 	ISC_R_SUCCESS if there were no errors examining the zone contents.
 */

isc_result_t
dns_zone_cdscheck(dns_zone_t *zone, dns_db_t *db, dns_dbversion_t *version);
/*%
 * Check if CSD, CDNSKEY and DNSKEY are consistent.
 *
 * Requires:
 * \li	'zone' to be valid.
 * \li	'db' to be valid.
 * \li	'version' to be valid or NULL.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#DNS_R_BADCDS
 *\li	#DNS_R_BADCDNSKEY
 *	Others
 */

void
dns_zone_setadded(dns_zone_t *zone, bool added);
/*%
 * Sets the value of zone->added, which should be true for
 * zones that were originally added by "rndc addzone".
 *
 * Requires:
 * \li	'zone' to be valid.
 */

bool
dns_zone_getadded(dns_zone_t *zone);
/*%
 * Returns true if the zone was originally added at runtime
 * using "rndc addzone".
 *
 * Requires:
 * \li	'zone' to be valid.
 */

void
dns_zone_setautomatic(dns_zone_t *zone, bool automatic);
/*%
 * Sets the value of zone->automatic, which should be true for
 * zones that were automatically added by named.
 *
 * Requires:
 * \li	'zone' to be valid.
 */

bool
dns_zone_getautomatic(dns_zone_t *zone);
/*%
 * Returns true if the zone was added automatically by named.
 *
 * Requires:
 * \li	'zone' to be valid.
 */

isc_result_t
dns_zone_dlzpostload(dns_zone_t *zone, dns_db_t *db);
/*%
 * Load the origin names for a writeable DLZ database.
 */

bool
dns_zone_isdynamic(dns_zone_t *zone, bool ignore_freeze);
/*%
 * Return true iff the zone is "dynamic", in the sense that the zone's
 * master file (if any) is written by the server, rather than being
 * updated manually and read by the server.
 *
 * This is true for slave zones, stub zones, key zones, and zones that
 * allow dynamic updates either by having an update policy ("ssutable")
 * or an "allow-update" ACL with a value other than exactly "{ none; }".
 *
 * If 'ignore_freeze' is true, then the zone which has had updates disabled
 * will still report itself to be dynamic.
 *
 * Requires:
 * \li	'zone' to be valid.
 */

isc_result_t
dns_zone_setrefreshkeyinterval(dns_zone_t *zone, uint32_t interval);
/*%
 * Sets the frequency, in minutes, with which the key repository will be
 * checked to see if the keys for this zone have been updated.  Any value
 * higher than 1440 minutes (24 hours) will be silently reduced.  A
 * value of zero will return an out-of-range error.
 *
 * Requires:
 * \li	'zone' to be valid.
 */

bool
dns_zone_getrequestexpire(dns_zone_t *zone);
/*%
 * Returns the true/false value of the request-expire option in the zone.
 *
 * Requires:
 * \li	'zone' to be valid.
 */

void
dns_zone_setrequestexpire(dns_zone_t *zone, bool flag);
/*%
 * Sets the request-expire option for the zone. Either true or false. The
 * default value is determined by the setting of this option in the view.
 *
 * Requires:
 * \li	'zone' to be valid.
 */

bool
dns_zone_getrequestixfr(dns_zone_t *zone);
/*%
 * Returns the true/false value of the request-ixfr option in the zone.
 *
 * Requires:
 * \li	'zone' to be valid.
 */

void
dns_zone_setrequestixfr(dns_zone_t *zone, bool flag);
/*%
 * Sets the request-ixfr option for the zone. Either true or false. The
 * default value is determined by the setting of this option in the view.
 *
 * Requires:
 * \li	'zone' to be valid.
 */

uint32_t
dns_zone_getixfrratio(dns_zone_t *zone);
/*%
 * Returns the zone's current IXFR ratio.
 *
 * Requires:
 * \li	'zone' to be valid.
 */

void
dns_zone_setixfrratio(dns_zone_t *zone, uint32_t ratio);
/*%
 * Sets the ratio of IXFR size to zone size above which we use an AXFR
 * response, expressed as a percentage. Cannot exceed 100.
 *
 * Requires:
 * \li	'zone' to be valid.
 */

void
dns_zone_setserialupdatemethod(dns_zone_t *zone, dns_updatemethod_t method);
/*%
 * Sets the update method to use when incrementing the zone serial number
 * due to a DDNS update.  Valid options are dns_updatemethod_increment
 * and dns_updatemethod_unixtime.
 *
 * Requires:
 * \li	'zone' to be valid.
 */

dns_updatemethod_t
dns_zone_getserialupdatemethod(dns_zone_t *zone);
/*%
 * Returns the update method to be used when incrementing the zone serial
 * number due to a DDNS update.
 *
 * Requires:
 * \li	'zone' to be valid.
 */

isc_result_t
dns_zone_link(dns_zone_t *zone, dns_zone_t *raw);

void
dns_zone_getraw(dns_zone_t *zone, dns_zone_t **raw);

isc_result_t
dns_zone_keydone(dns_zone_t *zone, const char *data);

isc_result_t
dns_zone_setnsec3param(dns_zone_t *zone, uint8_t hash, uint8_t flags,
		       uint16_t iter, uint8_t saltlen, unsigned char *salt,
		       bool replace, bool resalt);
/*%
 * Set the NSEC3 parameters for the zone.
 *
 * If 'replace' is true, then the existing NSEC3 chain, if any, will
 * be replaced with the new one.  If 'hash' is zero, then the replacement
 * chain will be NSEC rather than NSEC3. If 'resalt' is true, or if 'salt'
 * is NULL, generate a new salt with the given salt length.
 *
 * Requires:
 * \li	'zone' to be valid.
 */

void
dns_zone_setrawdata(dns_zone_t *zone, dns_masterrawheader_t *header);
/*%
 * Set the data to be included in the header when the zone is dumped in
 * binary format.
 */

isc_result_t
dns_zone_synckeyzone(dns_zone_t *zone);
/*%
 * Force the managed key zone to synchronize, and start the key
 * maintenance timer.
 */

isc_result_t
dns_zone_getloadtime(dns_zone_t *zone, isc_time_t *loadtime);
/*%
 * Return the time when the zone was last loaded.
 */

isc_result_t
dns_zone_getrefreshtime(dns_zone_t *zone, isc_time_t *refreshtime);
/*%
 * Return the time when the (slave) zone will need to be refreshed.
 */

isc_result_t
dns_zone_getexpiretime(dns_zone_t *zone, isc_time_t *expiretime);
/*%
 * Return the time when the (slave) zone will expire.
 */

isc_result_t
dns_zone_getrefreshkeytime(dns_zone_t *zone, isc_time_t *refreshkeytime);
/*%
 * Return the time of the next scheduled DNSSEC key event.
 */

unsigned int
dns_zone_getincludes(dns_zone_t *zone, char ***includesp);
/*%
 * Return the number include files that were encountered
 * during load.  If the number is greater than zero, 'includesp'
 * will point to an array containing the filenames.
 *
 * The array and its contents need to be freed using isc_mem_free.
 */

isc_result_t
dns_zone_rpz_enable(dns_zone_t *zone, dns_rpz_zones_t *rpzs,
		    dns_rpz_num_t rpz_num);
/*%
 * Set the response policy associated with a zone.
 */

void
dns_zone_rpz_enable_db(dns_zone_t *zone, dns_db_t *db);
/*%
 * If a zone is a response policy zone, mark its new database.
 */

dns_rpz_num_t
dns_zone_get_rpz_num(dns_zone_t *zone);

void
dns_zone_catz_enable(dns_zone_t *zone, dns_catz_zones_t *catzs);
/*%<
 * Enable zone as catalog zone.
 *
 * Requires:
 *
 * \li	'zone' is a valid zone object
 * \li	'catzs' is not NULL
 * \li	prior to calling, zone->catzs is NULL or is equal to 'catzs'
 */

void
dns_zone_catz_disable(dns_zone_t *zone);
/*%<
 * Disable zone as catalog zone, if it is one.  Also disables any
 * registered callbacks for the catalog zone.
 *
 * Requires:
 *
 * \li	'zone' is a valid zone object
 */

bool
dns_zone_catz_is_enabled(dns_zone_t *zone);
/*%<
 * Return a boolean indicating whether the zone is enabled as catalog zone.
 *
 * Requires:
 *
 * \li	'zone' is a valid zone object
 */

void
dns_zone_catz_enable_db(dns_zone_t *zone, dns_db_t *db);
/*%<
 * If 'zone' is a catalog zone, then set up a notify-on-update trigger
 * in its database. (If not a catalog zone, this function has no effect.)
 *
 * Requires:
 *
 * \li	'zone' is a valid zone object
 * \li	'db' is not NULL
 */
void
dns_zone_set_parentcatz(dns_zone_t *zone, dns_catz_zone_t *catz);
/*%<
 * Set parent catalog zone for this zone
 *
 * Requires:
 *
 * \li	'zone' is a valid zone object
 * \li	'catz' is not NULL
 */

dns_catz_zone_t *
dns_zone_get_parentcatz(const dns_zone_t *zone);
/*%<
 * Get parent catalog zone for this zone
 *
 * Requires:
 *
 * \li	'zone' is a valid zone object
 */

void
dns_zone_setstatlevel(dns_zone_t *zone, dns_zonestat_level_t level);

dns_zonestat_level_t
dns_zone_getstatlevel(dns_zone_t *zone);
/*%
 * Set and get the statistics reporting level for the zone;
 * full, terse, or none.
 */

isc_result_t
dns_zone_setserial(dns_zone_t *zone, uint32_t serial);
/*%
 * Set the zone's serial to 'serial'.
 */
ISC_LANG_ENDDECLS

isc_stats_t *
dns_zone_getgluecachestats(dns_zone_t *zone);
/*%<
 * Get the glue cache statistics for zone.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 *
 * Returns:
 * \li	if present, a pointer to the statistics set installed in zone;
 *	otherwise NULL.
 */

bool
dns_zone_isloaded(dns_zone_t *zone);
/*%<
 * Return true if 'zone' was loaded and has not expired yet, return
 * false otherwise.
 */

isc_result_t
dns_zone_verifydb(dns_zone_t *zone, dns_db_t *db, dns_dbversion_t *ver);
/*%<
 * If 'zone' is a mirror zone, perform DNSSEC validation of version 'ver' of
 * its database, 'db'.  Ensure that the DNSKEY RRset at zone apex is signed by
 * at least one trust anchor specified for the view that 'zone' is assigned to.
 * If 'ver' is NULL, use the current version of 'db'.
 *
 * If 'zone' is not a mirror zone, return ISC_R_SUCCESS immediately.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS		either 'zone' is not a mirror zone or 'zone' is
 *				a mirror zone and all DNSSEC checks succeeded
 *				and the DNSKEY RRset at zone apex is signed by
 *				a trusted key
 *
 * \li	#DNS_R_VERIFYFAILURE	any other case
 */

const char *
dns_zonetype_name(dns_zonetype_t type);
/*%<
 * Return the name of the zone type 'type'.
 */

#endif /* DNS_ZONE_H */
