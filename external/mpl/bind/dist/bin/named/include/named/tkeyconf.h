/*	$NetBSD: tkeyconf.h,v 1.6.2.1 2024/02/25 15:43:07 martin Exp $	*/

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

#include <isccfg/cfg.h>

ISC_LANG_BEGINDECLS

isc_result_t
named_tkeyctx_fromconfig(const cfg_obj_t *options, isc_mem_t *mctx,
			 dns_tkeyctx_t **tctxp);
/*%<
 * 	Create a TKEY context and configure it, including the default DH key
 *	and default domain, according to 'options'.
 *
 *	Requires:
 *\li		'cfg' is a valid configuration options object.
 *\li		'mctx' is not NULL
 *\li		'tctx' is not NULL
 *\li		'*tctx' is NULL
 *
 *	Returns:
 *\li		ISC_R_SUCCESS
 *\li		ISC_R_NOMEMORY
 */

ISC_LANG_ENDDECLS
