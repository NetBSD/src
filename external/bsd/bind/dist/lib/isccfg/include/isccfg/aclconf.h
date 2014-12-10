/*	$NetBSD: aclconf.h,v 1.7 2014/12/10 04:38:02 christos Exp $	*/

/*
 * Copyright (C) 2004-2007, 2010-2014  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
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

/* Id */

#ifndef ISCCFG_ACLCONF_H
#define ISCCFG_ACLCONF_H 1

#include <isc/lang.h>

#include <isccfg/cfg.h>

#ifdef HAVE_GEOIP
#include <dns/geoip.h>
#endif
#include <dns/types.h>

typedef struct cfg_aclconfctx {
	ISC_LIST(dns_acl_t) named_acl_cache;
	isc_mem_t *mctx;
#ifdef HAVE_GEOIP
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
		   isc_uint16_t family, dns_acl_t **target);
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
