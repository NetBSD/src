/*	$NetBSD: dnssectool.h,v 1.6.2.1 2024/02/25 15:43:04 martin Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>

#include <isc/attributes.h>
#include <isc/log.h>
#include <isc/stdtime.h>

#include <dns/rdatastruct.h>

#include <dst/dst.h>

/*! verbosity: set by -v and -q option in each program, defined in dnssectool.c
 */
extern int verbose;
extern bool quiet;

/*! program name, statically initialized in each program */
extern const char *program;

/*!
 * List of DS digest types used by dnssec-cds and dnssec-dsfromkey,
 * defined in dnssectool.c. Filled in by add_dtype() from -a
 * arguments, sorted (so that DS records are in a canonical order) and
 * terminated by a zero. The size of the array is an arbitrary limit
 * which should be greater than the number of known digest types.
 */
extern uint8_t dtype[8];

typedef void(fatalcallback_t)(void);

noreturn void
fatal(const char *format, ...) ISC_FORMAT_PRINTF(1, 2);

void
setfatalcallback(fatalcallback_t *callback);

void
check_result(isc_result_t result, const char *message);

void
vbprintf(int level, const char *fmt, ...) ISC_FORMAT_PRINTF(2, 3);

noreturn void
version(const char *program);

void
sig_format(dns_rdata_rrsig_t *sig, char *cp, unsigned int size);
#define SIG_FORMATSIZE \
	(DNS_NAME_FORMATSIZE + DNS_SECALG_FORMATSIZE + sizeof("65535"))

void
setup_logging(isc_mem_t *mctx, isc_log_t **logp);

void
cleanup_logging(isc_log_t **logp);

dns_ttl_t
strtottl(const char *str);

dst_key_state_t
strtokeystate(const char *str);

isc_stdtime_t
strtotime(const char *str, int64_t now, int64_t base, bool *setp);

dns_rdataclass_t
strtoclass(const char *str);

unsigned int
strtodsdigest(const char *str);

void
add_dtype(unsigned int dt);

isc_result_t
try_dir(const char *dirname);

void
check_keyversion(dst_key_t *key, char *keystr);

void
set_keyversion(dst_key_t *key);

bool
key_collision(dst_key_t *key, dns_name_t *name, const char *dir,
	      isc_mem_t *mctx, bool *exact);

bool
isoptarg(const char *arg, char **argv, void (*usage)(void));
