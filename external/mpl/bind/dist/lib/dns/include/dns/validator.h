/*	$NetBSD: validator.h,v 1.11 2025/01/26 16:25:29 christos Exp $	*/

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

/*! \file dns/validator.h
 *
 * \brief
 * DNS Validator
 * This is the BIND 9 validator, the module responsible for validating the
 * rdatasets and negative responses (messages).  It makes use of zones in
 * the view and may fetch RRset to complete trust chains.  It implements
 * DNSSEC as specified in RFC 4033, 4034 and 4035.
 *
 * Correct operation is critical to preventing spoofed answers from secure
 * zones being accepted.
 *
 * MP:
 *\li	The module ensures appropriate synchronization of data structures it
 *	creates and manipulates.
 *
 * Reliability:
 *\li	No anticipated impact.
 *
 * Resources:
 *\li	TBS
 *
 * Security:
 *\li	No anticipated impact.
 *
 * Standards:
 *\li	RFCs:	1034, 1035, 2181, 4033, 4034, 4035.
 */

#include <stdbool.h>

#include <isc/job.h>
#include <isc/lang.h>
#include <isc/refcount.h>

#include <dns/fixedname.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h> /* for dns_rdata_rrsig_t */
#include <dns/types.h>

#include <dst/dst.h>

#define DNS_VALIDATOR_NOQNAMEPROOF    0
#define DNS_VALIDATOR_NODATAPROOF     1
#define DNS_VALIDATOR_NOWILDCARDPROOF 2
#define DNS_VALIDATOR_CLOSESTENCLOSER 3

/*%
 * A validator object represents a validation in progress.
 * \brief
 * Clients are strongly discouraged from using this type directly, with
 * the exception of the 'link' field, which may be used directly for
 * whatever purpose the client desires.
 */
struct dns_validator {
	unsigned int   magic;
	dns_view_t    *view;
	isc_loop_t    *loop;
	uint32_t       tid;
	isc_refcount_t references;

	/* Name and type of the response to be validated. */
	dns_name_t     *name;
	dns_rdatatype_t type;

	/*
	 * Callback and argument to use to inform the caller
	 * that validation is complete.
	 */
	isc_job_cb cb;
	void	  *arg;

	/* Validation options (_DEFER, _NONTA, etc). */
	unsigned int options;

	/*
	 * Results of a completed validation.
	 */
	isc_result_t result;

	/*
	 * Rdata and RRSIG (if any) for positive responses.
	 */
	dns_rdataset_t *rdataset;
	dns_rdataset_t *sigrdataset;
	/*
	 * The full response.  Required for negative responses.
	 * Also required for positive wildcard responses.
	 */
	dns_message_t *message;
	/*
	 * Proofs to be cached.
	 */
	dns_name_t *proofs[4];
	/*
	 * Optout proof seen.
	 */
	bool optout;
	/*
	 * Answer is secure.
	 */
	bool secure;

	/* Internal validator state */
	atomic_bool	   canceling;
	unsigned int	   attributes;
	dns_fetch_t	  *fetch;
	dns_validator_t	  *subvalidator;
	dns_validator_t	  *parent;
	dns_keytable_t	  *keytable;
	dst_key_t	  *key;
	dns_rdata_rrsig_t *siginfo;
	unsigned int	   labels;
	dns_rdataset_t	  *nxset;
	dns_rdataset_t	  *keyset;
	dns_rdataset_t	  *dsset;
	dns_rdataset_t	   fdsset;
	dns_rdataset_t	   frdataset;
	dns_rdataset_t	   fsigrdataset;
	dns_fixedname_t	   fname;
	dns_fixedname_t	   wild;
	dns_fixedname_t	   closest;
	ISC_LINK(dns_validator_t) link;
	bool	      mustbesecure;
	unsigned int  depth;
	unsigned int  authcount;
	unsigned int  authfail;
	isc_stdtime_t start;

	bool	       digest_sha1;
	bool	       supported_algorithm;
	dns_rdata_t    rdata;
	bool	       resume;
	uint32_t      *nvalidations;
	uint32_t      *nfails;
	isc_counter_t *qc;
};

