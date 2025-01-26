/*	$NetBSD: aclconf.h,v 1.9 2025/01/26 16:25:45 christos Exp $	*/

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

#include <inttypes.h>

#include <isc/lang.h>

#include <dns/geoip.h>
#include <dns/types.h>

#include <isccfg/cfg.h>

typedef struct cfg_aclconfctx {
	ISC_LIST(dns_acl_t) named_acl_cache;
	isc_mem_t *mctx;
#if defined(HAVE_GEOIP2)
	dns_geoip_databases_t *geoip;
#endif /* if defined(HAVE_GEOIP2) */
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
		   isc_log_t *lctx, cfg_aclconfctx_t *ctx, isc_mem_t *mctx,
		   unsigned int nest_level, dns_acl_t **target);
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
 * On success, attach '*target' to the new dns_acl_t object.
 *
 * Require:
 *	'ctx' to be non NULL.
 *	'*target' to be NULL or a valid dns_acl_t.
 */

ISC_LANG_ENDDECLS
