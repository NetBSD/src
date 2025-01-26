/*	$NetBSD: compress.h,v 1.8 2025/01/26 16:25:26 christos Exp $	*/

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

#include <isc/lang.h>
#include <isc/region.h>

#include <dns/name.h>
#include <dns/types.h>

ISC_LANG_BEGINDECLS

/*! \file dns/compress.h
 * Direct manipulation of the structures is strongly discouraged.
 *
 * A name compression context handles compression of multiple DNS names in
 * relation to a single DNS message. The context can be used to selectively
 * turn on/off compression for specific names (depending on the RR type,
 * according to RFC 3597) by using \c dns_compress_setpermitted().
 *
 * The nameserver can be configured not to use compression at all using
 * \c dns_compress_disable().
 *
 * DNS name compression only needs exact matches on (suffixes of) names. We
 * could use a data structure that supports longest-match lookups, but that
 * would introduce a lot of heavyweight machinery, and all we need is
 * something that exists very briefly to store a few names before it is
 * thrown away.
 *
 * In the abstract we need a map from DNS names to compression offsets. But
 * a compression offset refers to a point in the message where the name has
 * been written. So in fact all we need is a hash set of compression offsets.
 *
 * Typical messages do not contain more than a few dozen names, so by
 * default our hash set is small (64 entries, 256 bytes). It can be
 * enlarged when a message is likely to contain a lot of names, such as for
 * outgoing zone transfers (which are handled in lib/ns/xfrout.c) and
 * update requests (for which nsupdate uses DNS_REQUESTOPT_LARGE - see
 * request.h).
 */

/*
 * Logarithms of hash set sizes. In the usual (small) case, allow for for a
 * few dozen names in the hash set. (We can't actually use every slot because
 * space is reserved for performance reasons.) For large messages, the number
 * of names is limited by the minimum size of an RR (owner, type, class, ttl,
 * length) which is 12 bytes - call it 16 bytes to make space for a new label.
 * Divide the maximum compression offset 0x4000 by 16 and you get 0x400 == 1024.
 * In practice, the root zone (for example) uses less than 200 distinct names
 * per message.
 */
enum {
	DNS_COMPRESS_SMALLBITS = 6,
	DNS_COMPRESS_LARGEBITS = 10,
};

/*
 * Compression context flags
 */
enum dns_compress_flags {
	/* affecting the whole message */
	DNS_COMPRESS_DISABLED = 0x00000001U,
	DNS_COMPRESS_CASE = 0x00000002U,
	DNS_COMPRESS_LARGE = 0x00000004U,
	/* can toggle while rendering a message */
	DNS_COMPRESS_PERMITTED = 0x00000008U,
};

/*
 * The hash may be any 16 bit value. Unused slots have coff == 0. (Valid
 * compression offsets cannot be zero because of the DNS message header.)
 */
struct dns_compress_slot {
	uint16_t hash;
	uint16_t coff;
};

struct dns_compress {
	unsigned int	     magic;
	dns_compress_flags_t flags;
	uint16_t	     mask;
	uint16_t	     count;
	isc_mem_t	    *mctx;
	dns_compress_slot_t *set;
	dns_compress_slot_t  smallset[1 << DNS_COMPRESS_SMALLBITS];
};

/*
 * Deompression context
 */
enum dns_decompress {
	DNS_DECOMPRESS_DEFAULT,
	DNS_DECOMPRESS_PERMITTED,
	DNS_DECOMPRESS_NEVER,
	DNS_DECOMPRESS_ALWAYS,
};

void
dns_compress_init(dns_compress_t *cctx, isc_mem_t *mctx,
		  dns_compress_flags_t flags);
/*%<
 *	Initialise the compression context structure pointed to by
 *	'cctx'.
 *
 *	The `flags` argument is usually zero; or some combination of:
 *\li		DNS_COMPRESS_DISABLED, so the whole message is uncompressed
 *\li		DNS_COMPRESS_CASE, for case-sensitive compression
 *\li		DNS_COMPRESS_LARGE, for messages with many names
 *
 *	(See also dns_request_create()'s options argument)
 *
 *	Requires:
 *\li		'cctx' is a dns_compress_t structure on the stack.
 *\li		'mctx' is an initialized memory context.
 *	Ensures:
 *\li		'cctx' is initialized.
 *\li		'dns_compress_getpermitted(cctx)' is true
 */

void
dns_compress_invalidate(dns_compress_t *cctx);

/*%<
 *	Invalidate the compression structure pointed to by
 *	'cctx', freeing any memory that has been allocated.
 *
 *	Requires:
 *\li		'cctx' is an initialized dns_compress_t
 */

void
dns_compress_setpermitted(dns_compress_t *cctx, bool permitted);

/*%<
 *	Sets whether compression is allowed, according to RFC 3597.
 *	This can vary depending on the rdata type.
 *
 *	Requires:
 *\li		'cctx' to be initialized.
 */

bool
dns_compress_getpermitted(dns_compress_t *cctx);

/*%<
 *	Find out whether compression is allowed, according to RFC 3597.
 *
 *	Requires:
 *\li		'cctx' to be initialized.
 *
 *	Returns:
 *\li		allowed compression bitmap.
 */

void
dns_compress_name(dns_compress_t *cctx, isc_buffer_t *buffer,
		  const dns_name_t *name, unsigned int *return_prefix,
		  unsigned int *return_coff);
/*%<
 *	Finds longest suffix matching 'name' in the compression table,
 *	and adds any remaining prefix of 'name' to the table.
 *
 *	This is used by dns_name_towire() for both compressed and uncompressed
 *	names; for uncompressed names, dns_name_towire() does not need to know
 *	about the matching suffix, but it still needs to add the name for use
 *	by later compression pointers. For example, an owner name of a record
 *	in the additional section will often need to refer back to an RFC 3597
 *	uncompressed name in the rdata of a record in the answer section.
 *
 *	Requires:
 *\li		'cctx' to be initialized.
 *\li		'buffer' contains the rendered message.
 *\li		'name' to be a absolute name.
 *\li		'return_prefix' points to an unsigned int.
 *\li		'return_coff' points to an unsigned int, which must be zero.
 *
 *	Ensures:
 *\li		When no suffix is found, the return variables
 *              'return_prefix' and 'return_coff' are unchanged
 *
 *\li		Otherwise, '*return_prefix' is set to the length of the
 *		prefix of the name that did not match, and '*suffix_coff'
 *		is set to a nonzero compression offset of the match.
 */

void
dns_compress_rollback(dns_compress_t *cctx, unsigned int offset);
/*%<
 *	Remove any compression pointers from the table that are >= offset.
 *
 *	Requires:
 *\li		'cctx' is initialized.
 */

/*%
 *	Set whether decompression is allowed, according to RFC 3597
 */
static inline dns_decompress_t /* inline to suppress code generation */
dns_decompress_setpermitted(dns_decompress_t dctx, bool permitted) {
	if (dctx == DNS_DECOMPRESS_NEVER || dctx == DNS_DECOMPRESS_ALWAYS) {
		return dctx;
	} else if (permitted) {
		return DNS_DECOMPRESS_PERMITTED;
	} else {
		return DNS_DECOMPRESS_DEFAULT;
	}
}

/*%
 *	Returns whether decompression is allowed here
 */
static inline bool /* inline to suppress code generation */
dns_decompress_getpermitted(dns_decompress_t dctx) {
	return dctx == DNS_DECOMPRESS_ALWAYS ||
	       dctx == DNS_DECOMPRESS_PERMITTED;
}

ISC_LANG_ENDDECLS
