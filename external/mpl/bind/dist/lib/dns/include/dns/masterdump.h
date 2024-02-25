/*	$NetBSD: masterdump.h,v 1.7.2.1 2024/02/25 15:46:57 martin Exp $	*/

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

/*! \file dns/masterdump.h */

/***
 ***	Imports
 ***/

#include <stdio.h>

#include <isc/lang.h>

#include <dns/types.h>

/***
 *** Types
 ***/

typedef struct dns_master_style dns_master_style_t;

/***
 *** Definitions
 ***/

/*
 * Flags affecting master file formatting.  Flags 0x0000FFFF
 * define the formatting of the rdata part and are defined in
 * rdata.h.
 */

/*% Omit the owner name when possible. */
#define DNS_STYLEFLAG_OMIT_OWNER 0x000010000ULL

/*%
 * Omit the TTL when possible.  If DNS_STYLEFLAG_TTL is
 * also set, this means no TTLs are ever printed
 * because $TTL directives are generated before every
 * change in the TTL.  In this case, no columns need to
 * be reserved for the TTL.  Master files generated with
 * these options will be rejected by BIND 4.x because it
 * does not recognize the $TTL directive.
 *
 * If DNS_STYLEFLAG_TTL is not also set, the TTL will be
 * omitted when it is equal to the previous TTL.
 * This is correct according to RFC1035, but the
 * TTLs may be silently misinterpreted by older
 * versions of BIND which use the SOA MINTTL as a
 * default TTL value.
 */
#define DNS_STYLEFLAG_OMIT_TTL 0x000020000ULL

/*% Omit the class when possible. */
#define DNS_STYLEFLAG_OMIT_CLASS 0x000040000ULL

/*% Output $TTL directives. */
#define DNS_STYLEFLAG_TTL 0x000080000ULL

/*%
 * Output $ORIGIN directives and print owner names relative to
 * the origin when possible.
 */
#define DNS_STYLEFLAG_REL_OWNER 0x000100000ULL

/*% Print domain names in RR data in relative form when possible.
 * For this to take effect, DNS_STYLEFLAG_REL_OWNER must also be set. */
#define DNS_STYLEFLAG_REL_DATA 0x000200000ULL

/*% Print the trust level of each rdataset. */
#define DNS_STYLEFLAG_TRUST 0x000400000ULL

/*% Print negative caching entries. */
#define DNS_STYLEFLAG_NCACHE 0x000800000ULL

/*% Never print the TTL. */
#define DNS_STYLEFLAG_NO_TTL 0x001000000ULL

/*% Never print the CLASS. */
#define DNS_STYLEFLAG_NO_CLASS 0x002000000ULL

/*% Report re-signing time. */
#define DNS_STYLEFLAG_RESIGN 0x004000000ULL

/*% Don't printout the cryptographic parts of DNSSEC records. */
#define DNS_STYLEFLAG_NOCRYPTO 0x008000000ULL

/*% Comment out data by prepending with ";" */
#define DNS_STYLEFLAG_COMMENTDATA 0x010000000ULL

/*% Print TTL with human-readable units. */
#define DNS_STYLEFLAG_TTL_UNITS 0x020000000ULL

/*% Indent output. */
#define DNS_STYLEFLAG_INDENT 0x040000000ULL

/*% Output in YAML style. */
#define DNS_STYLEFLAG_YAML 0x080000000ULL

/*% Print ECS cache entries as comments (reserved for future use). */
#define DNS_STYLEFLAG_ECSCACHE 0x100000000ULL

/*% Print expired cache entries. */
#define DNS_STYLEFLAG_EXPIRED 0x200000000ULL

ISC_LANG_BEGINDECLS

/***
 ***	Constants
 ***/

/*%
 * The default master file style.
 *
 * This uses $TTL directives to avoid the need to dedicate a
 * tab stop for the TTL.  The class is only printed for the first
 * rrset in the file and shares a tab stop with the RR type.
 */
extern const dns_master_style_t dns_master_style_default;

/*%
 * A master file style that dumps zones to a very generic format easily
 * imported/checked with external tools.
 */
extern const dns_master_style_t dns_master_style_full;

/*%
 * A master file style that prints explicit TTL values on each
 * record line, never using $TTL statements.  The TTL has a tab
 * stop of its own, but the class and type share one.
 */
extern const dns_master_style_t dns_master_style_explicitttl;

/*%
 * A master style format designed for cache files.  It prints explicit TTL
 * values on each record line and never uses $ORIGIN or relative names.
 */
extern const dns_master_style_t dns_master_style_cache;

/*%
 * A master style format designed for cache files.  The same as above but
 * this also prints expired entries.
 */
extern const dns_master_style_t dns_master_style_cache_with_expired;

/*%
 * A master style that prints name, ttl, class, type, and value on
 * every line.  Similar to explicitttl above, but more verbose.
 * Intended for generating master files which can be easily parsed
 * by perl scripts and similar applications.
 */
extern const dns_master_style_t dns_master_style_simple;

/*%
 * The style used for debugging, "dig" output, etc.
 */
extern const dns_master_style_t dns_master_style_debug;

/*%
 * Similar to dns_master_style_debug but data is prepended with ";"
 */
extern const dns_master_style_t dns_master_style_comment;

/*%
 * Similar to dns_master_style_debug but data is indented with "\t" (tab)
 */
extern const dns_master_style_t dns_master_style_indent;

/*%
 * The style used for dumping "key" zones.
 */
extern const dns_master_style_t dns_master_style_keyzone;

/*%
 * YAML-compatible output
 */
extern const dns_master_style_t dns_master_style_yaml;

