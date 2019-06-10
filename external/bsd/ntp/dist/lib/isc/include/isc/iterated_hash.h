/*	$NetBSD: iterated_hash.h,v 1.4.16.1 2019/06/10 21:51:16 christos Exp $	*/

/*
 * Copyright (C) 2008  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id */

#ifndef ISC_ITERATED_HASH_H
#define ISC_ITERATED_HASH_H 1

#include <isc/lang.h>
#include <isc/sha1.h>

/*
 * The maximal hash length that can be encoded it a name
 * using base32hex.  floor(255/8)*5
 */
#define NSEC3_MAX_HASH_LENGTH 155

/*
 * The maximum has that can be encoded in a single label using
 * base32hex.  floor(63/8)*5
 */
#define NSEC3_MAX_LABEL_HASH 35

ISC_LANG_BEGINDECLS

int isc_iterated_hash(unsigned char out[NSEC3_MAX_HASH_LENGTH],
		      unsigned int hashalg, int iterations,
		      const unsigned char *salt, int saltlength,
		      const unsigned char *in, int inlength);


ISC_LANG_ENDDECLS

#endif /* ISC_ITERATED_HASH_H */
