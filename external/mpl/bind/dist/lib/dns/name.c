/*	$NetBSD: name.c,v 1.15 2025/01/26 16:25:23 christos Exp $	*/

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

#include <isc/ascii.h>
#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/hex.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/thread.h>
#include <isc/util.h>

#include <dns/compress.h>
#include <dns/fixedname.h>
#include <dns/name.h>

typedef enum {
	ft_init = 0,
	ft_start,
	ft_ordinary,
	ft_initialescape,
	ft_escape,
	ft_escdecimal,
	ft_at
} ft_state;

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
#define MAKE_EMPTY(name)                           \
	do {                                       \
		name->ndata = NULL;                \
		name->length = 0;                  \
		name->labels = 0;                  \
		name->attributes.absolute = false; \
	} while (0);

/*%
 * Note that the name data must be a char array, not a string
 * literal, to avoid compiler warnings about discarding
 * the const attribute of a string.
 */
static unsigned char root_ndata[] = { "" };
static unsigned char root_offsets[] = { 0 };

static dns_name_t root = DNS_NAME_INITABSOLUTE(root_ndata, root_offsets);
const dns_name_t *dns_rootname = &root;

static unsigned char wild_ndata[] = { "\001*" };
static unsigned char wild_offsets[] = { 0 };

static dns_name_t const wild = DNS_NAME_INITNONABSOLUTE(wild_ndata,
							wild_offsets);

const dns_name_t *dns_wildcardname = &wild;

/*
 * dns_name_t to text post-conversion procedure.
 */
static thread_local dns_name_totextfilter_t *totext_filter_proc = NULL;

static void
set_offsets(const dns_name_t *name, unsigned char *offsets,
	    dns_name_t *set_name);

bool
dns_name_isvalid(const dns_name_t *name) {
	unsigned char *ndata, *offsets;
	unsigned int offset, count, length, nlabels;

	if (!DNS_NAME_VALID(name)) {
		return false;
	}

	if (name->length > DNS_NAME_MAXWIRE ||
	    name->labels > DNS_NAME_MAXLABELS)
	{
		return false;
	}

	ndata = name->ndata;
	length = name->length;
	offsets = name->offsets;
	offset = 0;
	nlabels = 0;

	while (offset != length) {
		count = *ndata;
		if (count > DNS_NAME_LABELLEN) {
			return false;
		}
		if (offsets != NULL && offsets[nlabels] != offset) {
			return false;
		}

		nlabels++;
		offset += count + 1;
		ndata += count + 1;
		if (offset > length) {
			return false;
		}

		if (count == 0) {
			break;
		}
	}

	if (nlabels != name->labels || offset != name->length) {
		return false;
	}

	return true;
}

bool
dns_name_hasbuffer(const dns_name_t *name) {
	/*
	 * Does 'name' have a dedicated buffer?
	 */

	REQUIRE(DNS_NAME_VALID(name));

	if (name->buffer != NULL) {
		return true;
	}

	return false;
}

bool
dns_name_isabsolute(const dns_name_t *name) {
	/*
	 * Does 'name' end in the root label?
	 */

	REQUIRE(DNS_NAME_VALID(name));

	return name->attributes.absolute;
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

	REQUIRE(DNS_NAME_VALID(name));
	REQUIRE(name->labels > 0);
	REQUIRE(name->attributes.absolute);

	/*
	 * Root label.
	 */
	if (name->length == 1) {
		return true;
	}

	ndata = name->ndata;
	n = *ndata++;
	INSIST(n <= DNS_NAME_LABELLEN);
	while (n--) {
		ch = *ndata++;
		if (!domainchar(ch)) {
			return false;
		}
	}

	if (ndata == name->ndata + name->length) {
		return false;
	}

	/*
	 * RFC952/RFC1123 hostname.
	 */
	while (ndata < (name->ndata + name->length)) {
		n = *ndata++;
		INSIST(n <= DNS_NAME_LABELLEN);
		first = true;
		while (n--) {
			ch = *ndata++;
			if (first || n == 0) {
				if (!borderchar(ch)) {
					return false;
				}
			} else {
				if (!middlechar(ch)) {
					return false;
				}
			}
			first = false;
		}
	}
	return true;
}

bool
dns_name_ishostname(const dns_name_t *name, bool wildcard) {
	unsigned char *ndata, ch;
	unsigned int n;
	bool first;

	REQUIRE(DNS_NAME_VALID(name));
	REQUIRE(name->labels > 0);
	REQUIRE(name->attributes.absolute);

	/*
	 * Root label.
	 */
	if (name->length == 1) {
		return true;
	}

	/*
	 * Skip wildcard if this is a ownername.
	 */
	ndata = name->ndata;
	if (wildcard && ndata[0] == 1 && ndata[1] == '*') {
		ndata += 2;
	}

	/*
	 * RFC952/RFC1123 hostname.
	 */
	while (ndata < (name->ndata + name->length)) {
		n = *ndata++;
		INSIST(n <= DNS_NAME_LABELLEN);
		first = true;
		while (n--) {
			ch = *ndata++;
			if (first || n == 0) {
				if (!borderchar(ch)) {
					return false;
				}
			} else {
				if (!middlechar(ch)) {
					return false;
				}
			}
			first = false;
		}
	}
	return true;
}

