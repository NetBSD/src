/*	$NetBSD: dnssectool.h,v 1.2 2018/08/12 13:02:27 christos Exp $	*/

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


#ifndef DNSSECTOOL_H
#define DNSSECTOOL_H 1

#include <isc/log.h>
#include <isc/stdtime.h>
#include <dns/rdatastruct.h>
#include <dst/dst.h>

#define check_dns_dbiterator_current(result) \
	check_result((result == DNS_R_NEWORIGIN) ? ISC_R_SUCCESS : result, \
		     "dns_dbiterator_current()")


typedef void (fatalcallback_t)(void);

ISC_PLATFORM_NORETURN_PRE void
fatal(const char *format, ...)
ISC_FORMAT_PRINTF(1, 2) ISC_PLATFORM_NORETURN_POST;

void
setfatalcallback(fatalcallback_t *callback);

void
check_result(isc_result_t result, const char *message);

void
vbprintf(int level, const char *fmt, ...) ISC_FORMAT_PRINTF(2, 3);

ISC_PLATFORM_NORETURN_PRE void
version(const char *program) ISC_PLATFORM_NORETURN_POST;

void
type_format(const dns_rdatatype_t type, char *cp, unsigned int size);
#define TYPE_FORMATSIZE 20

void
sig_format(dns_rdata_rrsig_t *sig, char *cp, unsigned int size);
#define SIG_FORMATSIZE (DNS_NAME_FORMATSIZE + DNS_SECALG_FORMATSIZE + sizeof("65535"))

void
setup_logging(isc_mem_t *mctx, isc_log_t **logp);

void
cleanup_logging(isc_log_t **logp);

void
setup_entropy(isc_mem_t *mctx, const char *randomfile, isc_entropy_t **ectx);

void
cleanup_entropy(isc_entropy_t **ectx);

dns_ttl_t strtottl(const char *str);

isc_stdtime_t
strtotime(const char *str, isc_int64_t now, isc_int64_t base,
	  isc_boolean_t *setp);

unsigned int
strtodsdigest(const char *str);

dns_rdataclass_t
strtoclass(const char *str);

isc_result_t
try_dir(const char *dirname);

void
check_keyversion(dst_key_t *key, char *keystr);

void
set_keyversion(dst_key_t *key);

isc_boolean_t
key_collision(dst_key_t *key, dns_name_t *name, const char *dir,
	      isc_mem_t *mctx, isc_boolean_t *exact);

isc_boolean_t
is_delegation(dns_db_t *db, dns_dbversion_t *ver, dns_name_t *origin,
		      dns_name_t *name, dns_dbnode_t *node, isc_uint32_t *ttlp);

/*%
 * Return ISC_TRUE if version 'ver' of database 'db' contains a DNAME RRset at
 * 'node'; return ISC_FALSE otherwise.
 */
isc_boolean_t
has_dname(dns_db_t *db, dns_dbversion_t *ver, dns_dbnode_t *node);

void
verifyzone(dns_db_t *db, dns_dbversion_t *ver,
		   dns_name_t *origin, isc_mem_t *mctx,
		   isc_boolean_t ignore_kskflag, isc_boolean_t keyset_kskonly);

isc_boolean_t
isoptarg(const char *arg, char **argv, void (*usage)(void));

#ifdef _WIN32
void InitSockets(void);
void DestroySockets(void);
#endif

#endif /* DNSSEC_DNSSECTOOL_H */
