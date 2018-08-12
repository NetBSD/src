/*	$NetBSD: check-tool.h,v 1.2 2018/08/12 13:02:26 christos Exp $	*/

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

#include <isc/lang.h>
#include <isc/stdio.h>
#include <isc/types.h>

#include <dns/masterdump.h>
#include <dns/types.h>

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
	  const isc_uint32_t rawversion);

#ifdef _WIN32
void InitSockets(void);
void DestroySockets(void);
#endif

extern int debug;
extern const char *journal;
extern isc_boolean_t nomerge;
extern isc_boolean_t docheckmx;
extern isc_boolean_t docheckns;
extern isc_boolean_t dochecksrv;
extern unsigned int zone_options;
extern unsigned int zone_options2;

ISC_LANG_ENDDECLS

#endif
