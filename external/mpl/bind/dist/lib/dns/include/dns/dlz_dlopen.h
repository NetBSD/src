/*	$NetBSD: dlz_dlopen.h,v 1.1.1.1 2018/08/12 12:08:20 christos Exp $	*/

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


/*! \file dns/dlz_dlopen.h */

#ifndef DLZ_DLOPEN_H
#define DLZ_DLOPEN_H

#include <dns/sdlz.h>

ISC_LANG_BEGINDECLS

/*
 * This header provides a minimal set of defines and typedefs needed
 * for the entry points of an external DLZ module for bind9.
 */

#define DLZ_DLOPEN_VERSION 3
#define DLZ_DLOPEN_AGE 0

/*
 * dlz_dlopen_version() is required for all DLZ external drivers. It
 * should return DLZ_DLOPEN_VERSION
 */
typedef int dlz_dlopen_version_t(unsigned int *flags);

/*
 * dlz_dlopen_create() is required for all DLZ external drivers.
 */
typedef isc_result_t dlz_dlopen_create_t(const char *dlzname,
					 unsigned int argc,
					 char *argv[],
					 void **dbdata,
					 ...);

/*
 * dlz_dlopen_destroy() is optional, and will be called when the
 * driver is unloaded if supplied
 */
typedef void dlz_dlopen_destroy_t(void *dbdata);

/*
 * dlz_dlopen_findzonedb() is required for all DLZ external drivers
 */
typedef isc_result_t dlz_dlopen_findzonedb_t(void *dbdata,
					     const char *name,
					     dns_clientinfomethods_t *methods,
					     dns_clientinfo_t *clientinfo);

/*
 * dlz_dlopen_lookup() is required for all DLZ external drivers
 */
typedef isc_result_t dlz_dlopen_lookup_t(const char *zone,
					 const char *name,
					 void *dbdata,
					 dns_sdlzlookup_t *lookup,
					 dns_clientinfomethods_t *methods,
					 dns_clientinfo_t *clientinfo);

/*
 * dlz_dlopen_authority is optional() if dlz_dlopen_lookup()
 * supplies authority information for the dns record
 */
typedef isc_result_t dlz_dlopen_authority_t(const char *zone,
					    void *dbdata,
					    dns_sdlzlookup_t *lookup);

/*
 * dlz_dlopen_allowzonexfr() is optional, and should be supplied if
 * you want to support zone transfers
 */
typedef isc_result_t dlz_dlopen_allowzonexfr_t(void *dbdata,
					       const char *name,
					       const char *client);

/*
 * dlz_dlopen_allnodes() is optional, but must be supplied if supply a
 * dlz_dlopen_allowzonexfr() function
 */
typedef isc_result_t dlz_dlopen_allnodes_t(const char *zone,
					   void *dbdata,
					   dns_sdlzallnodes_t *allnodes);

/*
 * dlz_dlopen_newversion() is optional. It should be supplied if you
 * want to support dynamic updates.
 */
typedef isc_result_t dlz_dlopen_newversion_t(const char *zone,
					     void *dbdata,
					     void **versionp);

/*
 * dlz_closeversion() is optional, but must be supplied if you supply
 * a dlz_newversion() function
 */
typedef void dlz_dlopen_closeversion_t(const char *zone,
				       isc_boolean_t commit,
				       void *dbdata,
				       void **versionp);

/*
 * dlz_dlopen_configure() is optional, but must be supplied if you
 * want to support dynamic updates
 */
typedef isc_result_t dlz_dlopen_configure_t(dns_view_t *view,
					    dns_dlzdb_t *dlzdb,
					    void *dbdata);

/*
 * dlz_dlopen_setclientcallback() is optional, but must be supplied if you
 * want to retrieve information about the client (e.g., source address)
 * before sending a replay.
 */
typedef isc_result_t dlz_dlopen_setclientcallback_t(dns_view_t *view,
						    void *dbdata);


/*
 * dlz_dlopen_ssumatch() is optional, but must be supplied if you want
 * to support dynamic updates
 */
typedef isc_boolean_t dlz_dlopen_ssumatch_t(const char *signer,
					    const char *name,
					    const char *tcpaddr,
					    const char *type,
					    const char *key,
					    isc_uint32_t keydatalen,
					    unsigned char *keydata,
					    void *dbdata);

/*
 * dlz_dlopen_addrdataset() is optional, but must be supplied if you
 * want to support dynamic updates
 */
typedef isc_result_t dlz_dlopen_addrdataset_t(const char *name,
					      const char *rdatastr,
					      void *dbdata,
					      void *version);

/*
 * dlz_dlopen_subrdataset() is optional, but must be supplied if you
 * want to support dynamic updates
 */
typedef isc_result_t dlz_dlopen_subrdataset_t(const char *name,
					      const char *rdatastr,
					      void *dbdata,
					      void *version);

/*
 * dlz_dlopen_delrdataset() is optional, but must be supplied if you
 * want to support dynamic updates
 */
typedef isc_result_t dlz_dlopen_delrdataset_t(const char *name,
					      const char *type,
					      void *dbdata,
					      void *version);

ISC_LANG_ENDDECLS

#endif
