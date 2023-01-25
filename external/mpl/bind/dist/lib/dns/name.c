/*	$NetBSD: name.c,v 1.12 2023/01/25 21:43:30 christos Exp $	*/

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

/*! \file */

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/string.h>
#include <isc/thread.h>
#include <isc/util.h>

#include <dns/compress.h>
#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/result.h>

#define VALID_NAME(n) ISC_MAGIC_VALID(n, DNS_NAME_MAGIC)

typedef enum {
	ft_init = 0,
	ft_start,
	ft_ordinary,
	ft_initialescape,
	ft_escape,
	ft_escdecimal,
	ft_at
} ft_state;

typedef enum { fw_start = 0, fw_ordinary, fw_newcurrent } fw_state;

static char digitvalue[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*16*/
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*32*/
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*48*/
	0,  1,	2,  3,	4,  5,	6,  7,	8,  9,	-1, -1, -1, -1, -1, -1, /*64*/
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*80*/
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*96*/
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*112*/
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*128*/
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*256*/
};

static unsigned char maptolower[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
	0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
	0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
	0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73,
	0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
	0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83,
	0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
	0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
	0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
	0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3,
	0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb,
	0xfc, 0xfd, 0xfe, 0xff
};

#define CONVERTTOASCII(c)
#define CONVERTFROMASCII(c)

#define INIT_OFFSETS(name, var, default_offsets) \
	if ((name)->offsets != NULL)             \
		var = (name)->offsets;           \
	else                                     \
		var = (default_offsets);

#define SETUP_OFFSETS(name, var, default_offsets) \
	if ((name)->offsets != NULL) {            \
		var = (name)->offsets;            \
	} else {                                  \
		var = (default_offsets);          \
		set_offsets(name, var, NULL);     \
	}

/*%
 * Note:  If additional attributes are added that should not be set for
 *	  empty names, MAKE_EMPTY() must be changed so it clears them.
 */
#define MAKE_EMPTY(name)                                    \
	do {                                                \
		name->ndata = NULL;                         \
		name->length = 0;                           \
		name->labels = 0;                           \
		name->attributes &= ~DNS_NAMEATTR_ABSOLUTE; \
	} while (0)

/*%
 * A name is "bindable" if it can be set to point to a new value, i.e.
 * name->ndata and name->length may be changed.
 */
#define BINDABLE(name)       \
	((name->attributes & \
	  (DNS_NAMEATTR_READONLY | DNS_NAMEATTR_DYNAMIC)) == 0)

/*%
 * Note that the name data must be a char array, not a string
 * literal, to avoid compiler warnings about discarding
 * the const attribute of a string.
 */
static unsigned char root_ndata[] = { "" };
static unsigned char root_offsets[] = { 0 };

static dns_name_t root = DNS_NAME_INITABSOLUTE(root_ndata, root_offsets);
LIBDNS_EXTERNAL_DATA const dns_name_t *dns_rootname = &root;

static unsigned char wild_ndata[] = { "\001*" };
static unsigned char wild_offsets[] = { 0 };

static dns_name_t const wild = DNS_NAME_INITNONABSOLUTE(wild_ndata,
							wild_offsets);

LIBDNS_EXTERNAL_DATA const dns_name_t *dns_wildcardname = &wild;

/*
 * dns_name_t to text post-conversion procedure.
 */
ISC_THREAD_LOCAL dns_name_totextfilter_t *totext_filter_proc = NULL;

static void
set_offsets(const dns_name_t *name, unsigned char *offsets,
	    dns_name_t *set_name);

void
dns_name_init(dns_name_t *name, unsigned char *offsets) {
	/*
	 * Initialize 'name'.
	 */
	DNS_NAME_INIT(name, offsets);
}

void
dns_name_reset(dns_name_t *name) {
	REQUIRE(VALID_NAME(name));
	REQUIRE(BINDABLE(name));

	DNS_NAME_RESET(name);
}

void
dns_name_invalidate(dns_name_t *name) {
	/*
	 * Make 'name' invalid.
	 */

	REQUIRE(VALID_NAME(name));

	name->magic = 0;
	name->ndata = NULL;
	name->length = 0;
	name->labels = 0;
	name->attributes = 0;
	name->offsets = NULL;
	name->buffer = NULL;
	ISC_LINK_INIT(name, link);
}

bool
dns_name_isvalid(const dns_name_t *name) {
	unsigned char *ndata, *offsets;
	unsigned int offset, count, length, nlabels;

	if (!VALID_NAME(name)) {
		return (false);
	}

	if (name->length > 255U || name->labels > 127U) {
		return (false);
	}

	ndata = name->ndata;
	length = name->length;
	offsets = name->offsets;
	offset = 0;
	nlabels = 0;

	while (offset != length) {
		count = *ndata;
		if (count > 63U) {
			return (false);
		}
		if (offsets != NULL && offsets[nlabels] != offset) {
			return (false);
		}

		nlabels++;
		offset += count + 1;
		ndata += count + 1;
		if (offset > length) {
			return (false);
		}

		if (count == 0) {
			break;
		}
	}

	if (nlabels != name->labels || offset != name->length) {
		return (false);
	}

	return (true);
}

void
dns_name_setbuffer(dns_name_t *name, isc_buffer_t *buffer) {
	/*
	 * Dedicate a buffer for use with 'name'.
	 */

	REQUIRE(VALID_NAME(name));
	REQUIRE((buffer != NULL && name->buffer == NULL) || (buffer == NULL));

	name->buffer = buffer;
}

bool
dns_name_hasbuffer(const dns_name_t *name) {
	/*
	 * Does 'name' have a dedicated buffer?
	 */

	REQUIRE(VALID_NAME(name));

	if (name->buffer != NULL) {
		return (true);
	}

	return (false);
}

bool
dns_name_isabsolute(const dns_name_t *name) {
	/*
	 * Does 'name' end in the root label?
	 */

	REQUIRE(VALID_NAME(name));

	if ((name->attributes & DNS_NAMEATTR_ABSOLUTE) != 0) {
		return (true);
	}
	return (false);
}

#define hyphenchar(c) ((c) == 0x2d)
#define asterchar(c)  ((c) == 0x2a)
#define alphachar(c) \
	(((c) >= 0x41 && (c) <= 0x5a) || ((c) >= 0x61 && (c) <= 0x7a))
#define digitchar(c)  ((c) >= 0x30 && (c) <= 0x39)
#define borderchar(c) (alphachar(c) || digitchar(c))
#define middlechar(c) (borderchar(c) || hyphenchar(c))
#define domainchar(c) ((c) > 0x20 && (c) < 0x7f)

bool
dns_name_ismailbox(const dns_name_t *name) {
	unsigned char *ndata, ch;
	unsigned int n;
	bool first;

	REQUIRE(VALID_NAME(name));
	REQUIRE(name->labels > 0);
	REQUIRE(name->attributes & DNS_NAMEATTR_ABSOLUTE);

	/*
	 * Root label.
	 */
	if (name->length == 1) {
		return (true);
	}

	ndata = name->ndata;
	n = *ndata++;
	INSIST(n <= 63);
	while (n--) {
		ch = *ndata++;
		if (!domainchar(ch)) {
			return (false);
		}
	}

	if (ndata == name->ndata + name->length) {
		return (false);
	}

	/*
	 * RFC952/RFC1123 hostname.
	 */
	while (ndata < (name->ndata + name->length)) {
		n = *ndata++;
		INSIST(n <= 63);
		first = true;
		while (n--) {
			ch = *ndata++;
			if (first || n == 0) {
				if (!borderchar(ch)) {
					return (false);
				}
			} else {
				if (!middlechar(ch)) {
					return (false);
				}
			}
			first = false;
		}
	}
	return (true);
}

bool
dns_name_ishostname(const dns_name_t *name, bool wildcard) {
	unsigned char *ndata, ch;
	unsigned int n;
	bool first;

	REQUIRE(VALID_NAME(name));
	REQUIRE(name->labels > 0);
	REQUIRE(name->attributes & DNS_NAMEATTR_ABSOLUTE);

	/*
	 * Root label.
	 */
	if (name->length == 1) {
		return (true);
	}

	/*
	 * Skip wildcard if this is a ownername.
	 */
	ndata = name->ndata;
	if (wildcard && ndata[0] == 1 && ndata[1] == '*') {
		ndata += 2;
	}

	/*
	 * RFC292/RFC1123 hostname.
	 */
	while (ndata < (name->ndata + name->length)) {
		n = *ndata++;
		INSIST(n <= 63);
		first = true;
		while (n--) {
			ch = *ndata++;
			if (first || n == 0) {
				if (!borderchar(ch)) {
					return (false);
				}
			} else {
				if (!middlechar(ch)) {
					return (false);
				}
			}
			first = false;
		}
	}
	return (true);
}