bool
dns_name_iswildcard(const dns_name_t *name) {
	unsigned char *ndata;

	/*
	 * Is 'name' a wildcard name?
	 */

	REQUIRE(DNS_NAME_VALID(name));
	REQUIRE(name->labels > 0);

	if (name->length >= 2) {
		ndata = name->ndata;
		if (ndata[0] == 1 && ndata[1] == '*') {
			return true;
		}
	}

	return false;
}

bool
dns_name_internalwildcard(const dns_name_t *name) {
	unsigned char *ndata;
	unsigned int count;
	unsigned int label;

	/*
	 * Does 'name' contain a internal wildcard?
	 */

	REQUIRE(DNS_NAME_VALID(name));
	REQUIRE(name->labels > 0);

	/*
	 * Skip first label.
	 */
	ndata = name->ndata;
	count = *ndata++;
	INSIST(count <= DNS_NAME_LABELLEN);
	ndata += count;
	label = 1;
	/*
	 * Check all but the last of the remaining labels.
	 */
	while (label + 1 < name->labels) {
		count = *ndata++;
		INSIST(count <= DNS_NAME_LABELLEN);
		if (count == 1 && *ndata == '*') {
			return true;
		}
		ndata += count;
		label++;
	}
	return false;
}

uint32_t
dns_name_hash(const dns_name_t *name) {
	REQUIRE(DNS_NAME_VALID(name));

	return isc_hash32(name->ndata, name->length, false);
}