/***
 ***	Functions
 ***/

void
dns_dumpctx_attach(dns_dumpctx_t *source, dns_dumpctx_t **target);
/*%<
 * Attach to a dump context.
 *
 * Require:
 *\li	'source' to be valid.
 *\li	'target' to be non NULL and '*target' to be NULL.
 */

void
dns_dumpctx_detach(dns_dumpctx_t **dctxp);
/*%<
 * Detach from a dump context.
 *
 * Require:
 *\li	'dctxp' to point to a valid dump context.
 *
 * Ensures:
 *\li	'*dctxp' is NULL.
 */

void
dns_dumpctx_cancel(dns_dumpctx_t *dctx);
/*%<
 * Cancel a in progress dump.
 *
 * Require:
 *\li	'dctx' to be valid.
 */

dns_dbversion_t *
dns_dumpctx_version(dns_dumpctx_t *dctx);
/*%<
 * Return the version handle (if any) of the database being dumped.
 *
 * Require:
 *\li	'dctx' to be valid.
 */

dns_db_t *
dns_dumpctx_db(dns_dumpctx_t *dctx);
/*%<
 * Return the database being dumped.
 *
 * Require:
 *\li	'dctx' to be valid.
 */

/*@{*/
isc_result_t
dns_master_dumptostreamasync(isc_mem_t *mctx, dns_db_t *db,
			     dns_dbversion_t	      *version,
			     const dns_master_style_t *style, FILE *f,
			     isc_task_t *task, dns_dumpdonefunc_t done,
			     void *done_arg, dns_dumpctx_t **dctxp);

isc_result_t
dns_master_dumptostream(isc_mem_t *mctx, dns_db_t *db, dns_dbversion_t *version,
			const dns_master_style_t *style,
			dns_masterformat_t	  format,
			dns_masterrawheader_t *header, FILE *f);
/*%<
 * Dump the database 'db' to the steam 'f' in the specified format by
 * 'format'.  If the format is dns_masterformat_text (the RFC1035 format),
 * 'style' specifies the file style (e.g., &dns_master_style_default).
 *
 * If 'format' is dns_masterformat_raw, then 'header' can contain
 * information to be written to the file header.
 *
 * Temporary dynamic memory may be allocated from 'mctx'.
 *
 * Require:
 *\li	'task' to be valid.
 *\li	'done' to be non NULL.
 *\li	'dctxp' to be non NULL && '*dctxp' to be NULL.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_NOMEMORY
 *\li	Any database or rrset iterator error.
 *\li	Any dns_rdata_totext() error code.
 */
/*@}*/

/*@{*/

isc_result_t
dns_master_dumpasync(isc_mem_t *mctx, dns_db_t *db, dns_dbversion_t *version,
		     const dns_master_style_t *style, const char *filename,
		     isc_task_t *task, dns_dumpdonefunc_t done, void *done_arg,
		     dns_dumpctx_t **dctxp, dns_masterformat_t format,
		     dns_masterrawheader_t *header);

isc_result_t
dns_master_dump(isc_mem_t *mctx, dns_db_t *db, dns_dbversion_t *version,
		const dns_master_style_t *style, const char *filename,
		dns_masterformat_t format, dns_masterrawheader_t *header);

/*%<
 * Dump the database 'db' to the file 'filename' in the specified format by
 * 'format'.  If the format is dns_masterformat_text (the RFC1035 format),
 * 'style' specifies the file style (e.g., &dns_master_style_default).
 *
 * If 'format' is dns_masterformat_raw, then 'header' can contain
 * information to be written to the file header.
 *
 * Temporary dynamic memory may be allocated from 'mctx'.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_NOMEMORY
 *\li	Any database or rrset iterator error.
 *\li	Any dns_rdata_totext() error code.
 */
/*@}*/

isc_result_t
dns_master_rdatasettotext(const dns_name_t	   *owner_name,
			  dns_rdataset_t	   *rdataset,
			  const dns_master_style_t *style, dns_indent_t *indent,
			  isc_buffer_t *target);
/*%<
 * Convert 'rdataset' to text format, storing the result in 'target'.
 *
 * Notes:
 *\li	The rdata cursor position will be changed.
 *
 * Requires:
 *\li	'rdataset' is a valid non-question rdataset.
 *
 *\li	'rdataset' is not empty.
 */

isc_result_t
dns_master_questiontotext(const dns_name_t	   *owner_name,
			  dns_rdataset_t	   *rdataset,
			  const dns_master_style_t *style,
			  isc_buffer_t		   *target);

isc_result_t
dns_master_dumpnodetostream(isc_mem_t *mctx, dns_db_t *db,
			    dns_dbversion_t *version, dns_dbnode_t *node,
			    const dns_name_t	     *name,
			    const dns_master_style_t *style, FILE *f);

isc_result_t
dns_master_dumpnode(isc_mem_t *mctx, dns_db_t *db, dns_dbversion_t *version,
		    dns_dbnode_t *node, const dns_name_t *name,
		    const dns_master_style_t *style, const char *filename);

dns_masterstyle_flags_t
dns_master_styleflags(const dns_master_style_t *style);

isc_result_t
dns_master_stylecreate(dns_master_style_t    **style,
		       dns_masterstyle_flags_t flags, unsigned int ttl_column,
		       unsigned int class_column, unsigned int type_column,
		       unsigned int rdata_column, unsigned int line_length,
		       unsigned int tab_width, unsigned int split_width,
		       isc_mem_t *mctx);

void
dns_master_styledestroy(dns_master_style_t **style, isc_mem_t *mctx);

ISC_LANG_ENDDECLS
