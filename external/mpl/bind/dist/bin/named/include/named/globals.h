/*	$NetBSD: globals.h,v 1.9.2.1 2024/02/25 15:43:07 martin Exp $	*/

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

#include <isc/log.h>
#include <isc/net.h>
#include <isc/netmgr.h>
#include <isc/rwlock.h>

#include <dns/acl.h>
#include <dns/zone.h>

#include <isccfg/aclconf.h>
#include <isccfg/cfg.h>

#include <dst/dst.h>
#include <named/fuzz.h>
#include <named/types.h>

#undef EXTERN
#undef INIT
#ifdef NAMED_MAIN
#define EXTERN
#define INIT(v) = (v)
#else /* ifdef NAMED_MAIN */
#define EXTERN extern
#define INIT(v)
#endif /* ifdef NAMED_MAIN */

#ifndef NAMED_RUN_PID_DIR
#define NAMED_RUN_PID_DIR 1
#endif /* ifndef NAMED_RUN_PID_DIR */

EXTERN isc_mem_t *named_g_mctx		      INIT(NULL);
EXTERN unsigned int named_g_cpus	      INIT(0);
EXTERN unsigned int named_g_udpdisp	      INIT(0);
EXTERN isc_taskmgr_t *named_g_taskmgr	      INIT(NULL);
EXTERN dns_dispatchmgr_t *named_g_dispatchmgr INIT(NULL);
EXTERN unsigned int named_g_cpus_detected     INIT(1);

#ifdef ENABLE_AFL
EXTERN bool named_g_run_done INIT(false);
#endif /* ifdef ENABLE_AFL */
/*
 * XXXRTH  We're going to want multiple timer managers eventually.  One
 *         for really short timers, another for client timers, and one
 *         for zone timers.
 */
EXTERN isc_timermgr_t *named_g_timermgr INIT(NULL);
EXTERN isc_nm_t *named_g_netmgr		INIT(NULL);
EXTERN cfg_parser_t *named_g_parser	INIT(NULL);
EXTERN cfg_parser_t *named_g_addparser	INIT(NULL);
EXTERN const char *named_g_version	INIT(PACKAGE_VERSION);
EXTERN const char *named_g_product	INIT(PACKAGE_NAME);
EXTERN const char *named_g_description	INIT(PACKAGE_DESCRIPTION);
EXTERN const char *named_g_srcid	INIT(PACKAGE_SRCID);
EXTERN const char *named_g_configargs	INIT(PACKAGE_CONFIGARGS);
EXTERN const char *named_g_builder	INIT(PACKAGE_BUILDER);
EXTERN in_port_t named_g_port		INIT(0);
EXTERN in_port_t named_g_tlsport	INIT(0);
EXTERN in_port_t named_g_httpsport	INIT(0);
EXTERN in_port_t named_g_httpport	INIT(0);

EXTERN in_port_t named_g_http_listener_clients INIT(0);
EXTERN in_port_t named_g_http_streams_per_conn INIT(0);

EXTERN named_server_t *named_g_server INIT(NULL);

/*
 * Logging.
 */
EXTERN isc_log_t *named_g_lctx		     INIT(NULL);
EXTERN isc_logcategory_t *named_g_categories INIT(NULL);
EXTERN isc_logmodule_t *named_g_modules	     INIT(NULL);
EXTERN unsigned int named_g_debuglevel	     INIT(0);

/*
 * Current configuration information.
 */
EXTERN cfg_obj_t *named_g_config	   INIT(NULL);
EXTERN const cfg_obj_t *named_g_defaults   INIT(NULL);
EXTERN const char *named_g_conffile	   INIT(NAMED_SYSCONFDIR "/named.conf");
EXTERN const char *named_g_defaultbindkeys INIT(NAMED_SYSCONFDIR "/bind.keys");
EXTERN const char *named_g_keyfile	   INIT(NAMED_SYSCONFDIR "/rndc.key");

EXTERN dns_tsigkey_t *named_g_sessionkey    INIT(NULL);
EXTERN dns_name_t			    named_g_sessionkeyname;
EXTERN bool named_g_conffileset		    INIT(false);
EXTERN cfg_aclconfctx_t *named_g_aclconfctx INIT(NULL);

/*
 * Initial resource limits.
 */
EXTERN isc_resourcevalue_t named_g_initstacksize INIT(0);
EXTERN isc_resourcevalue_t named_g_initdatasize	 INIT(0);
EXTERN isc_resourcevalue_t named_g_initcoresize	 INIT(0);
EXTERN isc_resourcevalue_t named_g_initopenfiles INIT(0);

/*
 * Misc.
 */
EXTERN bool named_g_coreok	     INIT(true);
EXTERN const char *named_g_chrootdir INIT(NULL);
EXTERN bool named_g_foreground	     INIT(false);
EXTERN bool named_g_logstderr	     INIT(false);
EXTERN bool named_g_nosyslog	     INIT(false);
EXTERN const char *named_g_logfile   INIT(NULL);

EXTERN const char *named_g_defaultsessionkeyfile INIT(NAMED_LOCALSTATEDIR
						      "/run/named/"
						      "session.key");
EXTERN const char *named_g_defaultlockfile INIT(NAMED_LOCALSTATEDIR "/run/"
								    "named/"
								    "named."
								    "lock");
EXTERN bool named_g_forcelock		   INIT(false);

#if NAMED_RUN_PID_DIR
EXTERN const char *named_g_defaultpidfile INIT(NAMED_LOCALSTATEDIR "/run/named/"
								   "named.pid");
#else  /* if NAMED_RUN_PID_DIR */
EXTERN const char *named_g_defaultpidfile INIT(NAMED_LOCALSTATEDIR "/run/"
								   "named.pid");
#endif /* if NAMED_RUN_PID_DIR */

EXTERN const char *named_g_username INIT(NULL);

EXTERN const char *named_g_engine INIT(NULL);

EXTERN isc_time_t		  named_g_boottime;
EXTERN isc_time_t		  named_g_configtime;
EXTERN bool named_g_memstatistics INIT(false);
EXTERN bool named_g_keepstderr	  INIT(false);

EXTERN unsigned int named_g_tat_interval INIT(24 * 3600);
EXTERN unsigned int named_g_maxcachesize INIT(0);

#if defined(HAVE_GEOIP2)
EXTERN dns_geoip_databases_t *named_g_geoip INIT(NULL);
#endif /* if defined(HAVE_GEOIP2) */

EXTERN const char *named_g_fuzz_addr	INIT(NULL);
EXTERN isc_fuzztype_t named_g_fuzz_type INIT(isc_fuzz_none);

EXTERN dns_acl_t *named_g_mapped INIT(NULL);

#undef EXTERN
#undef INIT