/*%
 * dns_validator_create() options.
 */
/* obsolete: #define DNS_VALIDATOR_DLV	0x0001U */
#define DNS_VALIDATOR_DEFER    0x0002U
#define DNS_VALIDATOR_NOCDFLAG 0x0004U
#define DNS_VALIDATOR_NONTA    0x0008U /*% Ignore NTA table */

ISC_LANG_BEGINDECLS

isc_result_t
dns_validator_create(dns_view_t *view, dns_name_t *name, dns_rdatatype_t type,
		     dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset,
		     dns_message_t *message, unsigned int options,
		     isc_loop_t *loop, isc_job_cb cb, void *arg,
		     uint32_t *nvalidations, uint32_t *nfails,
		     isc_counter_t *qc, dns_validator_t **validatorp);
/*%<
 * Start a DNSSEC validation.
 *
 * On success (which is guaranteed as long as the view has valid
 * trust anchors), `validatorp` is updated to point to the new
 * validator. The caller is responsible for detaching it.
 *
 * The validator will validate a response to the question given by
 * 'name' and 'type'.
 *
 * To validate a positive response, the response data is
 * given by 'rdataset' and 'sigrdataset'.  If 'sigrdataset'
 * is NULL, the data is presumed insecure and an attempt
 * is made to prove its insecurity by finding the appropriate
 * null key.
 *
 * The complete response message may be given in 'message',
 * to make available any authority section NSECs that may be
 * needed for validation of a response resulting from a
 * wildcard expansion (though no such wildcard validation
 * is implemented yet).  If the complete response message
 * is not available, 'message' is NULL.
 *
 * To validate a negative response, the complete negative response
 * message is given in 'message'.  The 'rdataset', and
 * 'sigrdataset' arguments must be NULL, but the 'name' and 'type'
 * arguments must be provided.
 *
 * The validation is performed in the context of 'view'.
 *
 * When the validation finishes, the callback function 'cb' is
 * called, passing a dns_valstatus_t object which contains a
 * poiner to 'arg'. The caller is responsible for freeing this
 * object.
 *
 * Its 'result' field will be ISC_R_SUCCESS iff the
 * response was successfully proven to be either secure or
 * part of a known insecure domain.
 */

void
dns_validator_send(dns_validator_t *validator);
/*%<
 * Send a deferred validation request
 *
 * Requires:
 *	'validator' to points to a valid DNSSEC validator.
 */

void
dns_validator_cancel(dns_validator_t *validator);
/*%<
 * Cancel a DNSSEC validation in progress.
 *
 * Requires:
 *\li	'validator' points to a valid DNSSEC validator, which
 *	may or may not already have completed.
 *
 * Ensures:
 *\li	It the validator has not already sent its completion
 *	event, it will send it with result code ISC_R_CANCELED.
 */

void
dns_validator_shutdown(dns_validator_t *val);
/*%<
 * Release the name associated with the DNSSEC validator.
 *
 * Requires:
 * \li	'val' points to a valid DNSSEC validator.
 * \li	The validator must have completed and sent its completion
 *	event.
 *
 * Ensures:
 *\li	The name associated with the DNSSEC validator is released.
 */

#if DNS_VALIDATOR_TRACE
#define dns_validator_ref(ptr) \
	dns_validator__ref(ptr, __func__, __FILE__, __LINE__)
#define dns_validator_unref(ptr) \
	dns_validator__unref(ptr, __func__, __FILE__, __LINE__)
#define dns_validator_attach(ptr, ptrp) \
	dns_validator__attach(ptr, ptrp, __func__, __FILE__, __LINE__)
#define dns_validator_detach(ptrp) \
	dns_validator__detach(ptrp, __func__, __FILE__, __LINE__)
ISC_REFCOUNT_TRACE_DECL(dns_validator);
#else
ISC_REFCOUNT_DECL(dns_validator);
#endif

ISC_LANG_ENDDECLS