dns_namereln_t
dns_name_fullcompare(const dns_name_t *name1, const dns_name_t *name2,
		     int *orderp, unsigned int *nlabelsp) {
	unsigned int l1, l2, l, count1, count2, count, nlabels;
	int cdiff, ldiff, diff;
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

	REQUIRE(DNS_NAME_VALID(name1));
	REQUIRE(DNS_NAME_VALID(name2));
	REQUIRE(orderp != NULL);
	REQUIRE(nlabelsp != NULL);
	/*
	 * Either name1 is absolute and name2 is absolute, or neither is.
	 */
	REQUIRE((name1->attributes.absolute) == (name2->attributes.absolute));

	if (name1 == name2) {
		*orderp = 0;
		*nlabelsp = name1->labels;
		return dns_namereln_equal;
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

	while (l-- > 0) {
		offsets1--;
		offsets2--;
		label1 = &name1->ndata[*offsets1];
		label2 = &name2->ndata[*offsets2];
		count1 = *label1++;
		count2 = *label2++;

		cdiff = (int)count1 - (int)count2;
		if (cdiff < 0) {
			count = count1;
		} else {
			count = count2;
		}

		diff = isc_ascii_lowercmp(label1, label2, count);
		if (diff != 0) {
			*orderp = diff;
			goto done;
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
	return namereln;

done:
	*nlabelsp = nlabels;
	if (nlabels > 0) {
		namereln = dns_namereln_commonancestor;
	}

	return namereln;
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

	return order;
}

bool
dns_name_equal(const dns_name_t *name1, const dns_name_t *name2) {
	unsigned int length;

	/*
	 * Are 'name1' and 'name2' equal?
	 *
	 * Note: It makes no sense for one of the names to be relative and the
	 * other absolute.  If both names are relative, then to be meaningfully
	 * compared the caller must ensure that they are both relative to the
	 * same domain.
	 */

	REQUIRE(DNS_NAME_VALID(name1));
	REQUIRE(DNS_NAME_VALID(name2));
	/*
	 * Either name1 is absolute and name2 is absolute, or neither is.
	 */
	REQUIRE((name1->attributes.absolute) == (name2->attributes.absolute));

	if (name1 == name2) {
		return true;
	}

	length = name1->length;
	if (length != name2->length) {
		return false;
	}

	/* label lengths are < 64 so tolower() does not affect them */
	return isc_ascii_lowerequal(name1->ndata, name2->ndata, length);
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

	REQUIRE(DNS_NAME_VALID(name1));
	REQUIRE(DNS_NAME_VALID(name2));
	/*
	 * Either name1 is absolute and name2 is absolute, or neither is.
	 */
	REQUIRE((name1->attributes.absolute) == (name2->attributes.absolute));

	if (name1->length != name2->length) {
		return false;
	}

	if (memcmp(name1->ndata, name2->ndata, name1->length) != 0) {
		return false;
	}

	return true;
}

int
dns_name_rdatacompare(const dns_name_t *name1, const dns_name_t *name2) {
	/*
	 * Compare two absolute names as rdata.
	 */

	REQUIRE(DNS_NAME_VALID(name1));
	REQUIRE(name1->labels > 0);
	REQUIRE(name1->attributes.absolute);
	REQUIRE(DNS_NAME_VALID(name2));
	REQUIRE(name2->labels > 0);
	REQUIRE(name2->attributes.absolute);

	/* label lengths are < 64 so tolower() does not affect them */
	return isc_ascii_lowercmp(name1->ndata, name2->ndata,
				  ISC_MIN(name1->length, name2->length));
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
		return true;
	}

	return false;
}

bool
dns_name_matcheswildcard(const dns_name_t *name, const dns_name_t *wname) {
	int order;
	unsigned int nlabels, labels;
	dns_name_t tname;

	REQUIRE(DNS_NAME_VALID(name));
	REQUIRE(name->labels > 0);
	REQUIRE(DNS_NAME_VALID(wname));
	labels = wname->labels;
	REQUIRE(labels > 0);
	REQUIRE(dns_name_iswildcard(wname));

	dns_name_init(&tname, NULL);
	dns_name_getlabelsequence(wname, 1, labels - 1, &tname);
	if (dns_name_fullcompare(name, &tname, &order, &nlabels) ==
	    dns_namereln_subdomain)
	{
		return true;
	}
	return false;
}

void
dns_name_getlabel(const dns_name_t *name, unsigned int n, dns_label_t *label) {
	unsigned char *offsets;
	dns_offsets_t odata;

	/*
	 * Make 'label' refer to the 'n'th least significant label of 'name'.
	 */

	REQUIRE(DNS_NAME_VALID(name));
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

	REQUIRE(DNS_NAME_VALID(source));
	REQUIRE(DNS_NAME_VALID(target));
	REQUIRE(first <= source->labels);
	REQUIRE(n <= source->labels - first); /* note first+n could overflow */
	REQUIRE(DNS_NAME_BINDABLE(target));

	p = source->ndata;
	if (first == source->labels) {
		firstoffset = source->length;
	} else {
		for (i = 0; i < first; i++) {
			l = *p;
			p += l + 1;
		}
		firstoffset = (unsigned int)(p - source->ndata);
	}

	if (first + n == source->labels) {
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

	if (first + n == source->labels && n > 0 && source->attributes.absolute)
	{
		target->attributes.absolute = true;
	} else {
		target->attributes.absolute = false;
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

	REQUIRE(DNS_NAME_VALID(source));
	REQUIRE(DNS_NAME_VALID(target));
	REQUIRE(DNS_NAME_BINDABLE(target));

	target->ndata = source->ndata;
	target->length = source->length;
	target->labels = source->labels;
	target->attributes = source->attributes;
	target->attributes.readonly = false;
	target->attributes.dynamic = false;
	target->attributes.dynoffsets = false;
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
	isc_region_t r2 = { .base = NULL, .length = 0 };

	/*
	 * Make 'name' refer to region 'r'.
	 */

	REQUIRE(DNS_NAME_VALID(name));
	REQUIRE(r != NULL);
	REQUIRE(DNS_NAME_BINDABLE(name));

	INIT_OFFSETS(name, offsets, odata);

	name->ndata = r->base;
	if (name->buffer != NULL) {
		isc_buffer_clear(name->buffer);
		isc_buffer_availableregion(name->buffer, &r2);
		len = (r->length < r2.length) ? r->length : r2.length;
		if (len > DNS_NAME_MAXWIRE) {
			len = DNS_NAME_MAXWIRE;
		}
		name->length = len;
	} else {
		name->length = (r->length <= DNS_NAME_MAXWIRE)
				       ? r->length
				       : DNS_NAME_MAXWIRE;
	}

	if (r->length > 0) {
		set_offsets(name, offsets, name);
	} else {
		name->labels = 0;
		name->attributes.absolute = false;
	}

	if (name->buffer != NULL) {
		/*
		 * name->length has been updated by set_offsets to the actual
		 * length of the name data so we can now copy the actual name
		 * data and not anything after it.
		 */
		if (name->length > 0) {
			memmove(r2.base, r->base, name->length);
		}
		name->ndata = r2.base;
		isc_buffer_add(name->buffer, name->length);
	}
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

	REQUIRE(DNS_NAME_VALID(name));
	REQUIRE(ISC_BUFFER_VALID(source));
	REQUIRE((target != NULL && ISC_BUFFER_VALID(target)) ||
		(target == NULL && ISC_BUFFER_VALID(name->buffer)));

	downcase = ((options & DNS_NAME_DOWNCASE) != 0);

	if (target == NULL && name->buffer != NULL) {
		target = name->buffer;
		isc_buffer_clear(target);
	}

	REQUIRE(DNS_NAME_BINDABLE(name));

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
	if (nrem > DNS_NAME_MAXWIRE) {
		nrem = DNS_NAME_MAXWIRE;
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
					return DNS_R_EMPTYLABEL;
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
				return ISC_R_NOSPACE;
			}
			FALLTHROUGH;
		case ft_ordinary:
			if (c == '.') {
				if (count == 0) {
					return DNS_R_EMPTYLABEL;
				}
				*label = count;
				labels++;
				INSIST(labels < DNS_NAME_MAXLABELS);
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
				if (count >= DNS_NAME_LABELLEN) {
					return DNS_R_LABELTOOLONG;
				}
				count++;
				if (downcase) {
					c = isc_ascii_tolower(c);
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
				return DNS_R_BADLABELTYPE;
			}
			state = ft_escape;
			POST(state);
			FALLTHROUGH;
		case ft_escape:
			if (!isdigit((unsigned char)c)) {
				if (count >= DNS_NAME_LABELLEN) {
					return DNS_R_LABELTOOLONG;
				}
				count++;
				if (downcase) {
					c = isc_ascii_tolower(c);
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
				return DNS_R_BADESCAPE;
			}
			value = 10 * value + c - '0';
			digits++;
			if (digits == 3) {
				if (value > 255) {
					return DNS_R_BADESCAPE;
				}
				if (count >= DNS_NAME_LABELLEN) {
					return DNS_R_LABELTOOLONG;
				}
				count++;
				if (downcase) {
					value = isc_ascii_tolower(value);
				}
				*ndata++ = value;
				nrem--;
				nused++;
				state = ft_ordinary;
			}
			break;
		default:
			FATAL_ERROR("Unexpected state %d", state);
			/* Does not return. */
		}
	}

	if (!done) {
		if (nrem == 0) {
			return ISC_R_NOSPACE;
		}
		INSIST(tlen == 0);
		if (state != ft_ordinary && state != ft_at) {
			return ISC_R_UNEXPECTEDEND;
		}
		if (state == ft_ordinary) {
			INSIST(count != 0);
			INSIST(label != NULL);
			*label = count;
			labels++;
			INSIST(labels < DNS_NAME_MAXLABELS);
			offsets[labels] = nused;
		}
		if (origin != NULL) {
			if (nrem < origin->length) {
				return ISC_R_NOSPACE;
			}
			label = origin->ndata;
			n1 = origin->length;
			nrem -= n1;
			POST(nrem);
			while (n1 > 0) {
				n2 = *label++;
				INSIST(n2 <= DNS_NAME_LABELLEN);
				*ndata++ = n2;
				n1 -= n2 + 1;
				nused += n2 + 1;
				while (n2 > 0) {
					c = *label++;
					if (downcase) {
						c = isc_ascii_tolower(c);
					}
					*ndata++ = c;
					n2--;
				}
				labels++;
				if (n1 > 0) {
					INSIST(labels < DNS_NAME_MAXLABELS);
					offsets[labels] = nused;
				}
			}
			if (origin->attributes.absolute) {
				name->attributes.absolute = true;
			}
		}
	} else {
		name->attributes.absolute = true;
	}

	name->ndata = (unsigned char *)target->base + target->used;
	name->labels = labels;
	name->length = nused;

	isc_buffer_forward(source, tused);
	isc_buffer_add(target, name->length);

	return ISC_R_SUCCESS;
}

isc_result_t
dns_name_totext(const dns_name_t *name, unsigned int options,
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
	REQUIRE(DNS_NAME_VALID(name));
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
			return ISC_R_NOSPACE;
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
			return ISC_R_NOSPACE;
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
		if (count <= DNS_NAME_LABELLEN) {
			INSIST(nlen >= count);
			while (count > 0) {
				c = *ndata;
				switch (c) {
				/* Special modifiers in zone files. */
				case 0x40: /* '@' */
				case 0x24: /* '$' */
					if ((options & DNS_NAME_PRINCIPAL) != 0)
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
						return ISC_R_NOSPACE;
					}
					*tdata++ = '\\';
					*tdata++ = c;
					ndata++;
					trem -= 2;
					nlen--;
					break;
				no_escape:
				default:
					if (c > 0x20 && c < 0x7f) {
						if (trem == 0) {
							return ISC_R_NOSPACE;
						}
						*tdata++ = c;
						ndata++;
						trem--;
						nlen--;
					} else {
						if (trem < 4) {
							return ISC_R_NOSPACE;
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
			FATAL_ERROR("Unexpected label type %02x", count);
			UNREACHABLE();
		}

		/*
		 * The following assumes names are absolute.  If not, we
		 * fix things up later.  Note that this means that in some
		 * cases one more byte of text buffer is required than is
		 * needed in the final output.
		 */
		if (trem == 0) {
			return ISC_R_NOSPACE;
		}
		*tdata++ = '.';
		trem--;
	}

	if (nlen != 0 && trem == 0) {
		return ISC_R_NOSPACE;
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
		return (totext_filter_proc)(target, oused);
	}

	return ISC_R_SUCCESS;
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
	REQUIRE(DNS_NAME_VALID(name));
	REQUIRE(name->attributes.absolute);
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
			return ISC_R_NOSPACE;
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
		if (count <= DNS_NAME_LABELLEN) {
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
						return ISC_R_NOSPACE;
					}
					/* downcase */
					if (c >= 0x41 && c <= 0x5A) {
						c += 0x20;
					}
					*tdata++ = c;
					ndata++;
					trem--;
					nlen--;
				} else {
					if (trem < 4) {
						return ISC_R_NOSPACE;
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
			FATAL_ERROR("Unexpected label type %02x", count);
			UNREACHABLE();
		}

		/*
		 * The following assumes names are absolute.  If not, we
		 * fix things up later.  Note that this means that in some
		 * cases one more byte of text buffer is required than is
		 * needed in the final output.
		 */
		if (trem == 0) {
			return ISC_R_NOSPACE;
		}
		*tdata++ = '.';
		trem--;
	}

	if (nlen != 0 && trem == 0) {
		return ISC_R_NOSPACE;
	}

	if (omit_final_dot) {
		trem++;
	}

	isc_buffer_add(target, tlen - trem);

	return ISC_R_SUCCESS;
}

isc_result_t
dns_name_downcase(const dns_name_t *source, dns_name_t *name,
		  isc_buffer_t *target) {
	unsigned char *ndata;
	isc_buffer_t buffer;

	/*
	 * Downcase 'source'.
	 */

	REQUIRE(DNS_NAME_VALID(source));
	REQUIRE(DNS_NAME_VALID(name));
	if (source == name) {
		REQUIRE(!name->attributes.readonly);
		isc_buffer_init(&buffer, source->ndata, source->length);
		target = &buffer;
		ndata = source->ndata;
	} else {
		REQUIRE(DNS_NAME_BINDABLE(name));
		REQUIRE((target != NULL && ISC_BUFFER_VALID(target)) ||
			(target == NULL && ISC_BUFFER_VALID(name->buffer)));
		if (target == NULL) {
			target = name->buffer;
			isc_buffer_clear(name->buffer);
		}
		ndata = (unsigned char *)target->base + target->used;
		name->ndata = ndata;
	}

	if (source->length > (target->length - target->used)) {
		MAKE_EMPTY(name);
		return ISC_R_NOSPACE;
	}

	/* label lengths are < 64 so tolower() does not affect them */
	isc_ascii_lowercopy(ndata, source->ndata, source->length);

	if (source != name) {
		name->labels = source->labels;
		name->length = source->length;
		name->attributes = (struct dns_name_attrs){
			.absolute = source->attributes.absolute
		};
		if (name->labels > 0 && name->offsets != NULL) {
			set_offsets(name, name->offsets, NULL);
		}
	}

	isc_buffer_add(target, name->length);

	return ISC_R_SUCCESS;
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
	while (offset != length) {
		INSIST(nlabels < DNS_NAME_MAXLABELS);
		offsets[nlabels++] = offset;
		count = *ndata;
		INSIST(count <= DNS_NAME_LABELLEN);
		offset += count + 1;
		ndata += count + 1;
		INSIST(offset <= length);
		if (count == 0) {
			absolute = true;
			break;
		}
	}
	if (set_name != NULL) {
		INSIST(set_name == name);

		set_name->labels = nlabels;
		set_name->length = offset;
		set_name->attributes.absolute = absolute;
	}
	INSIST(nlabels == name->labels);
	INSIST(offset == name->length);
}

isc_result_t
dns_name_fromwire(dns_name_t *const name, isc_buffer_t *const source,
		  const dns_decompress_t dctx, isc_buffer_t *target) {
	/*
	 * Copy the name at source into target, decompressing it.
	 *
	 *	*** WARNING ***
	 *
	 * dns_name_fromwire() deals with raw network data. An error in this
	 * routine could result in the failure or hijacking of the server.
	 *
	 * The description of name compression in RFC 1035 section 4.1.4 is
	 * subtle wrt certain edge cases. The first important sentence is:
	 *
	 * > In this scheme, an entire domain name or a list of labels at the
	 * > end of a domain name is replaced with a pointer to a prior
	 * > occurance of the same name.
	 *
	 * The key word is "prior". This says that compression pointers must
	 * point strictly earlier in the message (before our "marker" variable),
	 * which is enough to prevent DoS attacks due to compression loops.
	 *
	 * The next important sentence is:
	 *
	 * > If a domain name is contained in a part of the message subject to a
	 * > length field (such as the RDATA section of an RR), and compression
	 * > is used, the length of the compressed name is used in the length
	 * > calculation, rather than the length of the expanded name.
	 *
	 * When decompressing, this means that the amount of the source buffer
	 * that we consumed (which is checked wrt the container's length field)
	 * is the length of the compressed name. A compressed name is defined as
	 * a sequence of labels ending with the root label or a compression
	 * pointer, that is, the segment of the name that dns_name_fromwire()
	 * examines first.
	 *
	 * This matters when handling names that play dirty tricks, like:
	 *
	 *	+---+---+---+---+---+---+
	 *	| 4 | 1 |'a'|192| 0 | 0 |
	 *	+---+---+---+---+---+---+
	 *
	 * We start at octet 1. There is an ordinary single character label "a",
	 * followed by a compression pointer that refers back to octet zero.
	 * Here there is a label of length 4, which weirdly re-uses the octets
	 * we already examined as the data for the label. It is followed by the
	 * root label,
	 *
	 * The specification says that the compressed name ends after the first
	 * zero octet (after the compression pointer) not the second zero octet,
	 * even though the second octet is later in the message. This shows the
	 * correct way to set our "consumed" variable.
	 */

	REQUIRE(DNS_NAME_VALID(name));
	REQUIRE(DNS_NAME_BINDABLE(name));
	REQUIRE((target != NULL && ISC_BUFFER_VALID(target)) ||
		(target == NULL && ISC_BUFFER_VALID(name->buffer)));

	if (target == NULL && name->buffer != NULL) {
		target = name->buffer;
		isc_buffer_clear(target);
	}

	uint8_t *const name_buf = isc_buffer_used(target);
	const uint32_t name_max = ISC_MIN(DNS_NAME_MAXWIRE,
					  isc_buffer_availablelength(target));
	uint32_t name_len = 0;
	MAKE_EMPTY(name); /* in case of failure */

	dns_offsets_t odata;
	uint8_t *offsets = NULL;
	uint32_t labels = 0;
	INIT_OFFSETS(name, offsets, odata);

	/*
	 * After chasing a compression pointer, these variables refer to the
	 * source buffer as follows:
	 *
	 * sb --- mr --- cr --- st --- cd --- sm
	 *
	 * sb = source_buf (const)
	 * mr = marker
	 * cr = cursor
	 * st = start (const)
	 * cd = consumed
	 * sm = source_max (const)
	 *
	 * The marker hops backwards for each pointer.
	 * The cursor steps forwards for each label.
	 * The amount of the source we consumed is set once.
	 */
	const uint8_t *const source_buf = isc_buffer_base(source);
	const uint8_t *const source_max = isc_buffer_used(source);
	const uint8_t *const start = isc_buffer_current(source);
	const uint8_t *marker = start;
	const uint8_t *cursor = start;
	const uint8_t *consumed = NULL;

	/*
	 * One iteration per label.
	 */
	while (cursor < source_max) {
		const uint8_t label_len = *cursor++;
		if (label_len <= DNS_NAME_LABELLEN) {
			/*
			 * Normal label: record its offset, and check bounds on
			 * the name length, which also ensures we don't overrun
			 * the offsets array. Don't touch any source bytes yet!
			 * The source bounds check will happen when we loop.
			 */
			offsets[labels++] = name_len;
			/* and then a step to the ri-i-i-i-i-ight */
			cursor += label_len;
			name_len += label_len + 1;
			if (name_len > name_max) {
				return name_max == DNS_NAME_MAXWIRE
					       ? DNS_R_NAMETOOLONG
					       : ISC_R_NOSPACE;
			} else if (label_len == 0) {
				goto root_label;
			}
		} else if (label_len < 192) {
			return DNS_R_BADLABELTYPE;
		} else if (!dns_decompress_getpermitted(dctx)) {
			return DNS_R_DISALLOWED;
		} else if (cursor < source_max) {
			/*
			 * Compression pointer. Ensure it does not loop.
			 *
			 * Copy multiple labels in one go, to make the most of
			 * memmove() performance. Start at the marker and finish
			 * just before the pointer's hi+lo bytes, before the
			 * cursor. Bounds were already checked.
			 */
			const uint32_t hi = label_len & 0x3F;
			const uint32_t lo = *cursor++;
			const uint8_t *pointer = source_buf + (256 * hi + lo);
			if (pointer >= marker) {
				return DNS_R_BADPOINTER;
			}
			const uint32_t copy_len = (cursor - 2) - marker;
			uint8_t *const dest = name_buf + name_len - copy_len;
			memmove(dest, marker, copy_len);
			consumed = consumed != NULL ? consumed : cursor;
			/* it's just a jump to the left */
			cursor = marker = pointer;
		}
	}
	return ISC_R_UNEXPECTEDEND;
root_label:;
	/*
	 * Copy labels almost like we do for compression pointers,
	 * from the marker up to and including the root label.
	 */
	const uint32_t copy_len = cursor - marker;
	memmove(name_buf + name_len - copy_len, marker, copy_len);
	consumed = consumed != NULL ? consumed : cursor;
	isc_buffer_forward(source, consumed - start);

	name->attributes.absolute = true;
	name->ndata = name_buf;
	name->labels = labels;
	name->length = name_len;
	isc_buffer_add(target, name_len);

	return ISC_R_SUCCESS;
}

isc_result_t
dns_name_towire(const dns_name_t *name, dns_compress_t *cctx,
		isc_buffer_t *target, uint16_t *name_coff) {
	bool compress;
	dns_offsets_t clo;
	dns_name_t clname;
	unsigned int here;
	unsigned int prefix_length;
	unsigned int suffix_coff;

	/*
	 * Convert 'name' into wire format, compressing it as specified by the
	 * compression context 'cctx', and storing the result in 'target'.
	 */

	REQUIRE(DNS_NAME_VALID(name));
	REQUIRE(cctx != NULL);
	REQUIRE(ISC_BUFFER_VALID(target));

	compress = !name->attributes.nocompress &&
		   dns_compress_getpermitted(cctx);

	/*
	 * Write a compression pointer directly if the caller passed us
	 * a pointer to this name's offset that we saved previously.
	 */
	if (compress && name_coff != NULL && *name_coff < 0x4000) {
		if (isc_buffer_availablelength(target) < 2) {
			return ISC_R_NOSPACE;
		}
		isc_buffer_putuint16(target, *name_coff | 0xc000);
		return ISC_R_SUCCESS;
	}

	if (name->offsets == NULL) {
		dns_name_init(&clname, clo);
		dns_name_clone(name, &clname);
		name = &clname;
	}

	/*
	 * Always add the name to the compression context; if compression
	 * is off, reset the return values before writing the name.
	 */
	prefix_length = name->length;
	suffix_coff = 0;
	dns_compress_name(cctx, target, name, &prefix_length, &suffix_coff);
	if (!compress) {
		prefix_length = name->length;
		suffix_coff = 0;
	}

	/*
	 * Return this name's compression offset for use next time, provided
	 * it isn't too short for compression to help (i.e. it's the root)
	 */
	here = isc_buffer_usedlength(target);
	if (name_coff != NULL && here < 0x4000 && prefix_length > 1) {
		*name_coff = (uint16_t)here;
	}

	if (prefix_length > 0) {
		if (isc_buffer_availablelength(target) < prefix_length) {
			return ISC_R_NOSPACE;
		}
		memmove(isc_buffer_used(target), name->ndata, prefix_length);
		isc_buffer_add(target, prefix_length);
	}

	if (suffix_coff > 0) {
		if (name_coff != NULL && prefix_length == 0) {
			*name_coff = suffix_coff;
		}
		if (isc_buffer_availablelength(target) < 2) {
			return ISC_R_NOSPACE;
		}
		isc_buffer_putuint16(target, suffix_coff | 0xc000);
	}

	return ISC_R_SUCCESS;
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

	REQUIRE(prefix == NULL || DNS_NAME_VALID(prefix));
	REQUIRE(suffix == NULL || DNS_NAME_VALID(suffix));
	REQUIRE(name == NULL || DNS_NAME_VALID(name));
	REQUIRE((target != NULL && ISC_BUFFER_VALID(target)) ||
		(target == NULL && name != NULL &&
		 ISC_BUFFER_VALID(name->buffer)));
	if (prefix == NULL || prefix->labels == 0) {
		copy_prefix = false;
	}
	if (suffix == NULL || suffix->labels == 0) {
		copy_suffix = false;
	}
	if (copy_prefix && prefix->attributes.absolute) {
		absolute = true;
		REQUIRE(!copy_suffix);
	}
	if (name == NULL) {
		dns_name_init(&tmp_name, odata);
		name = &tmp_name;
	}
	if (target == NULL) {
		INSIST(name->buffer != NULL);
		target = name->buffer;
		isc_buffer_clear(name->buffer);
	}

	REQUIRE(DNS_NAME_BINDABLE(name));

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
		return DNS_R_NAMETOOLONG;
	}
	if (length > nrem) {
		MAKE_EMPTY(name);
		return ISC_R_NOSPACE;
	}

	if (copy_suffix) {
		if (suffix->attributes.absolute) {
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
	name->attributes.absolute = absolute;

	if (name->labels > 0 && name->offsets != NULL) {
		INIT_OFFSETS(name, offsets, odata);
		set_offsets(name, offsets, NULL);
	}

	isc_buffer_add(target, name->length);

	return ISC_R_SUCCESS;
}

void
dns_name_dup(const dns_name_t *source, isc_mem_t *mctx, dns_name_t *target) {
	/*
	 * Make 'target' a dynamically allocated copy of 'source'.
	 */

	REQUIRE(DNS_NAME_VALID(source));
	REQUIRE(source->length > 0);
	REQUIRE(DNS_NAME_VALID(target));
	REQUIRE(DNS_NAME_BINDABLE(target));

	/*
	 * Make 'target' empty in case of failure.
	 */
	MAKE_EMPTY(target);

	target->ndata = isc_mem_get(mctx, source->length);

	memmove(target->ndata, source->ndata, source->length);

	target->length = source->length;
	target->labels = source->labels;
	target->attributes = (struct dns_name_attrs){ .dynamic = true };
	target->attributes.absolute = source->attributes.absolute;
	if (target->offsets != NULL) {
		if (source->offsets != NULL) {
			memmove(target->offsets, source->offsets,
				source->labels);
		} else {
			set_offsets(target, target->offsets, NULL);
		}
	}
}

void
dns_name_dupwithoffsets(const dns_name_t *source, isc_mem_t *mctx,
			dns_name_t *target) {
	/*
	 * Make 'target' a read-only dynamically allocated copy of 'source'.
	 * 'target' will also have a dynamically allocated offsets table.
	 */

	REQUIRE(DNS_NAME_VALID(source));
	REQUIRE(source->length > 0);
	REQUIRE(DNS_NAME_VALID(target));
	REQUIRE(DNS_NAME_BINDABLE(target));
	REQUIRE(target->offsets == NULL);

	/*
	 * Make 'target' empty in case of failure.
	 */
	MAKE_EMPTY(target);

	target->ndata = isc_mem_get(mctx, source->length + source->labels);

	memmove(target->ndata, source->ndata, source->length);

	target->length = source->length;
	target->labels = source->labels;
	target->attributes = (struct dns_name_attrs){ .dynamic = true,
						      .dynoffsets = true,
						      .readonly = true };
	target->attributes.absolute = source->attributes.absolute;
	target->offsets = target->ndata + source->length;
	if (source->offsets != NULL) {
		memmove(target->offsets, source->offsets, source->labels);
	} else {
		set_offsets(target, target->offsets, NULL);
	}
}

void
dns_name_free(dns_name_t *name, isc_mem_t *mctx) {
	size_t size;

	/*
	 * Free 'name'.
	 */

	REQUIRE(DNS_NAME_VALID(name));
	REQUIRE(name->attributes.dynamic);

	size = name->length;
	if (name->attributes.dynoffsets) {
		size += name->labels;
	}
	isc_mem_put(mctx, name->ndata, size);
	dns_name_invalidate(name);
}

size_t
dns_name_size(const dns_name_t *name) {
	size_t size;

	REQUIRE(DNS_NAME_VALID(name));

	if (!name->attributes.dynamic) {
		return 0;
	}

	size = name->length;
	if (name->attributes.dynoffsets) {
		size += name->labels;
	}

	return size;
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

	REQUIRE(DNS_NAME_VALID(name));
	REQUIRE(digest != NULL);

	dns_name_init(&downname, NULL);

	isc_buffer_init(&buffer, data, sizeof(data));

	result = dns_name_downcase(name, &downname, &buffer);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	isc_buffer_usedregion(&buffer, &r);

	return (digest)(arg, &r);
}

bool
dns_name_dynamic(const dns_name_t *name) {
	REQUIRE(DNS_NAME_VALID(name));

	/*
	 * Returns whether there is dynamic memory associated with this name.
	 */

	return name->attributes.dynamic;
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

	REQUIRE(DNS_NAME_VALID(name));

	isc_buffer_init(&b, t, sizeof(t));
	result = dns_name_totext(name, 0, &b);
	if (result != ISC_R_SUCCESS) {
		return result;
	}
	isc_buffer_usedregion(&b, &r);
	fprintf(stream, "%.*s", (int)r.length, (char *)r.base);

	return ISC_R_SUCCESS;
}

isc_result_t
dns_name_settotextfilter(dns_name_totextfilter_t *proc) {
	/*
	 * If we already have been here set / clear as appropriate.
	 */
	if (totext_filter_proc != NULL && proc != NULL) {
		if (totext_filter_proc == proc) {
			return ISC_R_SUCCESS;
		}
	}
	if (proc == NULL && totext_filter_proc != NULL) {
		totext_filter_proc = NULL;
		return ISC_R_SUCCESS;
	}

	totext_filter_proc = proc;

	return ISC_R_SUCCESS;
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
	result = dns_name_totext(name, DNS_NAME_OMITFINALDOT, &buf);
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

	REQUIRE(DNS_NAME_VALID(name));
	REQUIRE(target != NULL && *target == NULL);

	isc_buffer_init(&buf, txt, sizeof(txt));
	result = dns_name_totext(name, 0, &buf);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	isc_buffer_usedregion(&buf, &reg);
	p = isc_mem_allocate(mctx, reg.length + 1);
	memmove(p, (char *)reg.base, (int)reg.length);
	p[reg.length] = '\0';

	*target = p;
	return ISC_R_SUCCESS;
}

isc_result_t
dns_name_fromstring(dns_name_t *target, const char *src,
		    const dns_name_t *origin, unsigned int options,
		    isc_mem_t *mctx) {
	isc_result_t result;
	isc_buffer_t buf;
	dns_fixedname_t fn;
	dns_name_t *name;

	REQUIRE(src != NULL);

	isc_buffer_constinit(&buf, src, strlen(src));
	isc_buffer_add(&buf, strlen(src));
	if (DNS_NAME_BINDABLE(target) && target->buffer != NULL) {
		name = target;
	} else {
		name = dns_fixedname_initname(&fn);
	}

	result = dns_name_fromtext(name, &buf, origin, options, NULL);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	if (name != target) {
		dns_name_dupwithoffsets(name, mctx, target);
	}
	return result;
}

void
dns_name_copy(const dns_name_t *source, dns_name_t *dest) {
	isc_buffer_t *target = NULL;
	unsigned char *ndata = NULL;

	REQUIRE(DNS_NAME_VALID(source));
	REQUIRE(DNS_NAME_VALID(dest));
	REQUIRE(DNS_NAME_BINDABLE(dest));

	target = dest->buffer;

	REQUIRE(target != NULL);
	REQUIRE(target->length >= source->length);

	isc_buffer_clear(target);

	ndata = (unsigned char *)target->base;
	dest->ndata = target->base;

	if (source->length != 0) {
		memmove(ndata, source->ndata, source->length);
	}

	dest->ndata = ndata;
	dest->labels = source->labels;
	dest->length = source->length;
	dest->attributes.absolute = source->attributes.absolute;

	if (dest->labels > 0 && dest->offsets != NULL) {
		if (source->offsets != NULL && source->labels != 0) {
			memmove(dest->offsets, source->offsets, source->labels);
		} else {
			set_offsets(dest, dest->offsets, NULL);
		}
	}

	isc_buffer_add(target, dest->length);
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
				return true;
			}
		}
	}

	return false;
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
			return true;
		}
	}
	return false;
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
			return true;
		}
	}
	return false;
}

