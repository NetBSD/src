/*	$NetBSD: transportconf.h,v 1.2.2.2 2024/02/25 15:43:07 martin Exp $	*/

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

#include <isc/lang.h>
#include <isc/types.h>

#include <dns/transport.h>

#include <isccfg/cfg.h>

ISC_LANG_BEGINDECLS

isc_result_t
named_transports_fromconfig(const cfg_obj_t *config, const cfg_obj_t *vconfig,
			    isc_mem_t *mctx, dns_transport_list_t **listp);
/*%<
 * Create a list of transport objects (DoT or DoH) and configure them
 * according to 'key-file', 'cert-file', 'ca-file' or 'hostname'
 * statements.
 *
 *	Requires:
 *	\li	'config' is not NULL.
 *	\li	'vconfig' is not NULL.
 *	\li	'mctx' is not NULL
 *	\li	'listp' is not NULL, and '*listp' is NULL
 *
 */

ISC_LANG_ENDDECLS
