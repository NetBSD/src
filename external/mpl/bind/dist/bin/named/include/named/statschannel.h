/*	$NetBSD: statschannel.h,v 1.2 2018/08/12 13:02:28 christos Exp $	*/

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

#ifndef NAMED_STATSCHANNEL_H
#define NAMED_STATSCHANNEL_H 1

/*! \file
 * \brief
 * The statistics channels built-in the name server.
 */

#include <isccc/types.h>

#include <isccfg/aclconf.h>

#include <named/types.h>

#define NAMED_STATSCHANNEL_HTTPPORT		80

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

#endif	/* NAMED_STATSCHANNEL_H */
