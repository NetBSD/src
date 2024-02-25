/*	$NetBSD: journal.h,v 1.7.2.1 2024/02/25 15:46:56 martin Exp $	*/

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

/*****
***** Module Info
*****/

/*! \file dns/journal.h
 * \brief
 * Database journaling.
 */

/***
 *** Imports
 ***/

#include <inttypes.h>
#include <stdbool.h>

#include <isc/lang.h>
#include <isc/magic.h>

#include <dns/diff.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/types.h>

/***
 *** Defines.
 ***/
#define DNS_JOURNALOPT_RESIGN 0x00000001

#define DNS_JOURNAL_READ   0x00000000 /* false */
#define DNS_JOURNAL_CREATE 0x00000001 /* true */
#define DNS_JOURNAL_WRITE  0x00000002

#define DNS_JOURNAL_SIZE_MAX INT32_MAX
#define DNS_JOURNAL_SIZE_MIN 4096

/*% Print transaction header data */
#define DNS_JOURNAL_PRINTXHDR 0x0001

/*% Rewrite whole journal file instead of compacting */
#define DNS_JOURNAL_COMPACTALL 0x0001
#define DNS_JOURNAL_VERSION1   0x0002

/***
 *** Types
 ***/

/*%
 * A dns_journal_t represents an open journal file.  This is an opaque type.
 *
 * A particular dns_journal_t object may be opened for writing, in which case
 * it can be used for writing transactions to a journal file, or it can be
 * opened for reading, in which case it can be used for reading transactions
 * from (iterating over) a journal file.  A single dns_journal_t object may
 * not be used for both purposes.
 */
typedef struct dns_journal dns_journal_t;

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

/**************************************************************************/

isc_result_t
dns_db_createsoatuple(dns_db_t *db, dns_dbversion_t *ver, isc_mem_t *mctx,
		      dns_diffop_t op, dns_difftuple_t **tp);
/*!< brief
 * Create a diff tuple for the current database SOA.
 * XXX this probably belongs somewhere else.
 */

/*@{*/
#define DNS_SERIAL_GT(a, b) ((int)(((a) - (b)) & 0xFFFFFFFF) > 0)
#define DNS_SERIAL_GE(a, b) ((int)(((a) - (b)) & 0xFFFFFFFF) >= 0)
/*!< brief
 * Compare SOA serial numbers.  DNS_SERIAL_GT(a, b) returns true iff
 * a is "greater than" b where "greater than" is as defined in RFC1982.
 * DNS_SERIAL_GE(a, b) returns true iff a is "greater than or equal to" b.
 */
/*@}*/

/**************************************************************************/
/*
 * Journal object creation and destruction.
 */

isc_result_t
dns_journal_open(isc_mem_t *mctx, const char *filename, unsigned int mode,
		 dns_journal_t **journalp);
/*%<
 * Open the journal file 'filename' and create a dns_journal_t object for it.
 *
 * DNS_JOURNAL_CREATE open the journal for reading and writing and create
 * the journal if it does not exist.
 * DNS_JOURNAL_WRITE open the journal for reading and writing.
 * DNS_JOURNAL_READ open the journal for reading only.
 */

void
dns_journal_destroy(dns_journal_t **journalp);
/*%<
 * Destroy a dns_journal_t, closing any open files and freeing its memory.
 */

/**************************************************************************/
/*
 * Writing transactions to journals.
 */

isc_result_t
dns_journal_begin_transaction(dns_journal_t *j);
/*%<
 * Prepare to write a new transaction to the open journal file 'j'.
 *
 * Requires:
 *     \li 'j' is open for writing.
 */

isc_result_t
dns_journal_writediff(dns_journal_t *j, dns_diff_t *diff);
/*%<
 * Write 'diff' to the current transaction of journal file 'j'.
 *
 * Requires:
 * \li     'j' is open for writing and dns_journal_begin_transaction()
 * 	has been called.
 *
 *\li 	'diff' is a full or partial, correctly ordered IXFR
 *      difference sequence.
 */

isc_result_t
dns_journal_commit(dns_journal_t *j);
/*%<
 * Commit the current transaction of journal file 'j'.
 *
 * Requires:
 * \li     'j' is open for writing and dns_journal_begin_transaction()
 * 	has been called.
 *
 *   \li   dns_journal_writediff() has been called one or more times
 * 	to form a complete, correctly ordered IXFR difference
 *      sequence.
 */

isc_result_t
dns_journal_write_transaction(dns_journal_t *j, dns_diff_t *diff);
/*%
 * Write a complete transaction at once to a journal file,
 * sorting it if necessary, and commit it.  Equivalent to calling
 * dns_diff_sort(), dns_journal_begin_transaction(),
 * dns_journal_writediff(), and dns_journal_commit().
 *
 * Requires:
 *\li      'j' is open for writing.
 *
 * \li	'diff' contains exactly one SOA deletion, one SOA addition
 *       with a greater serial number, and possibly other changes,
 *       in arbitrary order.
 */

/**************************************************************************/
/*
 * Reading transactions from journals.
 */

