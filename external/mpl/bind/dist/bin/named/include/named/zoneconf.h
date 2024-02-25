/*	$NetBSD: zoneconf.h,v 1.6.2.1 2024/02/25 15:43:07 martin Exp $	*/

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

/*! \file */

#include <stdbool.h>

#include <isc/lang.h>
#include <isc/types.h>

#include <isccfg/aclconf.h>
#include <isccfg/cfg.h>

ISC_LANG_BEGINDECLS

isc_result_t
named_zone_configure(const cfg_obj_t *config, const cfg_obj_t *vconfig,
		     const cfg_obj_t *zconfig, cfg_aclconfctx_t *ac,
		     dns_kasplist_t *kasplist, dns_zone_t *zone,
		     dns_zone_t *raw);
/*%<
 * Configure or reconfigure a zone according to the named.conf
 * data.
 *
 * The zone origin is not configured, it is assumed to have been set
 * at zone creation time.
 *
 * Require:
 * \li	'ac' to point to an initialized cfg_aclconfctx_t.
 * \li	'kasplist' to be initialized.
 * \li	'zone' to be initialized.
 */

bool
named_zone_reusable(dns_zone_t *zone, const cfg_obj_t *zconfig);
/*%<
 * If 'zone' can be safely reconfigured according to the configuration
 * data in 'zconfig', return true.  If the configuration data is so
 * different from the current zone state that the zone needs to be destroyed
 * and recreated, return false.
 */

bool
named_zone_inlinesigning(const cfg_obj_t *zconfig);
/*%<
 * Determine if zone uses inline-signing. This is true if inline-signing
 * is set to yes.
 */

isc_result_t
named_zone_configure_writeable_dlz(dns_dlzdb_t *dlzdatabase, dns_zone_t *zone,
				   dns_rdataclass_t rdclass, dns_name_t *name);
/*%>
 * configure a DLZ zone, setting up the database methods and calling
 * postload to load the origin values
 *
 * Require:
 * \li	'dlzdatabase' to be a valid dlz database
 * \li	'zone' to be initialized.
 * \li	'rdclass' to be a valid rdataclass
 * \li	'name' to be a valid zone origin name
 */

ISC_LANG_ENDDECLS
