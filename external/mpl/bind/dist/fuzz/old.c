/*	$NetBSD: old.c,v 1.1.1.1 2025/01/26 16:12:29 christos Exp $	*/

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

#include <isc/ascii.h>
#include <isc/buffer.h>
#include <isc/result.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/compress.h>
#include <dns/types.h>

/*
 */

#include "old.h"

/*
 * code copied from lib/dns/name.c as of commit
 * 6967973568fe80b03e1729259f8907ce8792be34
 */

typedef enum { fw_start = 0, fw_ordinary, fw_newcurrent } fw_state;

#define VALID_NAME(n) ISC_MAGIC_VALID(n, DNS_NAME_MAGIC)

#define INIT_OFFSETS(name, var, default_offsets) \
	if ((name)->offsets != NULL)             \
		var = (name)->offsets;           \
	else                                     \
		var = (default_offsets);

#define MAKE_EMPTY(name)                           \
	do {                                       \
		name->ndata = NULL;                \
		name->length = 0;                  \
		name->labels = 0;                  \
		name->attributes.absolute = false; \
	} while (0)

#define BINDABLE(name) (!name->attributes.readonly && !name->attributes.dynamic)

isc_result_t
old_name_fromwire(dns_name_t *name, isc_buffer_t *source, dns_decompress_t dctx,
		  unsigned int options, isc_buffer_t *target) {
	unsigned char *cdata, *ndata;
	unsigned int cused; /* Bytes of compressed name data used */
	unsigned int nused, labels, n, nmax;
	unsigned int current, new_current, biggest_pointer;
	bool done;
	fw_state state = fw_start;
	unsigned int c;
	unsigned char *offsets;
	dns_offsets_t odata;
	bool downcase;
	bool seen_pointer;

	/*
	 * Copy the possibly-compressed name at source into target,
	 * decompressing it.  Loop prevention is performed by checking
	 * the new pointer against biggest_pointer.
	 */

	REQUIRE(VALID_NAME(name));
	REQUIRE((target != NULL && ISC_BUFFER_VALID(target)) ||
		(target == NULL && ISC_BUFFER_VALID(name->buffer)));

	downcase = ((options & DNS_NAME_DOWNCASE) != 0);

	if (target == NULL && name->buffer != NULL) {
		target = name->buffer;
		isc_buffer_clear(target);
	}

	REQUIRE(BINDABLE(name));

	INIT_OFFSETS(name, offsets, odata);

	/*
	 * Make 'name' empty in case of failure.
	 */
	MAKE_EMPTY(name);

	/*
	 * Initialize things to make the compiler happy; they're not required.
	 */
	n = 0;
	new_current = 0;

	/*
	 * Set up.
	 */
	labels = 0;
	done = false;

	ndata = isc_buffer_used(target);
	nused = 0;
	seen_pointer = false;

	/*
	 * Find the maximum number of uncompressed target name
	 * bytes we are willing to generate.  This is the smaller
	 * of the available target buffer length and the
	 * maximum legal domain name length (255).
	 */
	nmax = isc_buffer_availablelength(target);
	if (nmax > DNS_NAME_MAXWIRE) {
		nmax = DNS_NAME_MAXWIRE;
	}

	cdata = isc_buffer_current(source);
	cused = 0;

	current = source->current;
	biggest_pointer = current;

	/*
	 * Note:  The following code is not optimized for speed, but
	 * rather for correctness.  Speed will be addressed in the future.
	 */

	while (current < source->active && !done) {
		c = *cdata++;
		current++;
		if (!seen_pointer) {
			cused++;
		}

		switch (state) {
		case fw_start:
			if (c < 64) {
				offsets[labels] = nused;
				labels++;
				if (nused + c + 1 > nmax) {
					goto full;
				}
				nused += c + 1;
				*ndata++ = c;
				if (c == 0) {
					done = true;
				}
				n = c;
				state = fw_ordinary;
			} else if (c >= 192) {
				/*
				 * 14-bit compression pointer
				 */
				if (!dns_decompress_getpermitted(dctx)) {
					return DNS_R_DISALLOWED;
				}
				new_current = c & 0x3F;
				state = fw_newcurrent;
			} else {
				return DNS_R_BADLABELTYPE;
			}
			break;
		case fw_ordinary:
			if (downcase) {
				c = isc_ascii_tolower(c);
			}
			*ndata++ = c;
			n--;
			if (n == 0) {
				state = fw_start;
			}
			break;
		case fw_newcurrent:
			new_current *= 256;
			new_current += c;
			if (new_current >= biggest_pointer) {
				return DNS_R_BADPOINTER;
			}
			biggest_pointer = new_current;
			current = new_current;
			cdata = (unsigned char *)source->base + current;
			seen_pointer = true;
			state = fw_start;
			break;
		default:
			FATAL_ERROR("Unknown state %d", state);
			/* Does not return. */
		}
	}

	if (!done) {
		return ISC_R_UNEXPECTEDEND;
	}

	name->ndata = (unsigned char *)target->base + target->used;
	name->labels = labels;
	name->length = nused;
	name->attributes.absolute = true;

	isc_buffer_forward(source, cused);
	isc_buffer_add(target, name->length);

	return ISC_R_SUCCESS;

full:
	if (nmax == DNS_NAME_MAXWIRE) {
		/*
		 * The name did not fit even though we had a buffer
		 * big enough to fit a maximum-length name.
		 */
		return DNS_R_NAMETOOLONG;
	} else {
		/*
		 * The name might fit if only the caller could give us a
		 * big enough buffer.
		 */
		return ISC_R_NOSPACE;
	}
}