bool
dns_name_iswildcard(const dns_name_t *name) {
	unsigned char *ndata;

	/*
	 * Is 'name' a wildcard name?
	 */

	REQUIRE(VALID_NAME(name));
	REQUIRE(name->labels > 0);

	if (name->length >= 2) {
		ndata = name->ndata;
		if (ndata[0] == 1 && ndata[1] == '*') {
			return (true);
		}
	}

	return (false);
}

bool
dns_name_internalwildcard(const dns_name_t *name) {
	unsigned char *ndata;
	unsigned int count;
	unsigned int label;

	/*
	 * Does 'name' contain a internal wildcard?
	 */

	REQUIRE(VALID_NAME(name));
	REQUIRE(name->labels > 0);

	/*
	 * Skip first label.
	 */
	ndata = name->ndata;
	count = *ndata++;
	INSIST(count <= 63);
	ndata += count;
	label = 1;
	/*
	 * Check all but the last of the remaining labels.
	 */
	while (label + 1 < name->labels) {
		count = *ndata++;
		INSIST(count <= 63);
		if (count == 1 && *ndata == '*') {
			return (true);
		}
		ndata += count;
		label++;
	}
	return (false);
}

unsigned int
dns_name_hash(const dns_name_t *name, bool case_sensitive) {
	unsigned int length;

	/*
	 * Provide a hash value for 'name'.
	 */
	REQUIRE(VALID_NAME(name));

	if (name->labels == 0) {
		return (0);
	}

	length = name->length;
	if (length > 16) {
		length = 16;
	}

	/* High bits are more random. */
	return (isc_hash32(name->ndata, length, case_sensitive));
}

unsigned int
dns_name_fullhash(const dns_name_t *name, bool case_sensitive) {
	/*
	 * Provide a hash value for 'name'.
	 */
	REQUIRE(VALID_NAME(name));

	if (name->labels == 0) {
		return (0);
	}

	/* High bits are more random. */
	return (isc_hash32(name->ndata, name->length, case_sensitive));
}

dns_namereln_t
dns_name_fullcompare(const dns_name_t *name1, const dns_name_t *name2,
		     int *orderp, unsigned int *nlabelsp) {
	unsigned int l1, l2, l, count1, count2, count, nlabels;
	int cdiff, ldiff, chdiff;
	unsigned char *label1, *label2;
	unsigned char *offsets1, *offsets2;
	dns_offsets_t odata1, odata2;
	dns_namereln_t namereln = dns_namereln_none;

	/*
	 * Determine the relative ordering under the DNSSEC order relation of
	 * 'name1' and 'name2', and also determine the hierarchical
	 * relationship of the names.
	 *
	 * Note: It makes no sense for one of the names to be relative and the
	 * other absolute.  If both names are relative, then to be meaningfully
	 * compared the caller must ensure that they are both relative to the
	 * same domain.
	 */

	REQUIRE(VALID_NAME(name1));
	REQUIRE(VALID_NAME(name2));
	REQUIRE(orderp != NULL);
	REQUIRE(nlabelsp != NULL);
	/*
	 * Either name1 is absolute and name2 is absolute, or neither is.
	 */
	REQUIRE((name1->attributes & DNS_NAMEATTR_ABSOLUTE) ==
		(name2->attributes & DNS_NAMEATTR_ABSOLUTE));

	if (ISC_UNLIKELY(name1 == name2)) {
		*orderp = 0;
		*nlabelsp = name1->labels;
		return (dns_namereln_equal);
	}

	SETUP_OFFSETS(name1, offsets1, odata1);
	SETUP_OFFSETS(name2, offsets2, odata2);

	nlabels = 0;
	l1 = name1->labels;
	l2 = name2->labels;
	if (l2 > l1) {
		l = l1;
		ldiff = 0 - (l2 - l1);
	} else {
		l = l2;
		ldiff = l1 - l2;
	}

	offsets1 += l1;
	offsets2 += l2;

	while (ISC_LIKELY(l > 0)) {
		l--;
		offsets1--;
		offsets2--;
		label1 = &name1->ndata[*offsets1];
		label2 = &name2->ndata[*offsets2];
		count1 = *label1++;
		count2 = *label2++;

		/*
		 * We dropped bitstring labels, and we don't support any
		 * other extended label types.
		 */
		INSIST(count1 <= 63 && count2 <= 63);

		cdiff = (int)count1 - (int)count2;
		if (cdiff < 0) {
			count = count1;
		} else {
			count = count2;
		}

		/* Loop unrolled for performance */
		while (ISC_LIKELY(count > 3)) {
			chdiff = (int)maptolower[label1[0]] -
				 (int)maptolower[label2[0]];
			if (chdiff != 0) {
				*orderp = chdiff;
				goto done;
			}
			chdiff = (int)maptolower[label1[1]] -
				 (int)maptolower[label2[1]];
			if (chdiff != 0) {
				*orderp = chdiff;
				goto done;
			}
			chdiff = (int)maptolower[label1[2]] -
				 (int)maptolower[label2[2]];
			if (chdiff != 0) {
				*orderp = chdiff;
				goto done;
			}
			chdiff = (int)maptolower[label1[3]] -
				 (int)maptolower[label2[3]];
			if (chdiff != 0) {
				*orderp = chdiff;
				goto done;
			}
			count -= 4;
			label1 += 4;
			label2 += 4;
		}
		while (ISC_LIKELY(count-- > 0)) {
			chdiff = (int)maptolower[*label1++] -
				 (int)maptolower[*label2++];
			if (chdiff != 0) {
				*orderp = chdiff;
				goto done;
			}
		}
		if (cdiff != 0) {
			*orderp = cdiff;
			goto done;
		}
		nlabels++;
	}

	*orderp = ldiff;
	if (ldiff < 0) {
		namereln = dns_namereln_contains;
	} else if (ldiff > 0) {
		namereln = dns_namereln_subdomain;
	} else {
		namereln = dns_namereln_equal;
	}
	*nlabelsp = nlabels;
	return (namereln);

done:
	*nlabelsp = nlabels;
	if (nlabels > 0) {
		namereln = dns_namereln_commonancestor;
	}

	return (namereln);
}

int
dns_name_compare(const dns_name_t *name1, const dns_name_t *name2) {
	int order;
	unsigned int nlabels;

	/*
	 * Determine the relative ordering under the DNSSEC order relation of
	 * 'name1' and 'name2'.
	 *
	 * Note: It makes no sense for one of the names to be relative and the
	 * other absolute.  If both names are relative, then to be meaningfully
	 * compared the caller must ensure that they are both relative to the
	 * same domain.
	 */

	(void)dns_name_fullcompare(name1, name2, &order, &nlabels);

	return (order);
}

bool
dns_name_equal(const dns_name_t *name1, const dns_name_t *name2) {
	unsigned int l, count;
	unsigned char c;
	unsigned char *label1, *label2;

	/*
	 * Are 'name1' and 'name2' equal?
	 *
	 * Note: It makes no sense for one of the names to be relative and the
	 * other absolute.  If both names are relative, then to be meaningfully
	 * compared the caller must ensure that they are both relative to the
	 * same domain.
	 */

	REQUIRE(VALID_NAME(name1));
	REQUIRE(VALID_NAME(name2));
	/*
	 * Either name1 is absolute and name2 is absolute, or neither is.
	 */
	REQUIRE((name1->attributes & DNS_NAMEATTR_ABSOLUTE) ==
		(name2->attributes & DNS_NAMEATTR_ABSOLUTE));

	if (ISC_UNLIKELY(name1 == name2)) {
		return (true);
	}

	if (name1->length != name2->length) {
		return (false);
	}

	l = name1->labels;

	if (l != name2->labels) {
		return (false);
	}

	label1 = name1->ndata;
	label2 = name2->ndata;
	while (ISC_LIKELY(l-- > 0)) {
		count = *label1++;
		if (count != *label2++) {
			return (false);
		}

		INSIST(count <= 63); /* no bitstring support */

		/* Loop unrolled for performance */
		while (ISC_LIKELY(count > 3)) {
			c = maptolower[label1[0]];
			if (c != maptolower[label2[0]]) {
				return (false);
			}
			c = maptolower[label1[1]];
			if (c != maptolower[label2[1]]) {
				return (false);
			}
			c = maptolower[label1[2]];
			if (c != maptolower[label2[2]]) {
				return (false);
			}
			c = maptolower[label1[3]];
			if (c != maptolower[label2[3]]) {
				return (false);
			}
			count -= 4;
			label1 += 4;
			label2 += 4;
		}
		while (ISC_LIKELY(count-- > 0)) {
			c = maptolower[*label1++];
			if (c != maptolower[*label2++]) {
				return (false);
			}
		}
	}

	return (true);
}

