/*	$NetBSD: kaspconf.h,v 1.6 2025/01/26 16:25:45 christos Exp $	*/

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

#include <isc/lang.h>

#include <isccfg/cfg.h>

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

isc_result_t
cfg_kasp_fromconfig(const cfg_obj_t *config, dns_kasp_t *default_kasp,
		    bool check_algorithms, isc_mem_t *mctx, isc_log_t *logctx,
		    dns_keystorelist_t *keystorelist, dns_kasplist_t *kasplist,
		    dns_kasp_t **kaspp);
/*%<
 * Create and configure a KASP. If 'default_kasp' is not NULL, the built-in
 * default configuration is used to set values that are not explicitly set in
 * the policy.
 *
 * If a 'kasplist' is provided, a lookup happens and if a KASP already exists
 * with the same name, no new KASP is created, and no attach to 'kaspp' happens.
 *
 * The 'keystorelist' is where to lookup key stores if KASP keys are using them.
 *
 * If 'check_algorithms' is true then the dnssec-policy DNSSEC key
 * algorithms are checked against those supported by the crypto provider.
 *
 * Requires:
 *
 *\li  'name' is either NULL, or a valid C string.
 *
 *\li  'mctx' is a valid memory context.
 *
 *\li  'logctx' is a valid logging context.
 *
 *\li  kaspp != NULL && *kaspp == NULL
 *
 * Returns:
 *
 *\li  #ISC_R_SUCCESS  If creating and configuring the KASP succeeds.
 *\li  #ISC_R_EXISTS   If 'kasplist' already has a kasp structure with 'name'.
 *\li  #ISC_R_NOMEMORY
 *
 *\li  Other errors are possible.
 */

isc_result_t
cfg_keystore_fromconfig(const cfg_obj_t *config, isc_mem_t *mctx,
			isc_log_t *logctx, const char *engine,
			dns_keystorelist_t *keystorelist,
			dns_keystore_t	  **kspp);
/*%<
 * Create and configure a key store. If a 'keystorelist' is provided, a lookup
 * happens and if a keystore already exists with the same name, no new one is
 * created, and no attach to 'kspp' happens.
 *
 * Requires:
 *
 *\li  config != NULL

 *\li  'mctx' is a valid memory context.
 *
 *\li  'logctx' is a valid logging context.
 *
 *\li  kspp == NULL || *kspp == NULL
 *
 * Returns:
 *
 *\li  #ISC_R_SUCCESS  If creating and configuring the keystore succeeds.
 *\li  #ISC_R_EXISTS   If 'keystorelist' already has a keystore with 'name'.
 *\li  #ISC_R_NOMEMORY
 *
 *\li  Other errors are possible.
 */

ISC_LANG_ENDDECLS