bool
dns_name_istat(const dns_name_t *name) {
	unsigned char len;
	const unsigned char *ndata;

	REQUIRE(DNS_NAME_VALID(name));

	if (name->labels < 1) {
		return false;
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
		return false;
	}

	if (ndata[0] != '_' || isc_ascii_tolower(ndata[1]) != 't' ||
	    isc_ascii_tolower(ndata[2]) != 'a')
	{
		return false;
	}
	ndata += 3;
	len -= 3;

	while (len > 0) {
		INSIST(len >= 5);
		if (ndata[0] != '-' || !isc_hex_char(ndata[1]) ||
		    !isc_hex_char(ndata[2]) || !isc_hex_char(ndata[3]) ||
		    !isc_hex_char(ndata[4]))
		{
			return false;
		}
		ndata += 5;
		len -= 5;
	}
	return true;
}

bool
dns_name_isdnssvcb(const dns_name_t *name) {
	unsigned char len, len1;
	const unsigned char *ndata;

	REQUIRE(DNS_NAME_VALID(name));

	if (name->labels < 1 || name->length < 5) {
		return false;
	}

	ndata = name->ndata;
	len = len1 = ndata[0];
	INSIST(len <= name->length);
	ndata++;

	if (len < 2 || ndata[0] != '_') {
		return false;
	}
	if (isdigit(ndata[1]) && name->labels > 1) {
		char buf[sizeof("65000")];
		long port;
		char *endp;

		/*
		 * Do we have a valid _port label?
		 */
		if (len > 6U || (ndata[1] == '0' && len != 2)) {
			return false;
		}
		memcpy(buf, ndata + 1, len - 1);
		buf[len - 1] = 0;
		port = strtol(buf, &endp, 10);
		if (*endp != 0 || port < 0 || port > 0xffff) {
			return false;
		}

		/*
		 * Move to next label.
		 */
		ndata += len;
		INSIST(len1 + 1U < name->length);
		len = *ndata;
		INSIST(len + len1 + 1U <= name->length);
		ndata++;
	}

	if (len == 4U && strncasecmp((const char *)ndata, "_dns", 4) == 0) {
		return true;
	}

	return false;
}
