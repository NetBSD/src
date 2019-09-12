/*	$NetBSD: aclconf.h,v 1.3.4.1 2019/09/12 19:18:17 martin Exp $	*/

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


#ifndef ISCCFG_ACLCONF_H
#define ISCCFG_ACLCONF_H 1

#include <inttypes.h>

#include <isc/lang.h>

#include <isccfg/cfg.h>

#include <dns/geoip.h>
#include <dns/types.h>

typedef struct cfg_aclconfctx {
	ISC_LIST(dns_acl_t) named_acl_cache;
	isc_mem_t *mctx;
#if defined(HAVE_GEOIP) || defined(HAVE_GEOIP2)
	dns_geoip_databases_t *geoip;
#endif
	isc_refcount_t references;
} cfg_aclconfctx_t;

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

isc_result_t
cfg_aclconfctx_create(isc_mem_t *mctx, cfg_aclconfctx_t **ret);
/*
 * Creates and initializes an ACL configuration context.
 */

void
cfg_aclconfctx_detach(cfg_aclconfctx_t **actxp);
/*
 * Removes a reference to an ACL configuration context; when references
 * reaches zero, clears the contents and deallocate the structure.
 */

void
cfg_aclconfctx_attach(cfg_aclconfctx_t *src, cfg_aclconfctx_t **dest);
/*
 * Attaches a pointer to an existing ACL configuration context.
 */

isc_result_t
cfg_acl_fromconfig(const cfg_obj_t *caml, const cfg_obj_t *cctx,
		   isc_log_t *lctx, cfg_aclconfctx_t *ctx,
		   isc_mem_t *mctx, unsigned int nest_level,
		   dns_acl_t **target);

isc_result_t
cfg_acl_fromconfig2(const cfg_obj_t *caml, const cfg_obj_t *cctx,
		   isc_log_t *lctx, cfg_aclconfctx_t *ctx,
		   isc_mem_t *mctx, unsigned int nest_level,
		   uint16_t family, dns_acl_t **target);
/*
 * Construct a new dns_acl_t from configuration data in 'caml' and
 * 'cctx'.  Memory is allocated through 'mctx'.
 *
 * Any named ACLs referred to within 'caml' will be be converted
 * into nested dns_acl_t objects.  Multiple references to the same
 * named ACLs will be converted into shared references to a single
 * nested dns_acl_t object when the referring objects were created
 * passing the same ACL configuration context 'ctx'.
 *
 * cfg_acl_fromconfig() is a backward-compatible version of
 * cfg_acl_fromconfig2(), which allows an address family to be
 * specified.  If 'family' is not zero, then only addresses/prefixes
 * of a matching family (AF_INET or AF_INET6) may be configured.
 *
 * On success, attach '*target' to the new dns_acl_t object.
 */

ISC_LANG_ENDDECLS

#endif /* ISCCFG_ACLCONF_H */