bool
dns_journal_empty(dns_journal_t *j);
/*<
 * Find out if a journal is empty.
 */

bool
dns_journal_recovered(dns_journal_t *j);
/*<
 * Find out if the journal could be opened using old journal format
 */

uint32_t
dns_journal_first_serial(dns_journal_t *j);
uint32_t
dns_journal_last_serial(dns_journal_t *j);
/*%<
 * Get the first and last addressable serial number in the journal.
 */

isc_result_t
dns_journal_iter_init(dns_journal_t *j, uint32_t begin_serial,
		      uint32_t end_serial, size_t *xfrsizep);
/*%<
 * Prepare to iterate over the transactions that will bring the database
 * from SOA serial number 'begin_serial' to 'end_serial'.
 *
 * If 'xfrsizep' is not NULL, then on success it will be set to the
 * total size of all records in the iteration (excluding headers). This
 * is meant to be a rough approximation of the size of an incremental
 * zone transfer, though it does not account for DNS message overhead
 * or name compression.)
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_RANGE	begin_serial is outside the addressable range.
 *\li	ISC_R_NOTFOUND	begin_serial is within the range of addressable
 *			serial numbers covered by the journal, but
 *			this particular serial number does not exist.
 */

/*@{*/
isc_result_t
dns_journal_first_rr(dns_journal_t *j);
isc_result_t
dns_journal_next_rr(dns_journal_t *j);
/*%<
 * Position the iterator at the first/next RR in a journal
 * transaction sequence established using dns_journal_iter_init().
 *
 * Requires:
 *    \li  dns_journal_iter_init() has been called.
 *
 */
/*@}*/

void
dns_journal_current_rr(dns_journal_t *j, dns_name_t **name, uint32_t *ttl,
		       dns_rdata_t **rdata);
/*%<
 * Get the name, ttl, and rdata of the current journal RR.
 *
 * Requires:
 * \li     The last call to dns_journal_first_rr() or dns_journal_next_rr()
 *      returned ISC_R_SUCCESS.
 */

/**************************************************************************/
/*
 * Database roll-forward.
 */

isc_result_t
dns_journal_rollforward(dns_journal_t *j, dns_db_t *db, unsigned int options);
/*%<
 * Roll forward (play back) the journal file "filename" into the
 * database "db".  This should be called when the server starts
 * after a shutdown or crash.
 *
 * Requires:
 *\li   'journal' is a valid journal
 *\li	'db' is a valid database which does not have a version
 *           open for writing.
 *
 * Returns:
 *\li	ISC_R_NOTFOUND when current serial in not in journal.
 *\li	ISC_R_RANGE when current serial in not in journals range.
 *\li	DNS_R_UPTODATE when the database was already up to date.
 *\li	ISC_R_SUCCESS journal has been applied successfully to the
 *      database without any issues.
 *
 *	others
 */

isc_result_t
dns_journal_print(isc_mem_t *mctx, uint32_t flags, const char *filename,
		  FILE *file);
/* For debugging not general use */

isc_result_t
dns_db_diff(isc_mem_t *mctx, dns_db_t *dba, dns_dbversion_t *dbvera,
	    dns_db_t *dbb, dns_dbversion_t *dbverb,
	    const char *journal_filename);

isc_result_t
dns_db_diffx(dns_diff_t *diff, dns_db_t *dba, dns_dbversion_t *dbvera,
	     dns_db_t *dbb, dns_dbversion_t *dbverb,
	     const char *journal_filename);
/*%<
 * Compare the databases 'dba' and 'dbb' and generate a diff/journal
 * entry containing the changes to make 'dba' from 'dbb' (note
 * the order).  This journal entry will consist of a single,
 * possibly very large transaction.  Append the journal
 * entry to the journal file specified by 'journal_filename' if
 * non-NULL.
 */

isc_result_t
dns_journal_compact(isc_mem_t *mctx, char *filename, uint32_t serial,
		    uint32_t flags, uint32_t target_size);
/*%<
 * Attempt to compact the journal if it is greater that 'target_size'.
 * Changes from 'serial' onwards will be preserved. Changes prior than
 * that may be dropped in order to get the journal below `target_size`.
 *
 * If 'flags' includes DNS_JOURNAL_COMPACTALL, the entire journal is copied.
 * In this case, `serial` is ignored. This flag is used when upgrading or
 * downgrading the format version of the journal. If 'flags' also includes
 * DNS_JOURNAL_VERSION1, then the journal is copied out in the original
 * format used prior to BIND 9.16.12; otherwise it is copied in the
 * current format.
 *
 * If _COMPACTALL is not in use, and the journal file exists and is
 * non-empty, then 'serial' must exist in the journal.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_RANGE	serial is outside the range existing in the journal
 *
 * Other errors may be returned from file operations.
 */

bool
dns_journal_get_sourceserial(dns_journal_t *j, uint32_t *sourceserial);
void
dns_journal_set_sourceserial(dns_journal_t *j, uint32_t sourceserial);
/*%<
 * Get and set source serial.
 *
 * Returns:
 *      true if sourceserial has previously been set.
 */

ISC_LANG_ENDDECLS
