/*	$NetBSD: statschannel.h,v 1.5.2.1 2024/02/25 15:43:07 martin Exp $	*/

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

/*! \file
 * \brief
 * The statistics channels built-in the name server.
 */

#include <isccfg/aclconf.h>

#include <isccc/types.h>
#include <named/types.h>

#define NAMED_STATSCHANNEL_HTTPPORT 80

isc_result_t
named_statschannels_configure(named_server_t *server, const cfg_obj_t *config,
			      cfg_aclconfctx_t *aclconfctx);
/*%<
 * [Re]configure the statistics channels.
 *
 * If it is no longer there but was previously configured, destroy
 * it here.
 *
 * If the IP address or port has changed, destroy the old server
 * and create a new one.
 */

void
named_statschannels_shutdown(named_server_t *server);
/*%<
 * Initiate shutdown of all the statistics channel listeners.
 */

isc_result_t
named_stats_dump(named_server_t *server, FILE *fp);
/*%<
 * Dump statistics counters managed by the server to the file fp.
 */