bool
dns_name_caseequal(const dns_name_t *name1, const dns_name_t *name2) {
	/*
	 * Are 'name1' and 'name2' equal?
	 *
	 * Note: It makes no sense for one of the names to be relative and the
	 * other absolute.  If both names are relative, then to be meaningfully
	 * compared the caller must ensure that they are both relative to the
	 * same domain.
	 */

	REQUIRE(VALID_NAME(name1));
	REQUIRE(VALID_NAME(name2));
	/*
	 * Either name1 is absolute and name2 is absolute, or neither is.
	 */
	REQUIRE((name1->attributes & DNS_NAMEATTR_ABSOLUTE) ==
		(name2->attributes & DNS_NAMEATTR_ABSOLUTE));

	if (name1->length != name2->length) {
		return (false);
	}

	if (memcmp(name1->ndata, name2->ndata, name1->length) != 0) {
		return (false);
	}

	return (true);
}

int
dns_name_rdatacompare(const dns_name_t *name1, const dns_name_t *name2) {
	unsigned int l1, l2, l, count1, count2, count;
	unsigned char c1, c2;
	unsigned char *label1, *label2;

	/*
	 * Compare two absolute names as rdata.
	 */

	REQUIRE(VALID_NAME(name1));
	REQUIRE(name1->labels > 0);
	REQUIRE((name1->attributes & DNS_NAMEATTR_ABSOLUTE) != 0);
	REQUIRE(VALID_NAME(name2));
	REQUIRE(name2->labels > 0);
	REQUIRE((name2->attributes & DNS_NAMEATTR_ABSOLUTE) != 0);

	l1 = name1->labels;
	l2 = name2->labels;

	l = (l1 < l2) ? l1 : l2;

	label1 = name1->ndata;
	label2 = name2->ndata;
	while (l > 0) {
		l--;
		count1 = *label1++;
		count2 = *label2++;

		/* no bitstring support */
		INSIST(count1 <= 63 && count2 <= 63);

		if (count1 != count2) {
			return ((count1 < count2) ? -1 : 1);
		}
		count = count1;
		while (count > 0) {
			count--;
			c1 = maptolower[*label1++];
			c2 = maptolower[*label2++];
			if (c1 < c2) {
				return (-1);
			} else if (c1 > c2) {
				return (1);
			}
		}
	}

	/*
	 * If one name had more labels than the other, their common
	 * prefix must have been different because the shorter name
	 * ended with the root label and the longer one can't have
	 * a root label in the middle of it.  Therefore, if we get
	 * to this point, the lengths must be equal.
	 */
	INSIST(l1 == l2);

	return (0);
}

bool
dns_name_issubdomain(const dns_name_t *name1, const dns_name_t *name2) {
	int order;
	unsigned int nlabels;
	dns_namereln_t namereln;

	/*
	 * Is 'name1' a subdomain of 'name2'?
	 *
	 * Note: It makes no sense for one of the names to be relative and the
	 * other absolute.  If both names are relative, then to be meaningfully
	 * compared the caller must ensure that they are both relative to the
	 * same domain.
	 */

	namereln = dns_name_fullcompare(name1, name2, &order, &nlabels);
	if (namereln == dns_namereln_subdomain ||
	    namereln == dns_namereln_equal)
	{
		return (true);
	}

	return (false);
}

bool
dns_name_matcheswildcard(const dns_name_t *name, const dns_name_t *wname) {
	int order;
	unsigned int nlabels, labels;
	dns_name_t tname;

	REQUIRE(VALID_NAME(name));
	REQUIRE(name->labels > 0);
	REQUIRE(VALID_NAME(wname));
	labels = wname->labels;
	REQUIRE(labels > 0);
	REQUIRE(dns_name_iswildcard(wname));

	DNS_NAME_INIT(&tname, NULL);
	dns_name_getlabelsequence(wname, 1, labels - 1, &tname);
	if (dns_name_fullcompare(name, &tname, &order, &nlabels) ==
	    dns_namereln_subdomain)
	{
		return (true);
	}
	return (false);
}

unsigned int
dns_name_countlabels(const dns_name_t *name) {
	/*
	 * How many labels does 'name' have?
	 */

	REQUIRE(VALID_NAME(name));

	ENSURE(name->labels <= 128);

	return (name->labels);
}

void
dns_name_getlabel(const dns_name_t *name, unsigned int n, dns_label_t *label) {
	unsigned char *offsets;
	dns_offsets_t odata;

	/*
	 * Make 'label' refer to the 'n'th least significant label of 'name'.
	 */

	REQUIRE(VALID_NAME(name));
	REQUIRE(name->labels > 0);
	REQUIRE(n < name->labels);
	REQUIRE(label != NULL);

	SETUP_OFFSETS(name, offsets, odata);

	label->base = &name->ndata[offsets[n]];
	if (n == name->labels - 1) {
		label->length = name->length - offsets[n];
	} else {
		label->length = offsets[n + 1] - offsets[n];
	}
}

void
dns_name_getlabelsequence(const dns_name_t *source, unsigned int first,
			  unsigned int n, dns_name_t *target) {
	unsigned char *p, l;
	unsigned int firstoffset, endoffset;
	unsigned int i;

	/*
	 * Make 'target' refer to the 'n' labels including and following
	 * 'first' in 'source'.
	 */

	REQUIRE(VALID_NAME(source));
	REQUIRE(VALID_NAME(target));
	REQUIRE(first <= source->labels);
	REQUIRE(n <= source->labels - first); /* note first+n could overflow */
	REQUIRE(BINDABLE(target));

	p = source->ndata;
	if (ISC_UNLIKELY(first == source->labels)) {
		firstoffset = source->length;
	} else {
		for (i = 0; i < first; i++) {
			l = *p;
			p += l + 1;
		}
		firstoffset = (unsigned int)(p - source->ndata);
	}

	if (ISC_LIKELY(first + n == source->labels)) {
		endoffset = source->length;
	} else {
		for (i = 0; i < n; i++) {
			l = *p;
			p += l + 1;
		}
		endoffset = (unsigned int)(p - source->ndata);
	}

	target->ndata = &source->ndata[firstoffset];
	target->length = endoffset - firstoffset;

	if (first + n == source->labels && n > 0 &&
	    (source->attributes & DNS_NAMEATTR_ABSOLUTE) != 0)
	{
		target->attributes |= DNS_NAMEATTR_ABSOLUTE;
	} else {
		target->attributes &= ~DNS_NAMEATTR_ABSOLUTE;
	}

	target->labels = n;

	/*
	 * If source and target are the same, and we're making target
	 * a prefix of source, the offsets table is correct already
	 * so we don't need to call set_offsets().
	 */
	if (target->offsets != NULL && (target != source || first != 0)) {
		set_offsets(target, target->offsets, NULL);
	}
}

void
dns_name_clone(const dns_name_t *source, dns_name_t *target) {
	/*
	 * Make 'target' refer to the same name as 'source'.
	 */

	REQUIRE(VALID_NAME(source));
	REQUIRE(VALID_NAME(target));
	REQUIRE(BINDABLE(target));

	target->ndata = source->ndata;
	target->length = source->length;
	target->labels = source->labels;
	target->attributes = source->attributes &
			     (unsigned int)~(DNS_NAMEATTR_READONLY |
					     DNS_NAMEATTR_DYNAMIC |
					     DNS_NAMEATTR_DYNOFFSETS);
	if (target->offsets != NULL && source->labels > 0) {
		if (source->offsets != NULL) {
			memmove(target->offsets, source->offsets,
				source->labels);
		} else {
			set_offsets(target, target->offsets, NULL);
		}
	}
}

void
dns_name_fromregion(dns_name_t *name, const isc_region_t *r) {
	unsigned char *offsets;
	dns_offsets_t odata;
	unsigned int len;
	isc_region_t r2;

	/*
	 * Make 'name' refer to region 'r'.
	 */

	REQUIRE(VALID_NAME(name));
	REQUIRE(r != NULL);
	REQUIRE(BINDABLE(name));

	INIT_OFFSETS(name, offsets, odata);

	if (name->buffer != NULL) {
		isc_buffer_clear(name->buffer);
		isc_buffer_availableregion(name->buffer, &r2);
		len = (r->length < r2.length) ? r->length : r2.length;
		if (len > DNS_NAME_MAXWIRE) {
			len = DNS_NAME_MAXWIRE;
		}
		if (len != 0) {
			memmove(r2.base, r->base, len);
		}
		name->ndata = r2.base;
		name->length = len;
	} else {
		name->ndata = r->base;
		name->length = (r->length <= DNS_NAME_MAXWIRE)
				       ? r->length
				       : DNS_NAME_MAXWIRE;
	}

	if (r->length > 0) {
		set_offsets(name, offsets, name);
	} else {
		name->labels = 0;
		name->attributes &= ~DNS_NAMEATTR_ABSOLUTE;
	}

	if (name->buffer != NULL) {
		isc_buffer_add(name->buffer, name->length);
	}
}

