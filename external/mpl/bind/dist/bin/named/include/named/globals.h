/*	$NetBSD: globals.h,v 1.4 2019/02/24 20:01:27 christos Exp $	*/

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

#ifndef NAMED_GLOBALS_H
#define NAMED_GLOBALS_H 1

/*! \file */

#include <stdbool.h>

#include <isc/rwlock.h>
#include <isc/log.h>
#include <isc/net.h>

#include <isccfg/aclconf.h>
#include <isccfg/cfg.h>

#include <dns/acl.h>
#include <dns/zone.h>

#include <dst/dst.h>

#include <named/types.h>
#include <named/fuzz.h>

#undef EXTERN
#undef INIT
#ifdef NAMED_MAIN
#define EXTERN
#define INIT(v)	= (v)
#else
#define EXTERN extern
#define INIT(v)
#endif

#ifndef NAMED_RUN_PID_DIR
#define NAMED_RUN_PID_DIR 1
#endif

EXTERN isc_mem_t *		named_g_mctx		INIT(NULL);
EXTERN unsigned int		named_g_cpus		INIT(0);
EXTERN unsigned int		named_g_udpdisp		INIT(0);
EXTERN isc_taskmgr_t *		named_g_taskmgr		INIT(NULL);
EXTERN dns_dispatchmgr_t *	named_g_dispatchmgr	INIT(NULL);
EXTERN unsigned int		named_g_cpus_detected	INIT(1);

#ifdef ENABLE_AFL
EXTERN bool		named_g_run_done	INIT(false);
#endif
/*
 * XXXRTH  We're going to want multiple timer managers eventually.  One
 *         for really short timers, another for client timers, and one
 *         for zone timers.
 */
EXTERN isc_timermgr_t *		named_g_timermgr	INIT(NULL);
EXTERN isc_socketmgr_t *	named_g_socketmgr	INIT(NULL);
EXTERN cfg_parser_t *		named_g_parser		INIT(NULL);
EXTERN cfg_parser_t *		named_g_addparser	INIT(NULL);
EXTERN const char *		named_g_version		INIT(VERSION);
EXTERN const char *		named_g_product		INIT(PRODUCT);
EXTERN const char *		named_g_description	INIT(DESCRIPTION);
EXTERN const char *		named_g_srcid		INIT(SRCID);
EXTERN const char *		named_g_configargs	INIT(CONFIGARGS);
EXTERN const char *		named_g_builder		INIT(BUILDER);
EXTERN in_port_t		named_g_port		INIT(0);
EXTERN isc_dscp_t		named_g_dscp		INIT(-1);

EXTERN named_server_t *		named_g_server		INIT(NULL);

/*
 * Logging.
 */
EXTERN isc_log_t *		named_g_lctx		INIT(NULL);
EXTERN isc_logcategory_t *	named_g_categories	INIT(NULL);
EXTERN isc_logmodule_t *	named_g_modules		INIT(NULL);
EXTERN unsigned int		named_g_debuglevel	INIT(0);

/*
 * Current configuration information.
 */
EXTERN cfg_obj_t *		named_g_config		INIT(NULL);
EXTERN const cfg_obj_t *	named_g_defaults	INIT(NULL);
EXTERN const char *		named_g_conffile	INIT(NAMED_SYSCONFDIR
							     "/named.conf");
EXTERN const char *		named_g_defaultbindkeys	INIT(NAMED_SYSCONFDIR
							     "/bind.keys");
EXTERN const char *		named_g_keyfile		INIT(NAMED_SYSCONFDIR
							     "/rndc.key");

EXTERN dns_tsigkey_t *		named_g_sessionkey	INIT(NULL);
EXTERN dns_name_t		named_g_sessionkeyname;
EXTERN bool		named_g_conffileset	INIT(false);
EXTERN cfg_aclconfctx_t *	named_g_aclconfctx	INIT(NULL);

/*
 * Initial resource limits.
 */
EXTERN isc_resourcevalue_t	named_g_initstacksize	INIT(0);
EXTERN isc_resourcevalue_t	named_g_initdatasize	INIT(0);
EXTERN isc_resourcevalue_t	named_g_initcoresize	INIT(0);
EXTERN isc_resourcevalue_t	named_g_initopenfiles	INIT(0);

/*
 * Misc.
 */
EXTERN bool		named_g_coreok		INIT(true);
EXTERN const char *		named_g_chrootdir	INIT(NULL);
EXTERN bool		named_g_foreground	INIT(false);
EXTERN bool		named_g_logstderr	INIT(false);
EXTERN bool		named_g_nosyslog	INIT(false);
EXTERN const char *		named_g_logfile		INIT(NULL);

EXTERN const char *		named_g_defaultsessionkeyfile
					INIT(NAMED_LOCALSTATEDIR "/run/named/"
							      "session.key");
EXTERN const char *		named_g_defaultlockfile	INIT(NAMED_LOCALSTATEDIR
							     "/run/named/"
							     "named.lock");
EXTERN bool		named_g_forcelock	INIT(false);

#if NAMED_RUN_PID_DIR
EXTERN const char *		named_g_defaultpidfile 	INIT(NAMED_LOCALSTATEDIR
							     "/run/named/"
							     "named.pid");
#else
EXTERN const char *		named_g_defaultpidfile 	INIT(NAMED_LOCALSTATEDIR
							     "/run/named.pid");
#endif

#ifdef HAVE_DNSTAP
EXTERN const char *		named_g_defaultdnstap
					INIT(NAMED_LOCALSTATEDIR "/run/named/"
							      "dnstap.sock");
#else
EXTERN const char *		named_g_defaultdnstap	INIT(NULL);
#endif /* HAVE_DNSTAP */

EXTERN const char *		named_g_username	INIT(NULL);

EXTERN const char *		named_g_engine		INIT(NULL);

EXTERN isc_time_t		named_g_boottime;
EXTERN isc_time_t		named_g_configtime;
EXTERN bool		named_g_memstatistics	INIT(false);
EXTERN bool		named_g_keepstderr	INIT(false);

EXTERN unsigned int		named_g_tat_interval	INIT(24*3600);

#ifdef HAVE_GEOIP
EXTERN dns_geoip_databases_t	*named_g_geoip		INIT(NULL);
#endif

EXTERN const char *		named_g_fuzz_addr	INIT(NULL);
EXTERN isc_fuzztype_t		named_g_fuzz_type	INIT(isc_fuzz_none);

EXTERN dns_acl_t *		named_g_mapped		INIT(NULL);

#undef EXTERN
#undef INIT

#endif /* NAMED_GLOBALS_H */
