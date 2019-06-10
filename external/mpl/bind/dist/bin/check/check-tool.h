/*	$NetBSD: check-tool.h,v 1.3.2.2 2019/06/10 22:02:57 christos Exp $	*/

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


#ifndef CHECK_TOOL_H
#define CHECK_TOOL_H

/*! \file */

#include <inttypes.h>
#include <stdbool.h>

#include <isc/lang.h>
#include <isc/stdio.h>
#include <isc/types.h>

#include <dns/masterdump.h>
#include <dns/types.h>
#include <dns/zone.h>

ISC_LANG_BEGINDECLS

isc_result_t
setup_logging(isc_mem_t *mctx, FILE *errout, isc_log_t **logp);

isc_result_t
load_zone(isc_mem_t *mctx, const char *zonename, const char *filename,
	  dns_masterformat_t fileformat, const char *classname,
	  dns_ttl_t maxttl, dns_zone_t **zonep);

isc_result_t
dump_zone(const char *zonename, dns_zone_t *zone, const char *filename,
	  dns_masterformat_t fileformat, const dns_master_style_t *style,
	  const uint32_t rawversion);

#ifdef _WIN32
void InitSockets(void);
void DestroySockets(void);
#endif

extern int debug;
extern const char *journal;
extern bool nomerge;
extern bool docheckmx;
extern bool docheckns;
extern bool dochecksrv;
extern dns_zoneopt_t zone_options;

ISC_LANG_ENDDECLS

#endif