void
dns_name_toregion(const dns_name_t *name, isc_region_t *r) {
	/*
	 * Make 'r' refer to 'name'.
	 */

	REQUIRE(VALID_NAME(name));
	REQUIRE(r != NULL);

	DNS_NAME_TOREGION(name, r);
}

isc_result_t
dns_name_fromtext(dns_name_t *name, isc_buffer_t *source,
		  const dns_name_t *origin, unsigned int options,
		  isc_buffer_t *target) {
	unsigned char *ndata, *label = NULL;
	char *tdata;
	char c;
	ft_state state;
	unsigned int value = 0, count = 0;
	unsigned int n1 = 0, n2 = 0;
	unsigned int tlen, nrem, nused, digits = 0, labels, tused;
	bool done;
	unsigned char *offsets;
	dns_offsets_t odata;
	bool downcase;

	/*
	 * Convert the textual representation of a DNS name at source
	 * into uncompressed wire form stored in target.
	 *
	 * Notes:
	 *	Relative domain names will have 'origin' appended to them
	 *	unless 'origin' is NULL, in which case relative domain names
	 *	will remain relative.
	 */

	REQUIRE(VALID_NAME(name));
	REQUIRE(ISC_BUFFER_VALID(source));
	REQUIRE((target != NULL && ISC_BUFFER_VALID(target)) ||
		(target == NULL && ISC_BUFFER_VALID(name->buffer)));

	downcase = ((options & DNS_NAME_DOWNCASE) != 0);

	if (target == NULL && name->buffer != NULL) {
		target = name->buffer;
		isc_buffer_clear(target);
	}

	REQUIRE(BINDABLE(name));

	INIT_OFFSETS(name, offsets, odata);
	offsets[0] = 0;

	/*
	 * Make 'name' empty in case of failure.
	 */
	MAKE_EMPTY(name);

	/*
	 * Set up the state machine.
	 */
	tdata = (char *)source->base + source->current;
	tlen = isc_buffer_remaininglength(source);
	tused = 0;
	ndata = isc_buffer_used(target);
	nrem = isc_buffer_availablelength(target);
	if (nrem > 255) {
		nrem = 255;
	}
	nused = 0;
	labels = 0;
	done = false;
	state = ft_init;

	while (nrem > 0 && tlen > 0 && !done) {
		c = *tdata++;
		tlen--;
		tused++;

		switch (state) {
		case ft_init:
			/*
			 * Is this the root name?
			 */
			if (c == '.') {
				if (tlen != 0) {
					return (DNS_R_EMPTYLABEL);
				}
				labels++;
				*ndata++ = 0;
				nrem--;
				nused++;
				done = true;
				break;
			}
			if (c == '@' && tlen == 0) {
				state = ft_at;
				break;
			}

			FALLTHROUGH;
		case ft_start:
			label = ndata;
			ndata++;
			nrem--;
			nused++;
			count = 0;
			if (c == '\\') {
				state = ft_initialescape;
				break;
			}
			state = ft_ordinary;
			if (nrem == 0) {
				return (ISC_R_NOSPACE);
			}
			FALLTHROUGH;
		case ft_ordinary:
			if (c == '.') {
				if (count == 0) {
					return (DNS_R_EMPTYLABEL);
				}
				*label = count;
				labels++;
				INSIST(labels <= 127);
				offsets[labels] = nused;
				if (tlen == 0) {
					labels++;
					*ndata++ = 0;
					nrem--;
					nused++;
					done = true;
				}
				state = ft_start;
			} else if (c == '\\') {
				state = ft_escape;
			} else {
				if (count >= 63) {
					return (DNS_R_LABELTOOLONG);
				}
				count++;
				CONVERTTOASCII(c);
				if (downcase) {
					c = maptolower[c & 0xff];
				}
				*ndata++ = c;
				nrem--;
				nused++;
			}
			break;
		case ft_initialescape:
			if (c == '[') {
				/*
				 * This looks like a bitstring label, which
				 * was deprecated.  Intentionally drop it.
				 */
				return (DNS_R_BADLABELTYPE);
			}
			state = ft_escape;
			POST(state);
			FALLTHROUGH;
		case ft_escape:
			if (!isdigit((unsigned char)c)) {
				if (count >= 63) {
					return (DNS_R_LABELTOOLONG);
				}
				count++;
				CONVERTTOASCII(c);
				if (downcase) {
					c = maptolower[c & 0xff];
				}
				*ndata++ = c;
				nrem--;
				nused++;
				state = ft_ordinary;
				break;
			}
			digits = 0;
			value = 0;
			state = ft_escdecimal;
			FALLTHROUGH;
		case ft_escdecimal:
			if (!isdigit((unsigned char)c)) {
				return (DNS_R_BADESCAPE);
			}
			value *= 10;
			value += digitvalue[c & 0xff];
			digits++;
			if (digits == 3) {
				if (value > 255) {
					return (DNS_R_BADESCAPE);
				}
				if (count >= 63) {
					return (DNS_R_LABELTOOLONG);
				}
				count++;
				if (downcase) {
					value = maptolower[value];
				}
				*ndata++ = value;
				nrem--;
				nused++;
				state = ft_ordinary;
			}
			break;
		default:
			FATAL_ERROR(__FILE__, __LINE__, "Unexpected state %d",
				    state);
			/* Does not return. */
		}
	}

	if (!done) {
		if (nrem == 0) {
			return (ISC_R_NOSPACE);
		}
		INSIST(tlen == 0);
		if (state != ft_ordinary && state != ft_at) {
			return (ISC_R_UNEXPECTEDEND);
		}
		if (state == ft_ordinary) {
			INSIST(count != 0);
			INSIST(label != NULL);
			*label = count;
			labels++;
			INSIST(labels <= 127);
			offsets[labels] = nused;
		}
		if (origin != NULL) {
			if (nrem < origin->length) {
				return (ISC_R_NOSPACE);
			}
			label = origin->ndata;
			n1 = origin->length;
			nrem -= n1;
			POST(nrem);
			while (n1 > 0) {
				n2 = *label++;
				INSIST(n2 <= 63); /* no bitstring support */
				*ndata++ = n2;
				n1 -= n2 + 1;
				nused += n2 + 1;
				while (n2 > 0) {
					c = *label++;
					if (downcase) {
						c = maptolower[c & 0xff];
					}
					*ndata++ = c;
					n2--;
				}
				labels++;
				if (n1 > 0) {
					INSIST(labels <= 127);
					offsets[labels] = nused;
				}
			}
			if ((origin->attributes & DNS_NAMEATTR_ABSOLUTE) != 0) {
				name->attributes |= DNS_NAMEATTR_ABSOLUTE;
			}
		}
	} else {
		name->attributes |= DNS_NAMEATTR_ABSOLUTE;
	}

	name->ndata = (unsigned char *)target->base + target->used;
	name->labels = labels;
	name->length = nused;

	isc_buffer_forward(source, tused);
	isc_buffer_add(target, name->length);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_name_totext(const dns_name_t *name, bool omit_final_dot,
		isc_buffer_t *target) {
	unsigned int options = DNS_NAME_MASTERFILE;

	if (omit_final_dot) {
		options |= DNS_NAME_OMITFINALDOT;
	}
	return (dns_name_totext2(name, options, target));
}

isc_result_t
dns_name_toprincipal(const dns_name_t *name, isc_buffer_t *target) {
	return (dns_name_totext2(name, DNS_NAME_OMITFINALDOT, target));
}

isc_result_t
dns_name_totext2(const dns_name_t *name, unsigned int options,
		 isc_buffer_t *target) {
	unsigned char *ndata;
	char *tdata;
	unsigned int nlen, tlen;
	unsigned char c;
	unsigned int trem, count;
	unsigned int labels;
	bool saw_root = false;
	unsigned int oused;
	bool omit_final_dot = ((options & DNS_NAME_OMITFINALDOT) != 0);

	/*
	 * This function assumes the name is in proper uncompressed
	 * wire format.
	 */
	REQUIRE(VALID_NAME(name));
	REQUIRE(ISC_BUFFER_VALID(target));

	oused = target->used;

	ndata = name->ndata;
	nlen = name->length;
	labels = name->labels;
	tdata = isc_buffer_used(target);
	tlen = isc_buffer_availablelength(target);

	trem = tlen;

	if (labels == 0 && nlen == 0) {
		/*
		 * Special handling for an empty name.
		 */
		if (trem == 0) {
			return (ISC_R_NOSPACE);
		}

		/*
		 * The names of these booleans are misleading in this case.
		 * This empty name is not necessarily from the root node of
		 * the DNS root zone, nor is a final dot going to be included.
		 * They need to be set this way, though, to keep the "@"
		 * from being trounced.
		 */
		saw_root = true;
		omit_final_dot = false;
		*tdata++ = '@';
		trem--;

		/*
		 * Skip the while() loop.
		 */
		nlen = 0;
	} else if (nlen == 1 && labels == 1 && *ndata == '\0') {
		/*
		 * Special handling for the root label.
		 */
		if (trem == 0) {
			return (ISC_R_NOSPACE);
		}

		saw_root = true;
		omit_final_dot = false;
		*tdata++ = '.';
		trem--;

		/*
		 * Skip the while() loop.
		 */
		nlen = 0;
	}

	while (labels > 0 && nlen > 0 && trem > 0) {
		labels--;
		count = *ndata++;
		nlen--;
		if (count == 0) {
			saw_root = true;
			break;
		}
		if (count < 64) {
			INSIST(nlen >= count);
			while (count > 0) {
				c = *ndata;
				switch (c) {
				/* Special modifiers in zone files. */
				case 0x40: /* '@' */
				case 0x24: /* '$' */
					if ((options & DNS_NAME_MASTERFILE) ==
					    0)
					{
						goto no_escape;
					}
					FALLTHROUGH;
				case 0x22: /* '"' */
				case 0x28: /* '(' */
				case 0x29: /* ')' */
				case 0x2E: /* '.' */
				case 0x3B: /* ';' */
				case 0x5C: /* '\\' */
					if (trem < 2) {
						return (ISC_R_NOSPACE);
					}
					*tdata++ = '\\';
					CONVERTFROMASCII(c);
					*tdata++ = c;
					ndata++;
					trem -= 2;
					nlen--;
					break;
				no_escape:
				default:
					if (c > 0x20 && c < 0x7f) {
						if (trem == 0) {
							return (ISC_R_NOSPACE);
						}
						CONVERTFROMASCII(c);
						*tdata++ = c;
						ndata++;
						trem--;
						nlen--;
					} else {
						if (trem < 4) {
							return (ISC_R_NOSPACE);
						}
						*tdata++ = 0x5c;
						*tdata++ = 0x30 +
							   ((c / 100) % 10);
						*tdata++ = 0x30 +
							   ((c / 10) % 10);
						*tdata++ = 0x30 + (c % 10);
						trem -= 4;
						ndata++;
						nlen--;
					}
				}
				count--;
			}
		} else {
			FATAL_ERROR(__FILE__, __LINE__,
				    "Unexpected label type %02x", count);
			UNREACHABLE();
		}

		/*
		 * The following assumes names are absolute.  If not, we
		 * fix things up later.  Note that this means that in some
		 * cases one more byte of text buffer is required than is
		 * needed in the final output.
		 */
		if (trem == 0) {
			return (ISC_R_NOSPACE);
		}
		*tdata++ = '.';
		trem--;
	}

	if (nlen != 0 && trem == 0) {
		return (ISC_R_NOSPACE);
	}

	if (!saw_root || omit_final_dot) {
		trem++;
		tdata--;
	}
	if (trem > 0) {
		*tdata = 0;
	}
	isc_buffer_add(target, tlen - trem);

	if (totext_filter_proc != NULL) {
		return ((totext_filter_proc)(target, oused));
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_name_tofilenametext(const dns_name_t *name, bool omit_final_dot,
			isc_buffer_t *target) {
	unsigned char *ndata;
	char *tdata;
	unsigned int nlen, tlen;
	unsigned char c;
	unsigned int trem, count;
	unsigned int labels;

	/*
	 * This function assumes the name is in proper uncompressed
	 * wire format.
	 */
	REQUIRE(VALID_NAME(name));
	REQUIRE((name->attributes & DNS_NAMEATTR_ABSOLUTE) != 0);
	REQUIRE(ISC_BUFFER_VALID(target));

	ndata = name->ndata;
	nlen = name->length;
	labels = name->labels;
	tdata = isc_buffer_used(target);
	tlen = isc_buffer_availablelength(target);

	trem = tlen;

	if (nlen == 1 && labels == 1 && *ndata == '\0') {
		/*
		 * Special handling for the root label.
		 */
		if (trem == 0) {
			return (ISC_R_NOSPACE);
		}

		omit_final_dot = false;
		*tdata++ = '.';
		trem--;

		/*
		 * Skip the while() loop.
		 */
		nlen = 0;
	}

	while (labels > 0 && nlen > 0 && trem > 0) {
		labels--;
		count = *ndata++;
		nlen--;
		if (count == 0) {
			break;
		}
		if (count < 64) {
			INSIST(nlen >= count);
			while (count > 0) {
				c = *ndata;
				if ((c >= 0x30 && c <= 0x39) || /* digit */
				    (c >= 0x41 && c <= 0x5A) || /* uppercase */
				    (c >= 0x61 && c <= 0x7A) || /* lowercase */
				    c == 0x2D ||		/* hyphen */
				    c == 0x5F)			/* underscore */
				{
					if (trem == 0) {
						return (ISC_R_NOSPACE);
					}
					/* downcase */
					if (c >= 0x41 && c <= 0x5A) {
						c += 0x20;
					}
					CONVERTFROMASCII(c);
					*tdata++ = c;
					ndata++;
					trem--;
					nlen--;
				} else {
					if (trem < 4) {
						return (ISC_R_NOSPACE);
					}
					snprintf(tdata, trem, "%%%02X", c);
					tdata += 3;
					trem -= 3;
					ndata++;
					nlen--;
				}
				count--;
			}
		} else {
			FATAL_ERROR(__FILE__, __LINE__,
				    "Unexpected label type %02x", count);
			UNREACHABLE();
		}

		/*
		 * The following assumes names are absolute.  If not, we
		 * fix things up later.  Note that this means that in some
		 * cases one more byte of text buffer is required than is
		 * needed in the final output.
		 */
		if (trem == 0) {
			return (ISC_R_NOSPACE);
		}
		*tdata++ = '.';
		trem--;
	}

	if (nlen != 0 && trem == 0) {
		return (ISC_R_NOSPACE);
	}

	if (omit_final_dot) {
		trem++;
	}

	isc_buffer_add(target, tlen - trem);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_name_downcase(const dns_name_t *source, dns_name_t *name,
		  isc_buffer_t *target) {
	unsigned char *sndata, *ndata;
	unsigned int nlen, count, labels;
	isc_buffer_t buffer;

	/*
	 * Downcase 'source'.
	 */

	REQUIRE(VALID_NAME(source));
	REQUIRE(VALID_NAME(name));
	if (source == name) {
		REQUIRE((name->attributes & DNS_NAMEATTR_READONLY) == 0);
		isc_buffer_init(&buffer, source->ndata, source->length);
		target = &buffer;
		ndata = source->ndata;
	} else {
		REQUIRE(BINDABLE(name));
		REQUIRE((target != NULL && ISC_BUFFER_VALID(target)) ||
			(target == NULL && ISC_BUFFER_VALID(name->buffer)));
		if (target == NULL) {
			target = name->buffer;
			isc_buffer_clear(name->buffer);
		}
		ndata = (unsigned char *)target->base + target->used;
		name->ndata = ndata;
	}

	sndata = source->ndata;
	nlen = source->length;
	labels = source->labels;

	if (nlen > (target->length - target->used)) {
		MAKE_EMPTY(name);
		return (ISC_R_NOSPACE);
	}

	while (labels > 0 && nlen > 0) {
		labels--;
		count = *sndata++;
		*ndata++ = count;
		nlen--;
		if (count < 64) {
			INSIST(nlen >= count);
			while (count > 0) {
				*ndata++ = maptolower[(*sndata++)];
				nlen--;
				count--;
			}
		} else {
			FATAL_ERROR(__FILE__, __LINE__,
				    "Unexpected label type %02x", count);
			/* Does not return. */
		}
	}

	if (source != name) {
		name->labels = source->labels;
		name->length = source->length;
		if ((source->attributes & DNS_NAMEATTR_ABSOLUTE) != 0) {
			name->attributes = DNS_NAMEATTR_ABSOLUTE;
		} else {
			name->attributes = 0;
		}
		if (name->labels > 0 && name->offsets != NULL) {
			set_offsets(name, name->offsets, NULL);
		}
	}

	isc_buffer_add(target, name->length);

	return (ISC_R_SUCCESS);
}

static void
set_offsets(const dns_name_t *name, unsigned char *offsets,
	    dns_name_t *set_name) {
	unsigned int offset, count, length, nlabels;
	unsigned char *ndata;
	bool absolute;

	ndata = name->ndata;
	length = name->length;
	offset = 0;
	nlabels = 0;
	absolute = false;
	while (ISC_LIKELY(offset != length)) {
		INSIST(nlabels < 128);
		offsets[nlabels++] = offset;
		count = *ndata;
		INSIST(count <= 63);
		offset += count + 1;
		ndata += count + 1;
		INSIST(offset <= length);
		if (ISC_UNLIKELY(count == 0)) {
			absolute = true;
			break;
		}
	}
	if (set_name != NULL) {
		INSIST(set_name == name);

		set_name->labels = nlabels;
		set_name->length = offset;
		if (absolute) {
			set_name->attributes |= DNS_NAMEATTR_ABSOLUTE;
		} else {
			set_name->attributes &= ~DNS_NAMEATTR_ABSOLUTE;
		}
	}
	INSIST(nlabels == name->labels);
	INSIST(offset == name->length);
}

isc_result_t
dns_name_fromwire(dns_name_t *name, isc_buffer_t *source,
		  dns_decompress_t *dctx, unsigned int options,
		  isc_buffer_t *target) {
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

	REQUIRE(dctx != NULL);
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
			} else if (c >= 128 && c < 192) {
				/*
				 * 14 bit local compression pointer.
				 * Local compression is no longer an
				 * IETF draft.
				 */
				return (DNS_R_BADLABELTYPE);
			} else if (c >= 192) {
				/*
				 * Ordinary 14-bit pointer.
				 */
				if ((dctx->allowed & DNS_COMPRESS_GLOBAL14) ==
				    0)
				{
					return (DNS_R_DISALLOWED);
				}
				new_current = c & 0x3F;
				state = fw_newcurrent;
			} else {
				return (DNS_R_BADLABELTYPE);
			}
			break;
		case fw_ordinary:
			if (downcase) {
				c = maptolower[c];
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
				return (DNS_R_BADPOINTER);
			}
			biggest_pointer = new_current;
			current = new_current;
			cdata = (unsigned char *)source->base + current;
			seen_pointer = true;
			state = fw_start;
			break;
		default:
			FATAL_ERROR(__FILE__, __LINE__, "Unknown state %d",
				    state);
			/* Does not return. */
		}
	}

	if (!done) {
		return (ISC_R_UNEXPECTEDEND);
	}

	name->ndata = (unsigned char *)target->base + target->used;
	name->labels = labels;
	name->length = nused;
	name->attributes |= DNS_NAMEATTR_ABSOLUTE;

	isc_buffer_forward(source, cused);
	isc_buffer_add(target, name->length);

	return (ISC_R_SUCCESS);

full:
	if (nmax == DNS_NAME_MAXWIRE) {
		/*
		 * The name did not fit even though we had a buffer
		 * big enough to fit a maximum-length name.
		 */
		return (DNS_R_NAMETOOLONG);
	} else {
		/*
		 * The name might fit if only the caller could give us a
		 * big enough buffer.
		 */
		return (ISC_R_NOSPACE);
	}
}

isc_result_t
dns_name_towire(const dns_name_t *name, dns_compress_t *cctx,
		isc_buffer_t *target) {
	return (dns_name_towire2(name, cctx, target, NULL));
}

isc_result_t
dns_name_towire2(const dns_name_t *name, dns_compress_t *cctx,
		 isc_buffer_t *target, uint16_t *comp_offsetp) {
	unsigned int methods;
	uint16_t offset;
	dns_name_t gp; /* Global compression prefix */
	bool gf;       /* Global compression target found */
	uint16_t go;   /* Global compression offset */
	dns_offsets_t clo;
	dns_name_t clname;

	/*
	 * Convert 'name' into wire format, compressing it as specified by the
	 * compression context 'cctx', and storing the result in 'target'.
	 */

	REQUIRE(VALID_NAME(name));
	REQUIRE(cctx != NULL);
	REQUIRE(ISC_BUFFER_VALID(target));

	/*
	 * If this exact name was already rendered before, and the
	 * offset of the previously rendered name is passed to us, write
	 * a compression pointer directly.
	 */
	methods = dns_compress_getmethods(cctx);
	if (comp_offsetp != NULL && *comp_offsetp < 0x4000 &&
	    (name->attributes & DNS_NAMEATTR_NOCOMPRESS) == 0 &&
	    (methods & DNS_COMPRESS_GLOBAL14) != 0)
	{
		if (ISC_UNLIKELY(target->length - target->used < 2)) {
			return (ISC_R_NOSPACE);
		}
		offset = *comp_offsetp;
		offset |= 0xc000;
		isc_buffer_putuint16(target, offset);
		return (ISC_R_SUCCESS);
	}

	/*
	 * If 'name' doesn't have an offsets table, make a clone which
	 * has one.
	 */
	if (name->offsets == NULL) {
		DNS_NAME_INIT(&clname, clo);
		dns_name_clone(name, &clname);
		name = &clname;
	}
	DNS_NAME_INIT(&gp, NULL);

	offset = target->used; /*XXX*/

	if ((name->attributes & DNS_NAMEATTR_NOCOMPRESS) == 0 &&
	    (methods & DNS_COMPRESS_GLOBAL14) != 0)
	{
		gf = dns_compress_findglobal(cctx, name, &gp, &go);
	} else {
		gf = false;
	}

	/*
	 * If the offset is too high for 14 bit global compression, we're
	 * out of luck.
	 */
	if (gf && ISC_UNLIKELY(go >= 0x4000)) {
		gf = false;
	}

	/*
	 * Will the compression pointer reduce the message size?
	 */
	if (gf && (gp.length + 2) >= name->length) {
		gf = false;
	}

	if (gf) {
		if (ISC_UNLIKELY(target->length - target->used < gp.length)) {
			return (ISC_R_NOSPACE);
		}
		if (gp.length != 0) {
			unsigned char *base = target->base;
			(void)memmove(base + target->used, gp.ndata,
				      (size_t)gp.length);
		}
		isc_buffer_add(target, gp.length);
		if (ISC_UNLIKELY(target->length - target->used < 2)) {
			return (ISC_R_NOSPACE);
		}
		isc_buffer_putuint16(target, go | 0xc000);
		if (gp.length != 0) {
			dns_compress_add(cctx, name, &gp, offset);
			if (comp_offsetp != NULL) {
				*comp_offsetp = offset;
			}
		} else if (comp_offsetp != NULL) {
			*comp_offsetp = go;
		}
	} else {
		if (ISC_UNLIKELY(target->length - target->used < name->length))
		{
			return (ISC_R_NOSPACE);
		}
		if (name->length != 0) {
			unsigned char *base = target->base;
			(void)memmove(base + target->used, name->ndata,
				      (size_t)name->length);
		}
		isc_buffer_add(target, name->length);
		dns_compress_add(cctx, name, name, offset);
		if (comp_offsetp != NULL) {
			*comp_offsetp = offset;
		}
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_name_concatenate(const dns_name_t *prefix, const dns_name_t *suffix,
		     dns_name_t *name, isc_buffer_t *target) {
	unsigned char *ndata, *offsets;
	unsigned int nrem, labels, prefix_length, length;
	bool copy_prefix = true;
	bool copy_suffix = true;
	bool absolute = false;
	dns_name_t tmp_name;
	dns_offsets_t odata;

	/*
	 * Concatenate 'prefix' and 'suffix'.
	 */

	REQUIRE(prefix == NULL || VALID_NAME(prefix));
	REQUIRE(suffix == NULL || VALID_NAME(suffix));
	REQUIRE(name == NULL || VALID_NAME(name));
	REQUIRE((target != NULL && ISC_BUFFER_VALID(target)) ||
		(target == NULL && name != NULL &&
		 ISC_BUFFER_VALID(name->buffer)));
	if (prefix == NULL || prefix->labels == 0) {
		copy_prefix = false;
	}
	if (suffix == NULL || suffix->labels == 0) {
		copy_suffix = false;
	}
	if (copy_prefix && (prefix->attributes & DNS_NAMEATTR_ABSOLUTE) != 0) {
		absolute = true;
		REQUIRE(!copy_suffix);
	}
	if (name == NULL) {
		DNS_NAME_INIT(&tmp_name, odata);
		name = &tmp_name;
	}
	if (target == NULL) {
		INSIST(name->buffer != NULL);
		target = name->buffer;
		isc_buffer_clear(name->buffer);
	}

	REQUIRE(BINDABLE(name));

	/*
	 * Set up.
	 */
	nrem = target->length - target->used;
	ndata = (unsigned char *)target->base + target->used;
	if (nrem > DNS_NAME_MAXWIRE) {
		nrem = DNS_NAME_MAXWIRE;
	}
	length = 0;
	prefix_length = 0;
	labels = 0;
	if (copy_prefix) {
		prefix_length = prefix->length;
		length += prefix_length;
		labels += prefix->labels;
	}
	if (copy_suffix) {
		length += suffix->length;
		labels += suffix->labels;
	}
	if (length > DNS_NAME_MAXWIRE) {
		MAKE_EMPTY(name);
		return (DNS_R_NAMETOOLONG);
	}
	if (length > nrem) {
		MAKE_EMPTY(name);
		return (ISC_R_NOSPACE);
	}

	if (copy_suffix) {
		if ((suffix->attributes & DNS_NAMEATTR_ABSOLUTE) != 0) {
			absolute = true;
		}
		memmove(ndata + prefix_length, suffix->ndata, suffix->length);
	}

	/*
	 * If 'prefix' and 'name' are the same object, and the object has
	 * a dedicated buffer, and we're using it, then we don't have to
	 * copy anything.
	 */
	if (copy_prefix && (prefix != name || prefix->buffer != target)) {
		memmove(ndata, prefix->ndata, prefix_length);
	}

	name->ndata = ndata;
	name->labels = labels;
	name->length = length;
	if (absolute) {
		name->attributes = DNS_NAMEATTR_ABSOLUTE;
	} else {
		name->attributes = 0;
	}

	if (name->labels > 0 && name->offsets != NULL) {
		INIT_OFFSETS(name, offsets, odata);
		set_offsets(name, offsets, NULL);
	}

	isc_buffer_add(target, name->length);

	return (ISC_R_SUCCESS);
}

void
dns_name_split(const dns_name_t *name, unsigned int suffixlabels,
	       dns_name_t *prefix, dns_name_t *suffix)

{
	unsigned int splitlabel;

	REQUIRE(VALID_NAME(name));
	REQUIRE(suffixlabels > 0);
	REQUIRE(suffixlabels <= name->labels);
	REQUIRE(prefix != NULL || suffix != NULL);
	REQUIRE(prefix == NULL || (VALID_NAME(prefix) && BINDABLE(prefix)));
	REQUIRE(suffix == NULL || (VALID_NAME(suffix) && BINDABLE(suffix)));

	splitlabel = name->labels - suffixlabels;

	if (prefix != NULL) {
		dns_name_getlabelsequence(name, 0, splitlabel, prefix);
	}

	if (suffix != NULL) {
		dns_name_getlabelsequence(name, splitlabel, suffixlabels,
					  suffix);
	}

	return;
}

void
dns_name_dup(const dns_name_t *source, isc_mem_t *mctx, dns_name_t *target) {
	/*
	 * Make 'target' a dynamically allocated copy of 'source'.
	 */

	REQUIRE(VALID_NAME(source));
	REQUIRE(source->length > 0);
	REQUIRE(VALID_NAME(target));
	REQUIRE(BINDABLE(target));

	/*
	 * Make 'target' empty in case of failure.
	 */
	MAKE_EMPTY(target);

	target->ndata = isc_mem_get(mctx, source->length);

	memmove(target->ndata, source->ndata, source->length);

	target->length = source->length;
	target->labels = source->labels;
	target->attributes = DNS_NAMEATTR_DYNAMIC;
	if ((source->attributes & DNS_NAMEATTR_ABSOLUTE) != 0) {
		target->attributes |= DNS_NAMEATTR_ABSOLUTE;
	}
	if (target->offsets != NULL) {
		if (source->offsets != NULL) {
			memmove(target->offsets, source->offsets,
				source->labels);
		} else {
			set_offsets(target, target->offsets, NULL);
		}
	}
}

isc_result_t
dns_name_dupwithoffsets(const dns_name_t *source, isc_mem_t *mctx,
			dns_name_t *target) {
	/*
	 * Make 'target' a read-only dynamically allocated copy of 'source'.
	 * 'target' will also have a dynamically allocated offsets table.
	 */

	REQUIRE(VALID_NAME(source));
	REQUIRE(source->length > 0);
	REQUIRE(VALID_NAME(target));
	REQUIRE(BINDABLE(target));
	REQUIRE(target->offsets == NULL);

	/*
	 * Make 'target' empty in case of failure.
	 */
	MAKE_EMPTY(target);

	target->ndata = isc_mem_get(mctx, source->length + source->labels);

	memmove(target->ndata, source->ndata, source->length);

	target->length = source->length;
	target->labels = source->labels;
	target->attributes = DNS_NAMEATTR_DYNAMIC | DNS_NAMEATTR_DYNOFFSETS |
			     DNS_NAMEATTR_READONLY;
	if ((source->attributes & DNS_NAMEATTR_ABSOLUTE) != 0) {
		target->attributes |= DNS_NAMEATTR_ABSOLUTE;
	}
	target->offsets = target->ndata + source->length;
	if (source->offsets != NULL) {
		memmove(target->offsets, source->offsets, source->labels);
	} else {
		set_offsets(target, target->offsets, NULL);
	}

	return (ISC_R_SUCCESS);
}

void
dns_name_free(dns_name_t *name, isc_mem_t *mctx) {
	size_t size;

	/*
	 * Free 'name'.
	 */

	REQUIRE(VALID_NAME(name));
	REQUIRE((name->attributes & DNS_NAMEATTR_DYNAMIC) != 0);

	size = name->length;
	if ((name->attributes & DNS_NAMEATTR_DYNOFFSETS) != 0) {
		size += name->labels;
	}
	isc_mem_put(mctx, name->ndata, size);
	dns_name_invalidate(name);
}

isc_result_t
dns_name_digest(const dns_name_t *name, dns_digestfunc_t digest, void *arg) {
	dns_name_t downname;
	unsigned char data[256];
	isc_buffer_t buffer;
	isc_result_t result;
	isc_region_t r;

	/*
	 * Send 'name' in DNSSEC canonical form to 'digest'.
	 */

	REQUIRE(VALID_NAME(name));
	REQUIRE(digest != NULL);

	DNS_NAME_INIT(&downname, NULL);

	isc_buffer_init(&buffer, data, sizeof(data));

	result = dns_name_downcase(name, &downname, &buffer);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	isc_buffer_usedregion(&buffer, &r);

	return ((digest)(arg, &r));
}

bool
dns_name_dynamic(const dns_name_t *name) {
	REQUIRE(VALID_NAME(name));

	/*
	 * Returns whether there is dynamic memory associated with this name.
	 */

	return ((name->attributes & DNS_NAMEATTR_DYNAMIC) != 0 ? true : false);
}

isc_result_t
dns_name_print(const dns_name_t *name, FILE *stream) {
	isc_result_t result;
	isc_buffer_t b;
	isc_region_t r;
	char t[1024];

	/*
	 * Print 'name' on 'stream'.
	 */

	REQUIRE(VALID_NAME(name));

	isc_buffer_init(&b, t, sizeof(t));
	result = dns_name_totext(name, false, &b);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	isc_buffer_usedregion(&b, &r);
	fprintf(stream, "%.*s", (int)r.length, (char *)r.base);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_name_settotextfilter(dns_name_totextfilter_t *proc) {
	/*
	 * If we already have been here set / clear as appropriate.
	 */
	if (totext_filter_proc != NULL && proc != NULL) {
		if (totext_filter_proc == proc) {
			return (ISC_R_SUCCESS);
		}
	}
	if (proc == NULL && totext_filter_proc != NULL) {
		totext_filter_proc = NULL;
		return (ISC_R_SUCCESS);
	}

	totext_filter_proc = proc;

	return (ISC_R_SUCCESS);
}

void
dns_name_format(const dns_name_t *name, char *cp, unsigned int size) {
	isc_result_t result;
	isc_buffer_t buf;

	REQUIRE(size > 0);

	/*
	 * Leave room for null termination after buffer.
	 */
	isc_buffer_init(&buf, cp, size - 1);
	result = dns_name_totext(name, true, &buf);
	if (result == ISC_R_SUCCESS) {
		isc_buffer_putuint8(&buf, (uint8_t)'\0');
	} else {
		snprintf(cp, size, "<unknown>");
	}
}

/*
 * dns_name_tostring() -- similar to dns_name_format() but allocates its own
 * memory.
 */
isc_result_t
dns_name_tostring(const dns_name_t *name, char **target, isc_mem_t *mctx) {
	isc_result_t result;
	isc_buffer_t buf;
	isc_region_t reg;
	char *p, txt[DNS_NAME_FORMATSIZE];

	REQUIRE(VALID_NAME(name));
	REQUIRE(target != NULL && *target == NULL);

	isc_buffer_init(&buf, txt, sizeof(txt));
	result = dns_name_totext(name, false, &buf);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	isc_buffer_usedregion(&buf, &reg);
	p = isc_mem_allocate(mctx, reg.length + 1);
	memmove(p, (char *)reg.base, (int)reg.length);
	p[reg.length] = '\0';

	*target = p;
	return (ISC_R_SUCCESS);
}

/*
 * dns_name_fromstring() -- convert directly from a string to a name,
 * allocating memory as needed
 */
isc_result_t
dns_name_fromstring(dns_name_t *target, const char *src, unsigned int options,
		    isc_mem_t *mctx) {
	return (dns_name_fromstring2(target, src, dns_rootname, options, mctx));
}

isc_result_t
dns_name_fromstring2(dns_name_t *target, const char *src,
		     const dns_name_t *origin, unsigned int options,
		     isc_mem_t *mctx) {
	isc_result_t result;
	isc_buffer_t buf;
	dns_fixedname_t fn;
	dns_name_t *name;

	REQUIRE(src != NULL);

	isc_buffer_constinit(&buf, src, strlen(src));
	isc_buffer_add(&buf, strlen(src));
	if (BINDABLE(target) && target->buffer != NULL) {
		name = target;
	} else {
		name = dns_fixedname_initname(&fn);
	}

	result = dns_name_fromtext(name, &buf, origin, options, NULL);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	if (name != target) {
		result = dns_name_dupwithoffsets(name, mctx, target);
	}
	return (result);
}

static isc_result_t
name_copy(const dns_name_t *source, dns_name_t *dest, isc_buffer_t *target) {
	unsigned char *ndata = NULL;

	/*
	 * Make dest a copy of source.
	 */

	REQUIRE(BINDABLE(dest));

	/*
	 * Set up.
	 */
	if (target->length - target->used < source->length) {
		return (ISC_R_NOSPACE);
	}

	ndata = (unsigned char *)target->base + target->used;
	dest->ndata = target->base;

	if (source->length != 0) {
		memmove(ndata, source->ndata, source->length);
	}

	dest->ndata = ndata;
	dest->labels = source->labels;
	dest->length = source->length;
	if ((source->attributes & DNS_NAMEATTR_ABSOLUTE) != 0) {
		dest->attributes = DNS_NAMEATTR_ABSOLUTE;
	} else {
		dest->attributes = 0;
	}

	if (dest->labels > 0 && dest->offsets != NULL) {
		if (source->offsets != NULL && source->labels != 0) {
			memmove(dest->offsets, source->offsets, source->labels);
		} else {
			set_offsets(dest, dest->offsets, NULL);
		}
	}

	isc_buffer_add(target, dest->length);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_name_copy(const dns_name_t *source, dns_name_t *dest,
	      isc_buffer_t *target) {
	REQUIRE(VALID_NAME(source));
	REQUIRE(VALID_NAME(dest));
	REQUIRE(target != NULL);

	return (name_copy(source, dest, target));
}

void
dns_name_copynf(const dns_name_t *source, dns_name_t *dest) {
	REQUIRE(VALID_NAME(source));
	REQUIRE(VALID_NAME(dest));
	REQUIRE(dest->buffer != NULL);

	isc_buffer_clear(dest->buffer);
	RUNTIME_CHECK(name_copy(source, dest, dest->buffer) == ISC_R_SUCCESS);
}

/*
 * Service Discovery Prefixes RFC 6763.
 */
static unsigned char b_dns_sd_udp_data[] = "\001b\007_dns-sd\004_udp";
static unsigned char b_dns_sd_udp_offsets[] = { 0, 2, 10 };
static unsigned char db_dns_sd_udp_data[] = "\002db\007_dns-sd\004_udp";
static unsigned char db_dns_sd_udp_offsets[] = { 0, 3, 11 };
static unsigned char r_dns_sd_udp_data[] = "\001r\007_dns-sd\004_udp";
static unsigned char r_dns_sd_udp_offsets[] = { 0, 2, 10 };
static unsigned char dr_dns_sd_udp_data[] = "\002dr\007_dns-sd\004_udp";
static unsigned char dr_dns_sd_udp_offsets[] = { 0, 3, 11 };
static unsigned char lb_dns_sd_udp_data[] = "\002lb\007_dns-sd\004_udp";
static unsigned char lb_dns_sd_udp_offsets[] = { 0, 3, 11 };

static dns_name_t const dns_sd[] = {
	DNS_NAME_INITNONABSOLUTE(b_dns_sd_udp_data, b_dns_sd_udp_offsets),
	DNS_NAME_INITNONABSOLUTE(db_dns_sd_udp_data, db_dns_sd_udp_offsets),
	DNS_NAME_INITNONABSOLUTE(r_dns_sd_udp_data, r_dns_sd_udp_offsets),
	DNS_NAME_INITNONABSOLUTE(dr_dns_sd_udp_data, dr_dns_sd_udp_offsets),
	DNS_NAME_INITNONABSOLUTE(lb_dns_sd_udp_data, lb_dns_sd_udp_offsets)
};

bool
dns_name_isdnssd(const dns_name_t *name) {
	size_t i;
	dns_name_t prefix;

	if (dns_name_countlabels(name) > 3U) {
		dns_name_init(&prefix, NULL);
		dns_name_getlabelsequence(name, 0, 3, &prefix);
		for (i = 0; i < (sizeof(dns_sd) / sizeof(dns_sd[0])); i++) {
			if (dns_name_equal(&prefix, &dns_sd[i])) {
				return (true);
			}
		}
	}

	return (false);
}

static unsigned char inaddr10_offsets[] = { 0, 3, 11, 16 };
static unsigned char inaddr172_offsets[] = { 0, 3, 7, 15, 20 };
static unsigned char inaddr192_offsets[] = { 0, 4, 8, 16, 21 };

static unsigned char inaddr10[] = "\00210\007IN-ADDR\004ARPA";

static unsigned char inaddr16172[] = "\00216\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr17172[] = "\00217\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr18172[] = "\00218\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr19172[] = "\00219\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr20172[] = "\00220\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr21172[] = "\00221\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr22172[] = "\00222\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr23172[] = "\00223\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr24172[] = "\00224\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr25172[] = "\00225\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr26172[] = "\00226\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr27172[] = "\00227\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr28172[] = "\00228\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr29172[] = "\00229\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr30172[] = "\00230\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr31172[] = "\00231\003172\007IN-ADDR\004ARPA";

static unsigned char inaddr168192[] = "\003168\003192\007IN-ADDR\004ARPA";

static dns_name_t const rfc1918names[] = {
	DNS_NAME_INITABSOLUTE(inaddr10, inaddr10_offsets),
	DNS_NAME_INITABSOLUTE(inaddr16172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr17172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr18172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr19172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr20172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr21172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr22172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr23172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr24172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr25172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr26172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr27172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr28172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr29172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr30172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr31172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr168192, inaddr192_offsets)
};

bool
dns_name_isrfc1918(const dns_name_t *name) {
	size_t i;

	for (i = 0; i < (sizeof(rfc1918names) / sizeof(*rfc1918names)); i++) {
		if (dns_name_issubdomain(name, &rfc1918names[i])) {
			return (true);
		}
	}
	return (false);
}

static unsigned char ulaoffsets[] = { 0, 2, 4, 8, 13 };
static unsigned char ip6fc[] = "\001c\001f\003ip6\004ARPA";
static unsigned char ip6fd[] = "\001d\001f\003ip6\004ARPA";

static dns_name_t const ulanames[] = { DNS_NAME_INITABSOLUTE(ip6fc, ulaoffsets),
				       DNS_NAME_INITABSOLUTE(ip6fd,
							     ulaoffsets) };

bool
dns_name_isula(const dns_name_t *name) {
	size_t i;

	for (i = 0; i < (sizeof(ulanames) / sizeof(*ulanames)); i++) {
		if (dns_name_issubdomain(name, &ulanames[i])) {
			return (true);
		}
	}
	return (false);
}

/*
 * Use a simple table as we don't want all the locale stuff
 * associated with ishexdigit().
 */
const char ishex[256] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			  0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			  0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

bool
dns_name_istat(const dns_name_t *name) {
	unsigned char len;
	const unsigned char *ndata;

	REQUIRE(VALID_NAME(name));

	if (name->labels < 1) {
		return (false);
	}

	ndata = name->ndata;
	len = ndata[0];
	INSIST(len <= name->length);
	ndata++;

	/*
	 * Is there at least one trust anchor reported and is the
	 * label length consistent with a trust-anchor-telemetry label.
	 */
	if ((len < 8) || (len - 3) % 5 != 0) {
		return (false);
	}

	if (ndata[0] != '_' || maptolower[ndata[1]] != 't' ||
	    maptolower[ndata[2]] != 'a')
	{
		return (false);
	}
	ndata += 3;
	len -= 3;

	while (len > 0) {
		INSIST(len >= 5);
		if (ndata[0] != '-' || !ishex[ndata[1]] || !ishex[ndata[2]] ||
		    !ishex[ndata[3]] || !ishex[ndata[4]])
		{
			return (false);
		}
		ndata += 5;
		len -= 5;
	}
	return (true);
}
