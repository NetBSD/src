/*	$NetBSD: config.h,v 1.7.2.1 2024/02/25 15:43:07 martin Exp $	*/

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

#include <inttypes.h>

#include <dns/types.h>
#include <dns/zone.h>

#include <isccfg/cfg.h>

#define DEFAULT_IANA_ROOT_ZONE_PRIMARIES "_default_iana_root_zone_primaries"

isc_result_t
named_config_parsedefaults(cfg_parser_t *parser, cfg_obj_t **conf);

const char *
named_config_getdefault(void);

isc_result_t
named_config_get(cfg_obj_t const *const *maps, const char *name,
		 const cfg_obj_t **obj);

isc_result_t
named_checknames_get(const cfg_obj_t **maps, const char *const names[],
		     const cfg_obj_t **obj);

int
named_config_listcount(const cfg_obj_t *list);

isc_result_t
named_config_getclass(const cfg_obj_t *classobj, dns_rdataclass_t defclass,
		      dns_rdataclass_t *classp);

isc_result_t
named_config_gettype(const cfg_obj_t *typeobj, dns_rdatatype_t deftype,
		     dns_rdatatype_t *typep);

dns_zonetype_t
named_config_getzonetype(const cfg_obj_t *zonetypeobj);

isc_result_t
named_config_getiplist(const cfg_obj_t *config, const cfg_obj_t *list,
		       in_port_t defport, isc_mem_t *mctx,
		       isc_sockaddr_t **addrsp, uint32_t *countp);

void
named_config_putiplist(isc_mem_t *mctx, isc_sockaddr_t **addrsp,
		       uint32_t count);

isc_result_t
named_config_getremotesdef(const cfg_obj_t *cctx, const char *list,
			   const char *name, const cfg_obj_t **ret);

isc_result_t
named_config_getipandkeylist(const cfg_obj_t *config, const char *listtype,
			     const cfg_obj_t *list, isc_mem_t *mctx,
			     dns_ipkeylist_t *ipkl);

isc_result_t
named_config_getport(const cfg_obj_t *config, const char *type,
		     in_port_t *portp);

isc_result_t
named_config_getkeyalgorithm(const char *str, const dns_name_t **name,
			     uint16_t *digestbits);
isc_result_t
named_config_getkeyalgorithm2(const char *str, const dns_name_t **name,
			      unsigned int *typep, uint16_t *digestbits);
