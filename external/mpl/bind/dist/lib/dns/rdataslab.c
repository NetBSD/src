/*	$NetBSD: rdataslab.c,v 1.10 2025/01/26 16:25:24 christos Exp $	*/

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
#include <stdbool.h>
#include <stdlib.h>

#include <isc/ascii.h>
#include <isc/mem.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdataslab.h>
#include <dns/stats.h>

#define CASESET(header)                                \
	((atomic_load_acquire(&(header)->attributes) & \
	  DNS_SLABHEADERATTR_CASESET) != 0)
#define CASEFULLYLOWER(header)                         \
	((atomic_load_acquire(&(header)->attributes) & \
	  DNS_SLABHEADERATTR_CASEFULLYLOWER) != 0)
#define NONEXISTENT(header)                            \
	((atomic_load_acquire(&(header)->attributes) & \
	  DNS_SLABHEADERATTR_NONEXISTENT) != 0)

/*
 * The rdataslab structure allows iteration to occur in both load order
 * and DNSSEC order.  The structure is as follows:
 *
 *	header		(reservelen bytes)
 *	record count	(2 bytes)
 *	offset table	(4 x record count bytes in load order)
 *	data records
 *		data length	(2 bytes)
 *		order		(2 bytes)
 *		meta data	(1 byte for RRSIG's)
 *		data		(data length bytes)
 *
 * If DNS_RDATASET_FIXED is defined to be zero (0) the format of a
 * rdataslab is as follows:
 *
 *	header		(reservelen bytes)
 *	record count	(2 bytes)
 *	data records
 *		data length	(2 bytes)
 *		meta data	(1 byte for RRSIG's)
 *		data		(data length bytes)
 *
 * Offsets are from the end of the header.
 *
 * Load order traversal is performed by walking the offset table to find
 * the start of the record (DNS_RDATASET_FIXED = 1).
 *
 * DNSSEC order traversal is performed by walking the data records.
 *
 * The order is stored with record to allow for efficient reconstruction
 * of the offset table following a merge or subtraction.
 *
 * The iterator methods in rbtdb support both load order and DNSSEC order
 * iteration.
 *
 * WARNING:
 *	rbtdb.c directly interacts with the slab's raw structures.  If the
 *	structure changes then rbtdb.c also needs to be updated to reflect
 *	the changes.  See the areas tagged with "RDATASLAB".
 */

struct xrdata {
	dns_rdata_t rdata;
#if DNS_RDATASET_FIXED
	unsigned int order;
#endif /* if DNS_RDATASET_FIXED */
};

#define peek_uint16(buffer) ({ ((uint16_t)*(buffer) << 8) | *((buffer) + 1); })
#define get_uint16(buffer)                            \
	({                                            \
		uint16_t __ret = peek_uint16(buffer); \
		buffer += sizeof(uint16_t);           \
		__ret;                                \
	})

static void
rdataset_disassociate(dns_rdataset_t *rdataset DNS__DB_FLARG);
static isc_result_t
rdataset_first(dns_rdataset_t *rdataset);
static isc_result_t
rdataset_next(dns_rdataset_t *rdataset);
static void
rdataset_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata);
static void
rdataset_clone(dns_rdataset_t *source, dns_rdataset_t *target DNS__DB_FLARG);
static unsigned int
rdataset_count(dns_rdataset_t *rdataset);
static isc_result_t
rdataset_getnoqname(dns_rdataset_t *rdataset, dns_name_t *name,
		    dns_rdataset_t *neg, dns_rdataset_t *negsig DNS__DB_FLARG);
static isc_result_t
rdataset_getclosest(dns_rdataset_t *rdataset, dns_name_t *name,
		    dns_rdataset_t *neg, dns_rdataset_t *negsig DNS__DB_FLARG);
static void
rdataset_settrust(dns_rdataset_t *rdataset, dns_trust_t trust);
static void
rdataset_expire(dns_rdataset_t *rdataset DNS__DB_FLARG);
static void
rdataset_clearprefetch(dns_rdataset_t *rdataset);
static void
rdataset_setownercase(dns_rdataset_t *rdataset, const dns_name_t *name);
static void
rdataset_getownercase(const dns_rdataset_t *rdataset, dns_name_t *name);

/*% Note: the "const void *" are just to make qsort happy.  */
static int
compare_rdata(const void *p1, const void *p2) {
	const struct xrdata *x1 = p1;
	const struct xrdata *x2 = p2;
	return dns_rdata_compare(&x1->rdata, &x2->rdata);
}

#if DNS_RDATASET_FIXED
static void
fillin_offsets(unsigned char *offsetbase, unsigned int *offsettable,
	       unsigned int length) {
	unsigned int i, j;
	unsigned char *raw = NULL;

	for (i = 0, j = 0; i < length; i++) {
		if (offsettable[i] == 0) {
			continue;
		}

		/*
		 * Fill in offset table.
		 */
		raw = &offsetbase[j * 4 + 2];
		*raw++ = (offsettable[i] & 0xff000000) >> 24;
		*raw++ = (offsettable[i] & 0xff0000) >> 16;
		*raw++ = (offsettable[i] & 0xff00) >> 8;
		*raw = offsettable[i] & 0xff;

		/*
		 * Fill in table index.
		 */
		raw = offsetbase + offsettable[i] + 2;
		*raw++ = (j & 0xff00) >> 8;
		*raw = j++ & 0xff;
	}
}
#endif /* if DNS_RDATASET_FIXED */

isc_result_t
dns_rdataslab_fromrdataset(dns_rdataset_t *rdataset, isc_mem_t *mctx,
			   isc_region_t *region, unsigned int reservelen,
			   uint32_t maxrrperset) {
	/*
	 * Use &removed as a sentinel pointer for duplicate
	 * rdata as rdata.data == NULL is valid.
	 */
	static unsigned char removed;
	struct xrdata *x = NULL;
	unsigned char *rawbuf = NULL;
	unsigned int buflen;
	isc_result_t result;
	unsigned int nitems;
	unsigned int nalloc;
	unsigned int length;
	unsigned int i;
#if DNS_RDATASET_FIXED
	unsigned char *offsetbase = NULL;
	unsigned int *offsettable = NULL;
#endif /* if DNS_RDATASET_FIXED */

	buflen = reservelen + 2;

	nitems = dns_rdataset_count(rdataset);

	/*
	 * If there are no rdata then we can just need to allocate a header
	 * with zero a record count.
	 */
	if (nitems == 0) {
		if (rdataset->type != 0) {
			return ISC_R_FAILURE;
		}
		rawbuf = isc_mem_get(mctx, buflen);
		region->base = rawbuf;
		region->length = buflen;
		rawbuf += reservelen;
		*rawbuf++ = 0;
		*rawbuf = 0;
		return ISC_R_SUCCESS;
	}

	if (maxrrperset > 0 && nitems > maxrrperset) {
		return DNS_R_TOOMANYRECORDS;
	}

	if (nitems > 0xffff) {
		return ISC_R_NOSPACE;
	}

	/*
	 * Remember the original number of items.
	 */
	nalloc = nitems;
	x = isc_mem_cget(mctx, nalloc, sizeof(struct xrdata));

	/*
	 * Save all of the rdata members into an array.
	 */
	result = dns_rdataset_first(rdataset);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOMORE) {
		goto free_rdatas;
	}
	for (i = 0; i < nalloc && result == ISC_R_SUCCESS; i++) {
		INSIST(result == ISC_R_SUCCESS);
		dns_rdata_init(&x[i].rdata);
		dns_rdataset_current(rdataset, &x[i].rdata);
		INSIST(x[i].rdata.data != &removed);
#if DNS_RDATASET_FIXED
		x[i].order = i;
#endif /* if DNS_RDATASET_FIXED */
		result = dns_rdataset_next(rdataset);
	}
	if (i != nalloc || result != ISC_R_NOMORE) {
		/*
		 * Somehow we iterated over fewer rdatas than
		 * dns_rdataset_count() said there were or there
		 * were more items than dns_rdataset_count said
		 * there were.
		 */
		result = ISC_R_FAILURE;
		goto free_rdatas;
	}

	/*
	 * Put into DNSSEC order.
	 */
	if (nalloc > 1U) {
		qsort(x, nalloc, sizeof(struct xrdata), compare_rdata);
	}

	/*
	 * Remove duplicates and compute the total storage required.
	 *
	 * If an rdata is not a duplicate, accumulate the storage size
	 * required for the rdata.  We do not store the class, type, etc,
	 * just the rdata, so our overhead is 2 bytes for the number of
	 * records, and 8 for each rdata, (length(2), offset(4) and order(2))
	 * and then the rdata itself.
	 */
	for (i = 1; i < nalloc; i++) {
		if (compare_rdata(&x[i - 1].rdata, &x[i].rdata) == 0) {
			x[i - 1].rdata.data = &removed;
#if DNS_RDATASET_FIXED
			/*
			 * Preserve the least order so A, B, A -> A, B
			 * after duplicate removal.
			 */
			if (x[i - 1].order < x[i].order) {
				x[i].order = x[i - 1].order;
			}
#endif /* if DNS_RDATASET_FIXED */
			nitems--;
		} else {
#if DNS_RDATASET_FIXED
			buflen += (8 + x[i - 1].rdata.length);
#else  /* if DNS_RDATASET_FIXED */
			buflen += (2 + x[i - 1].rdata.length);
#endif /* if DNS_RDATASET_FIXED */
			/*
			 * Provide space to store the per RR meta data.
			 */
			if (rdataset->type == dns_rdatatype_rrsig) {
				buflen++;
			}
		}
	}

	/*
	 * Don't forget the last item!
	 */
#if DNS_RDATASET_FIXED
	buflen += (8 + x[i - 1].rdata.length);
#else  /* if DNS_RDATASET_FIXED */
	buflen += (2 + x[i - 1].rdata.length);
#endif /* if DNS_RDATASET_FIXED */
	/*
	 * Provide space to store the per RR meta data.
	 */
	if (rdataset->type == dns_rdatatype_rrsig) {
		buflen++;
	}

	/*
	 * Ensure that singleton types are actually singletons.
	 */
	if (nitems > 1 && dns_rdatatype_issingleton(rdataset->type)) {
		/*
		 * We have a singleton type, but there's more than one
		 * RR in the rdataset.
		 */
		result = DNS_R_SINGLETON;
		goto free_rdatas;
	}

	/*
	 * Allocate the memory, set up a buffer, start copying in
	 * data.
	 */
	rawbuf = isc_mem_cget(mctx, 1, buflen);

#if DNS_RDATASET_FIXED
	/* Allocate temporary offset table. */
	offsettable = isc_mem_cget(mctx, nalloc, sizeof(unsigned int));
#endif /* if DNS_RDATASET_FIXED */

	region->base = rawbuf;
	region->length = buflen;

	rawbuf += reservelen;

#if DNS_RDATASET_FIXED
	offsetbase = rawbuf;
#endif /* if DNS_RDATASET_FIXED */

	*rawbuf++ = (nitems & 0xff00) >> 8;
	*rawbuf++ = (nitems & 0x00ff);

#if DNS_RDATASET_FIXED
	/* Skip load order table.  Filled in later. */
	rawbuf += nitems * 4;
#endif /* if DNS_RDATASET_FIXED */

	for (i = 0; i < nalloc; i++) {
		if (x[i].rdata.data == &removed) {
			continue;
		}
#if DNS_RDATASET_FIXED
		offsettable[x[i].order] = rawbuf - offsetbase;
#endif /* if DNS_RDATASET_FIXED */
		length = x[i].rdata.length;
		if (rdataset->type == dns_rdatatype_rrsig) {
			length++;
		}
		INSIST(length <= 0xffff);
		*rawbuf++ = (length & 0xff00) >> 8;
		*rawbuf++ = (length & 0x00ff);
#if DNS_RDATASET_FIXED
		rawbuf += 2; /* filled in later */
#endif			     /* if DNS_RDATASET_FIXED */
		/*
		 * Store the per RR meta data.
		 */
		if (rdataset->type == dns_rdatatype_rrsig) {
			*rawbuf++ = (x[i].rdata.flags & DNS_RDATA_OFFLINE)
					    ? DNS_RDATASLAB_OFFLINE
					    : 0;
		}
		if (x[i].rdata.length != 0) {
			memmove(rawbuf, x[i].rdata.data, x[i].rdata.length);
		}
		rawbuf += x[i].rdata.length;
	}

#if DNS_RDATASET_FIXED
	fillin_offsets(offsetbase, offsettable, nalloc);
	isc_mem_cput(mctx, offsettable, nalloc, sizeof(unsigned int));
#endif /* if DNS_RDATASET_FIXED */

	result = ISC_R_SUCCESS;

free_rdatas:
	isc_mem_cput(mctx, x, nalloc, sizeof(struct xrdata));
	return result;
}

unsigned int
dns_rdataslab_size(unsigned char *slab, unsigned int reservelen) {
	REQUIRE(slab != NULL);

	unsigned char *current = slab + reservelen;
	uint16_t count = get_uint16(current);

#if DNS_RDATASET_FIXED
	current += (4 * count);
#endif /* if DNS_RDATASET_FIXED */

	while (count-- > 0) {
		uint16_t length = get_uint16(current);
		current += length;
#if DNS_RDATASET_FIXED
		current += 2;
#endif /* if DNS_RDATASET_FIXED */
	}

	return (unsigned int)(current - slab);
}

unsigned int
dns_rdataslab_rdatasize(unsigned char *slab, unsigned int reservelen) {
	REQUIRE(slab != NULL);

	uint16_t rdatalen = 0;
	unsigned char *current = slab + reservelen;
	uint16_t count = get_uint16(current);

#if DNS_RDATASET_FIXED
	current += (4 * count);
#endif /* if DNS_RDATASET_FIXED */

	while (count-- > 0) {
		uint16_t length = get_uint16(current);
		rdatalen += length;
		current += length;
#if DNS_RDATASET_FIXED
		current += 2;
#endif /* if DNS_RDATASET_FIXED */
	}

	return rdatalen;
}

unsigned int
dns_rdataslab_count(unsigned char *slab, unsigned int reservelen) {
	REQUIRE(slab != NULL);

	unsigned char *current = slab + reservelen;
	uint16_t count = get_uint16(current);

	return count;
}

/*
 * Make the dns_rdata_t 'rdata' refer to the slab item
 * beginning at '*current', which is part of a slab of type
 * 'type' and class 'rdclass', and advance '*current' to
 * point to the next item in the slab.
 */
static void
rdata_from_slab(unsigned char **current, dns_rdataclass_t rdclass,
		dns_rdatatype_t type, dns_rdata_t *rdata) {
	unsigned char *tcurrent = *current;
	isc_region_t region;
	bool offline = false;
	uint16_t length = get_uint16(tcurrent);

	if (type == dns_rdatatype_rrsig) {
		if ((*tcurrent & DNS_RDATASLAB_OFFLINE) != 0) {
			offline = true;
		}
		length--;
		tcurrent++;
	}
	region.length = length;
#if DNS_RDATASET_FIXED
	tcurrent += 2;
#endif /* if DNS_RDATASET_FIXED */
	region.base = tcurrent;
	tcurrent += region.length;
	dns_rdata_fromregion(rdata, rdclass, type, &region);
	if (offline) {
		rdata->flags |= DNS_RDATA_OFFLINE;
	}
	*current = tcurrent;
}

/*
 * Return true iff 'slab' (slab data of type 'type' and class 'rdclass')
 * contains an rdata identical to 'rdata'.  This does case insensitive
 * comparisons per DNSSEC.
 */
static bool
rdata_in_slab(unsigned char *slab, unsigned int reservelen,
	      dns_rdataclass_t rdclass, dns_rdatatype_t type,
	      dns_rdata_t *rdata) {
	unsigned char *current = slab + reservelen;

	uint16_t count = get_uint16(current);

#if DNS_RDATASET_FIXED
	current += (4 * count);
#endif /* if DNS_RDATASET_FIXED */

	for (size_t i = 0; i < count; i++) {
		dns_rdata_t trdata = DNS_RDATA_INIT;
		rdata_from_slab(&current, rdclass, type, &trdata);

		int n = dns_rdata_compare(&trdata, rdata);
		if (n == 0) {
			return true;
		}
		if (n > 0) { /* In DNSSEC order. */
			break;
		}
		dns_rdata_reset(&trdata);
	}
	return false;
}

isc_result_t
dns_rdataslab_merge(unsigned char *oslab, unsigned char *nslab,
		    unsigned int reservelen, isc_mem_t *mctx,
		    dns_rdataclass_t rdclass, dns_rdatatype_t type,
		    unsigned int flags, uint32_t maxrrperset,
		    unsigned char **tslabp) {
	unsigned char *ocurrent = NULL, *ostart = NULL, *ncurrent = NULL;
	unsigned char *tstart = NULL, *tcurrent = NULL, *data = NULL;
	unsigned int ocount, ncount, count, olength, tlength, tcount, length;
	dns_rdata_t ordata = DNS_RDATA_INIT;
	dns_rdata_t nrdata = DNS_RDATA_INIT;
	bool added_something = false;
	unsigned int oadded = 0;
	unsigned int nadded = 0;
	unsigned int nncount = 0;
#if DNS_RDATASET_FIXED
	unsigned int oncount;
	unsigned int norder = 0;
	unsigned int oorder = 0;
	unsigned char *offsetbase = NULL;
	unsigned int *offsettable = NULL;
#endif /* if DNS_RDATASET_FIXED */

	/*
	 * XXX  Need parameter to allow "delete rdatasets in nslab" merge,
	 * or perhaps another merge routine for this purpose.
	 */

	REQUIRE(tslabp != NULL && *tslabp == NULL);
	REQUIRE(oslab != NULL && nslab != NULL);

	ocurrent = oslab + reservelen;
	ocount = get_uint16(ocurrent);
#if DNS_RDATASET_FIXED
	ocurrent += (4 * ocount);
#endif /* if DNS_RDATASET_FIXED */
	ostart = ocurrent;
	ncurrent = nslab + reservelen;
	ncount = get_uint16(ncurrent);
#if DNS_RDATASET_FIXED
	ncurrent += (4 * ncount);
#endif /* if DNS_RDATASET_FIXED */
	INSIST(ocount > 0 && ncount > 0);

	if (maxrrperset > 0 && ocount + ncount > maxrrperset) {
		return DNS_R_TOOMANYRECORDS;
	}

#if DNS_RDATASET_FIXED
	oncount = ncount;
#endif /* if DNS_RDATASET_FIXED */

	/*
	 * Yes, this is inefficient!
	 */

	/*
	 * Figure out the length of the old slab's data.
	 */
	olength = 0;
	for (count = 0; count < ocount; count++) {
		length = get_uint16(ocurrent);
#if DNS_RDATASET_FIXED
		olength += length + 8;
		ocurrent += length + 2;
#else  /* if DNS_RDATASET_FIXED */
		olength += length + 2;
		ocurrent += length;
#endif /* if DNS_RDATASET_FIXED */
	}

	/*
	 * Start figuring out the target length and count.
	 */
	tlength = reservelen + 2 + olength;
	tcount = ocount;

	/*
	 * Add in the length of rdata in the new slab that aren't in
	 * the old slab.
	 */
	do {
		dns_rdata_init(&nrdata);
		rdata_from_slab(&ncurrent, rdclass, type, &nrdata);
		if (!rdata_in_slab(oslab, reservelen, rdclass, type, &nrdata)) {
			/*
			 * This rdata isn't in the old slab.
			 */
#if DNS_RDATASET_FIXED
			tlength += nrdata.length + 8;
#else  /* if DNS_RDATASET_FIXED */
			tlength += nrdata.length + 2;
#endif /* if DNS_RDATASET_FIXED */
			if (type == dns_rdatatype_rrsig) {
				tlength++;
			}
			tcount++;
			nncount++;
			added_something = true;
		}
		ncount--;
	} while (ncount > 0);
	ncount = nncount;

	if (((flags & DNS_RDATASLAB_EXACT) != 0) && (tcount != ncount + ocount))
	{
		return DNS_R_NOTEXACT;
	}

	if (!added_something && (flags & DNS_RDATASLAB_FORCE) == 0) {
		return DNS_R_UNCHANGED;
	}

	/*
	 * Ensure that singleton types are actually singletons.
	 */
	if (tcount > 1 && dns_rdatatype_issingleton(type)) {
		/*
		 * We have a singleton type, but there's more than one
		 * RR in the rdataset.
		 */
		return DNS_R_SINGLETON;
	}

	if (tcount > 0xffff) {
		return ISC_R_NOSPACE;
	}

	/*
	 * Copy the reserved area from the new slab.
	 */
	tstart = isc_mem_get(mctx, tlength);
	memmove(tstart, nslab, reservelen);
	tcurrent = tstart + reservelen;
#if DNS_RDATASET_FIXED
	offsetbase = tcurrent;
#endif /* if DNS_RDATASET_FIXED */

	/*
	 * Write the new count.
	 */
	*tcurrent++ = (tcount & 0xff00) >> 8;
	*tcurrent++ = (tcount & 0x00ff);

#if DNS_RDATASET_FIXED
	/*
	 * Skip offset table.
	 */
	tcurrent += (tcount * 4);

	offsettable = isc_mem_cget(mctx, (ocount + oncount),
				   sizeof(unsigned int));
#endif /* if DNS_RDATASET_FIXED */

	/*
	 * Merge the two slabs.
	 */
	ocurrent = ostart;
	INSIST(ocount != 0);
#if DNS_RDATASET_FIXED
	oorder = peek_uint16(&ocurrent[2]);
	INSIST(oorder < ocount);
#endif /* if DNS_RDATASET_FIXED */
	rdata_from_slab(&ocurrent, rdclass, type, &ordata);

	ncurrent = nslab + reservelen + 2;
#if DNS_RDATASET_FIXED
	ncurrent += (4 * oncount);
#endif /* if DNS_RDATASET_FIXED */

	if (ncount > 0) {
		do {
			dns_rdata_reset(&nrdata);
#if DNS_RDATASET_FIXED
			norder = peek_uint16(&ncurrent[2]);

			INSIST(norder < oncount);
#endif /* if DNS_RDATASET_FIXED */
			rdata_from_slab(&ncurrent, rdclass, type, &nrdata);
		} while (rdata_in_slab(oslab, reservelen, rdclass, type,
				       &nrdata));
	}

	while (oadded < ocount || nadded < ncount) {
		bool fromold;
		if (oadded == ocount) {
			fromold = false;
		} else if (nadded == ncount) {
			fromold = true;
		} else {
			fromold = (dns_rdata_compare(&ordata, &nrdata) < 0);
		}
		if (fromold) {
#if DNS_RDATASET_FIXED
			offsettable[oorder] = tcurrent - offsetbase;
#endif /* if DNS_RDATASET_FIXED */
			length = ordata.length;
			data = ordata.data;
			if (type == dns_rdatatype_rrsig) {
				length++;
				data--;
			}
			*tcurrent++ = (length & 0xff00) >> 8;
			*tcurrent++ = (length & 0x00ff);
#if DNS_RDATASET_FIXED
			tcurrent += 2; /* fill in later */
#endif				       /* if DNS_RDATASET_FIXED */
			memmove(tcurrent, data, length);
			tcurrent += length;
			oadded++;
			if (oadded < ocount) {
				dns_rdata_reset(&ordata);
#if DNS_RDATASET_FIXED
				oorder = peek_uint16(&ocurrent[2]);
				INSIST(oorder < ocount);
#endif /* if DNS_RDATASET_FIXED */
				rdata_from_slab(&ocurrent, rdclass, type,
						&ordata);
			}
		} else {
#if DNS_RDATASET_FIXED
			offsettable[ocount + norder] = tcurrent - offsetbase;
#endif /* if DNS_RDATASET_FIXED */
			length = nrdata.length;
			data = nrdata.data;
			if (type == dns_rdatatype_rrsig) {
				length++;
				data--;
			}
			*tcurrent++ = (length & 0xff00) >> 8;
			*tcurrent++ = (length & 0x00ff);
#if DNS_RDATASET_FIXED
			tcurrent += 2; /* fill in later */
#endif				       /* if DNS_RDATASET_FIXED */
			memmove(tcurrent, data, length);
			tcurrent += length;
			nadded++;
			if (nadded < ncount) {
				do {
					dns_rdata_reset(&nrdata);
#if DNS_RDATASET_FIXED
					norder = peek_uint16(&ncurrent[2]);
					INSIST(norder < oncount);
#endif /* if DNS_RDATASET_FIXED */
					rdata_from_slab(&ncurrent, rdclass,
							type, &nrdata);
				} while (rdata_in_slab(oslab, reservelen,
						       rdclass, type, &nrdata));
			}
		}
	}

#if DNS_RDATASET_FIXED
	fillin_offsets(offsetbase, offsettable, ocount + oncount);

	isc_mem_cput(mctx, offsettable, (ocount + oncount),
		     sizeof(unsigned int));
#endif /* if DNS_RDATASET_FIXED */

	INSIST(tcurrent == tstart + tlength);

	*tslabp = tstart;

	return ISC_R_SUCCESS;
}

isc_result_t
dns_rdataslab_subtract(unsigned char *mslab, unsigned char *sslab,
		       unsigned int reservelen, isc_mem_t *mctx,
		       dns_rdataclass_t rdclass, dns_rdatatype_t type,
		       unsigned int flags, unsigned char **tslabp) {
	unsigned char *mcurrent = NULL, *sstart = NULL, *scurrent = NULL;
	unsigned char *tstart = NULL, *tcurrent = NULL;
	unsigned int mcount, scount, rcount, count, tlength, tcount, i;
	dns_rdata_t srdata = DNS_RDATA_INIT;
	dns_rdata_t mrdata = DNS_RDATA_INIT;
#if DNS_RDATASET_FIXED
	unsigned char *offsetbase = NULL;
	unsigned int *offsettable = NULL;
	unsigned int order;
#endif /* if DNS_RDATASET_FIXED */

	REQUIRE(tslabp != NULL && *tslabp == NULL);
	REQUIRE(mslab != NULL && sslab != NULL);

	mcurrent = mslab + reservelen;
	mcount = get_uint16(mcurrent);
	scurrent = sslab + reservelen;
	scount = get_uint16(scurrent);
	INSIST(mcount > 0 && scount > 0);

	/*
	 * Yes, this is inefficient!
	 */

	/*
	 * Start figuring out the target length and count.
	 */
	tlength = reservelen + 2;
	tcount = 0;
	rcount = 0;

#if DNS_RDATASET_FIXED
	mcurrent += 4 * mcount;
	scurrent += 4 * scount;
#endif /* if DNS_RDATASET_FIXED */
	sstart = scurrent;

	/*
	 * Add in the length of rdata in the mslab that aren't in
	 * the sslab.
	 */
	for (i = 0; i < mcount; i++) {
		unsigned char *mrdatabegin = mcurrent;
		rdata_from_slab(&mcurrent, rdclass, type, &mrdata);
		scurrent = sstart;
		for (count = 0; count < scount; count++) {
			dns_rdata_reset(&srdata);
			rdata_from_slab(&scurrent, rdclass, type, &srdata);
			if (dns_rdata_compare(&mrdata, &srdata) == 0) {
				break;
			}
		}
		if (count == scount) {
			/*
			 * This rdata isn't in the sslab, and thus isn't
			 * being subtracted.
			 */
			tlength += (unsigned int)(mcurrent - mrdatabegin);
			tcount++;
		} else {
			rcount++;
		}
		dns_rdata_reset(&mrdata);
	}

#if DNS_RDATASET_FIXED
	tlength += (4 * tcount);
#endif /* if DNS_RDATASET_FIXED */

	/*
	 * Check that all the records originally existed.  The numeric
	 * check only works as rdataslabs do not contain duplicates.
	 */
	if (((flags & DNS_RDATASLAB_EXACT) != 0) && (rcount != scount)) {
		return DNS_R_NOTEXACT;
	}

	/*
	 * Don't continue if the new rdataslab would be empty.
	 */
	if (tcount == 0) {
		return DNS_R_NXRRSET;
	}

	/*
	 * If nothing is going to change, we can stop.
	 */
	if (rcount == 0) {
		return DNS_R_UNCHANGED;
	}

	/*
	 * Copy the reserved area from the mslab.
	 */
	tstart = isc_mem_get(mctx, tlength);
	memmove(tstart, mslab, reservelen);
	tcurrent = tstart + reservelen;
#if DNS_RDATASET_FIXED
	offsetbase = tcurrent;

	offsettable = isc_mem_cget(mctx, mcount, sizeof(unsigned int));
#endif /* if DNS_RDATASET_FIXED */

	/*
	 * Write the new count.
	 */
	*tcurrent++ = (tcount & 0xff00) >> 8;
	*tcurrent++ = (tcount & 0x00ff);

#if DNS_RDATASET_FIXED
	tcurrent += (4 * tcount);
#endif /* if DNS_RDATASET_FIXED */

	/*
	 * Copy the parts of mslab not in sslab.
	 */
	mcurrent = mslab + reservelen;
	mcount = get_uint16(mcurrent);
#if DNS_RDATASET_FIXED
	mcurrent += (4 * mcount);
#endif /* if DNS_RDATASET_FIXED */
	for (i = 0; i < mcount; i++) {
		unsigned char *mrdatabegin = mcurrent;
#if DNS_RDATASET_FIXED
		order = peek_uint16(&mcurrent[2]);
		INSIST(order < mcount);
#endif /* if DNS_RDATASET_FIXED */
		rdata_from_slab(&mcurrent, rdclass, type, &mrdata);
		scurrent = sstart;
		for (count = 0; count < scount; count++) {
			dns_rdata_reset(&srdata);
			rdata_from_slab(&scurrent, rdclass, type, &srdata);
			if (dns_rdata_compare(&mrdata, &srdata) == 0) {
				break;
			}
		}
		if (count == scount) {
			/*
			 * This rdata isn't in the sslab, and thus should be
			 * copied to the tslab.
			 */
			unsigned int length;
			length = (unsigned int)(mcurrent - mrdatabegin);
#if DNS_RDATASET_FIXED
			offsettable[order] = tcurrent - offsetbase;
#endif /* if DNS_RDATASET_FIXED */
			memmove(tcurrent, mrdatabegin, length);
			tcurrent += length;
		}
		dns_rdata_reset(&mrdata);
	}

#if DNS_RDATASET_FIXED
	fillin_offsets(offsetbase, offsettable, mcount);

	isc_mem_cput(mctx, offsettable, mcount, sizeof(unsigned int));
#endif /* if DNS_RDATASET_FIXED */

	INSIST(tcurrent == tstart + tlength);

	*tslabp = tstart;

	return ISC_R_SUCCESS;
}

bool
dns_rdataslab_equal(unsigned char *slab1, unsigned char *slab2,
		    unsigned int reservelen) {
	unsigned char *current1 = NULL, *current2 = NULL;
	unsigned int count1, count2;
	unsigned int length1, length2;

	current1 = slab1 + reservelen;
	count1 = get_uint16(current1);

	current2 = slab2 + reservelen;
	count2 = get_uint16(current2);

	if (count1 != count2) {
		return false;
	}

#if DNS_RDATASET_FIXED
	current1 += (4 * count1);
	current2 += (4 * count2);
#endif /* if DNS_RDATASET_FIXED */

	while (count1-- > 0) {
		length1 = get_uint16(current1);
		length2 = get_uint16(current2);

#if DNS_RDATASET_FIXED
		current1 += 2;
		current2 += 2;
#endif /* if DNS_RDATASET_FIXED */

		if (length1 != length2 ||
		    memcmp(current1, current2, length1) != 0)
		{
			return false;
		}

		current1 += length1;
		current2 += length1;
	}
	return true;
}

bool
dns_rdataslab_equalx(unsigned char *slab1, unsigned char *slab2,
		     unsigned int reservelen, dns_rdataclass_t rdclass,
		     dns_rdatatype_t type) {
	unsigned char *current1 = NULL, *current2 = NULL;
	unsigned int count1, count2;
	dns_rdata_t rdata1 = DNS_RDATA_INIT;
	dns_rdata_t rdata2 = DNS_RDATA_INIT;

	current1 = slab1 + reservelen;
	count1 = get_uint16(current1);

	current2 = slab2 + reservelen;
	count2 = get_uint16(current2);

	if (count1 != count2) {
		return false;
	}

#if DNS_RDATASET_FIXED
	current1 += (4 * count1);
	current2 += (4 * count2);
#endif /* if DNS_RDATASET_FIXED */

	while (count1-- > 0) {
		rdata_from_slab(&current1, rdclass, type, &rdata1);
		rdata_from_slab(&current2, rdclass, type, &rdata2);
		if (dns_rdata_compare(&rdata1, &rdata2) != 0) {
			return false;
		}
		dns_rdata_reset(&rdata1);
		dns_rdata_reset(&rdata2);
	}
	return true;
}

dns_slabheader_t *
dns_slabheader_fromrdataset(const dns_rdataset_t *rdataset) {
	dns_slabheader_t *header = (dns_slabheader_t *)rdataset->slab.raw;
	return header - 1;
}

void *
dns_slabheader_raw(dns_slabheader_t *header) {
	return header + 1;
}

void
dns_slabheader_setownercase(dns_slabheader_t *header, const dns_name_t *name) {
	unsigned int i;
	bool fully_lower;

	/*
	 * We do not need to worry about label lengths as they are all
	 * less than or equal to 63.
	 */
	memset(header->upper, 0, sizeof(header->upper));
	fully_lower = true;
	for (i = 0; i < name->length; i++) {
		if (isupper(name->ndata[i])) {
			header->upper[i / 8] |= 1 << (i % 8);
			fully_lower = false;
		}
	}
	DNS_SLABHEADER_SETATTR(header, DNS_SLABHEADERATTR_CASESET);
	if (fully_lower) {
		DNS_SLABHEADER_SETATTR(header,
				       DNS_SLABHEADERATTR_CASEFULLYLOWER);
	}
}

void
dns_slabheader_copycase(dns_slabheader_t *dest, dns_slabheader_t *src) {
	if (CASESET(src)) {
		uint_least16_t attr = DNS_SLABHEADER_GETATTR(
			src, (DNS_SLABHEADERATTR_CASESET |
			      DNS_SLABHEADERATTR_CASEFULLYLOWER));
		DNS_SLABHEADER_SETATTR(dest, attr);
		memmove(dest->upper, src->upper, sizeof(src->upper));
	}
}

void
dns_slabheader_reset(dns_slabheader_t *h, dns_db_t *db, dns_dbnode_t *node) {
	ISC_LINK_INIT(h, link);
	h->heap_index = 0;
	h->heap = NULL;
	h->db = db;
	h->node = node;

	atomic_init(&h->attributes, 0);
	atomic_init(&h->last_refresh_fail_ts, 0);

	STATIC_ASSERT((sizeof(h->attributes) == 2),
		      "The .attributes field of dns_slabheader_t needs to be "
		      "16-bit int type exactly.");
}

dns_slabheader_t *
dns_slabheader_new(dns_db_t *db, dns_dbnode_t *node) {
	dns_slabheader_t *h = NULL;

	h = isc_mem_get(db->mctx, sizeof(*h));
	*h = (dns_slabheader_t){
		.link = ISC_LINK_INITIALIZER,
	};
	dns_slabheader_reset(h, db, node);
	return h;
}

void
dns_slabheader_destroy(dns_slabheader_t **headerp) {
	unsigned int size;
	dns_slabheader_t *header = *headerp;

	*headerp = NULL;

	isc_mem_t *mctx = header->db->mctx;

	dns_db_deletedata(header->db, header->node, header);

	if (NONEXISTENT(header)) {
		size = sizeof(*header);
	} else {
		size = dns_rdataslab_size((unsigned char *)header,
					  sizeof(*header));
	}

	isc_mem_put(mctx, header, size);
}

void
dns_slabheader_freeproof(isc_mem_t *mctx, dns_slabheader_proof_t **proof) {
	if (dns_name_dynamic(&(*proof)->name)) {
		dns_name_free(&(*proof)->name, mctx);
	}
	if ((*proof)->neg != NULL) {
		isc_mem_put(mctx, (*proof)->neg,
			    dns_rdataslab_size((*proof)->neg, 0));
	}
	if ((*proof)->negsig != NULL) {
		isc_mem_put(mctx, (*proof)->negsig,
			    dns_rdataslab_size((*proof)->negsig, 0));
	}
	isc_mem_put(mctx, *proof, sizeof(**proof));
	*proof = NULL;
}

dns_rdatasetmethods_t dns_rdataslab_rdatasetmethods = {
	.disassociate = rdataset_disassociate,
	.first = rdataset_first,
	.next = rdataset_next,
	.current = rdataset_current,
	.clone = rdataset_clone,
	.count = rdataset_count,
	.getnoqname = rdataset_getnoqname,
	.getclosest = rdataset_getclosest,
	.settrust = rdataset_settrust,
	.expire = rdataset_expire,
	.clearprefetch = rdataset_clearprefetch,
	.setownercase = rdataset_setownercase,
	.getownercase = rdataset_getownercase,
};

/* Fixed RRSet helper macros */

#define DNS_RDATASET_LENGTH 2;

#if DNS_RDATASET_FIXED
#define DNS_RDATASET_ORDER 2
#define DNS_RDATASET_COUNT (count * 4)
#else /* !DNS_RDATASET_FIXED */
#define DNS_RDATASET_ORDER 0
#define DNS_RDATASET_COUNT 0
#endif /* DNS_RDATASET_FIXED */

static void
rdataset_disassociate(dns_rdataset_t *rdataset DNS__DB_FLARG) {
	dns_db_t *db = rdataset->slab.db;
	dns_dbnode_t *node = rdataset->slab.node;

	dns__db_detachnode(db, &node DNS__DB_FLARG_PASS);
}

static isc_result_t
rdataset_first(dns_rdataset_t *rdataset) {
	unsigned char *raw = rdataset->slab.raw;
	uint16_t count = peek_uint16(raw);
	if (count == 0) {
		rdataset->slab.iter_pos = NULL;
		rdataset->slab.iter_count = 0;
		return ISC_R_NOMORE;
	}

	if ((rdataset->attributes & DNS_RDATASETATTR_LOADORDER) == 0) {
		raw += DNS_RDATASET_COUNT;
	}

	/*
	 * iter_count is the number of rdata beyond the cursor
	 * position, so we decrement the total count by one before
	 * storing it.
	 *
	 * If DNS_RDATASETATTR_LOADORDER is not set 'raw' points to the
	 * first record.  If DNS_RDATASETATTR_LOADORDER is set 'raw' points
	 * to the first entry in the offset table.
	 */
	rdataset->slab.iter_pos = raw + DNS_RDATASET_LENGTH;
	rdataset->slab.iter_count = count - 1;

	return ISC_R_SUCCESS;
}

static isc_result_t
rdataset_next(dns_rdataset_t *rdataset) {
	uint16_t count = rdataset->slab.iter_count;
	if (count == 0) {
		rdataset->slab.iter_pos = NULL;
		return ISC_R_NOMORE;
	}
	rdataset->slab.iter_count = count - 1;

	/*
	 * Skip forward one record (length + 4) or one offset (4).
	 */
	unsigned char *raw = rdataset->slab.iter_pos;
#if DNS_RDATASET_FIXED
	if ((rdataset->attributes & DNS_RDATASETATTR_LOADORDER) == 0)
#endif /* DNS_RDATASET_FIXED */
	{
		uint16_t length = peek_uint16(raw);
		raw += length;
	}
	rdataset->slab.iter_pos = raw + DNS_RDATASET_ORDER +
				  DNS_RDATASET_LENGTH;

	return ISC_R_SUCCESS;
}

static void
rdataset_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata) {
	unsigned char *raw = NULL;
	unsigned int length;
	isc_region_t r;
	unsigned int flags = 0;

	raw = rdataset->slab.iter_pos;
	REQUIRE(raw != NULL);

	/*
	 * Find the start of the record if not already in iter_pos
	 * then skip the length and order fields.
	 */
#if DNS_RDATASET_FIXED
	if ((rdataset->attributes & DNS_RDATASETATTR_LOADORDER) != 0) {
		unsigned int offset;
		offset = ((unsigned int)raw[0] << 24) +
			 ((unsigned int)raw[1] << 16) +
			 ((unsigned int)raw[2] << 8) + (unsigned int)raw[3];
		raw = rdataset->slab.raw + offset;
	}
#endif /* if DNS_RDATASET_FIXED */

	length = peek_uint16(raw);

	raw += DNS_RDATASET_ORDER + DNS_RDATASET_LENGTH;

	if (rdataset->type == dns_rdatatype_rrsig) {
		if (*raw & DNS_RDATASLAB_OFFLINE) {
			flags |= DNS_RDATA_OFFLINE;
		}
		length--;
		raw++;
	}
	r.length = length;
	r.base = raw;
	dns_rdata_fromregion(rdata, rdataset->rdclass, rdataset->type, &r);
	rdata->flags |= flags;
}

static void
rdataset_clone(dns_rdataset_t *source, dns_rdataset_t *target DNS__DB_FLARG) {
	dns_db_t *db = source->slab.db;
	dns_dbnode_t *node = source->slab.node;
	dns_dbnode_t *cloned_node = NULL;

	dns__db_attachnode(db, node, &cloned_node DNS__DB_FLARG_PASS);
	INSIST(!ISC_LINK_LINKED(target, link));
	*target = *source;
	ISC_LINK_INIT(target, link);

	target->slab.iter_pos = NULL;
	target->slab.iter_count = 0;
}

static unsigned int
rdataset_count(dns_rdataset_t *rdataset) {
	unsigned char *raw = NULL;
	unsigned int count;

	raw = rdataset->slab.raw;
	count = get_uint16(raw);

	return count;
}

static isc_result_t
rdataset_getnoqname(dns_rdataset_t *rdataset, dns_name_t *name,
		    dns_rdataset_t *nsec,
		    dns_rdataset_t *nsecsig DNS__DB_FLARG) {
	dns_db_t *db = rdataset->slab.db;
	dns_dbnode_t *node = rdataset->slab.node;
	const dns_slabheader_proof_t *noqname = rdataset->slab.noqname;

	/*
	 * Usually, rdataset->slab.raw refers the data following a
	 * dns_slabheader, but in this case it points to a bare
	 * rdataslab belonging to the dns_slabheader's `noqname` field.
	 * The DNS_RDATASETATTR_KEEPCASE attribute is set to prevent
	 * setownercase and getownercase methods from affecting the
	 * case of NSEC/NSEC3 owner names.
	 */
	dns__db_attachnode(db, node,
			   &(dns_dbnode_t *){ NULL } DNS__DB_FLARG_PASS);
	*nsec = (dns_rdataset_t){
		.methods = &dns_rdataslab_rdatasetmethods,
		.rdclass = db->rdclass,
		.type = noqname->type,
		.ttl = rdataset->ttl,
		.trust = rdataset->trust,
		.slab.db = db,
		.slab.node = node,
		.slab.raw = noqname->neg,
		.link = nsec->link,
		.count = nsec->count,
		.attributes = nsec->attributes | DNS_RDATASETATTR_KEEPCASE,
		.magic = nsec->magic,
	};

	dns__db_attachnode(db, node,
			   &(dns_dbnode_t *){ NULL } DNS__DB_FLARG_PASS);
	*nsecsig = (dns_rdataset_t){
		.methods = &dns_rdataslab_rdatasetmethods,
		.rdclass = db->rdclass,
		.type = dns_rdatatype_rrsig,
		.covers = noqname->type,
		.ttl = rdataset->ttl,
		.trust = rdataset->trust,
		.slab.db = db,
		.slab.node = node,
		.slab.raw = noqname->negsig,
		.link = nsecsig->link,
		.count = nsecsig->count,
		.attributes = nsecsig->attributes | DNS_RDATASETATTR_KEEPCASE,
		.magic = nsecsig->magic,
	};

	dns_name_clone(&noqname->name, name);

	return ISC_R_SUCCESS;
}

static isc_result_t
rdataset_getclosest(dns_rdataset_t *rdataset, dns_name_t *name,
		    dns_rdataset_t *nsec,
		    dns_rdataset_t *nsecsig DNS__DB_FLARG) {
	dns_db_t *db = rdataset->slab.db;
	dns_dbnode_t *node = rdataset->slab.node;
	const dns_slabheader_proof_t *closest = rdataset->slab.closest;

	/*
	 * As mentioned above, rdataset->slab.raw usually refers the data
	 * following an dns_slabheader, but in this case it points to a bare
	 * rdataslab belonging to the dns_slabheader's `closest` field.
	 */
	dns__db_attachnode(db, node,
			   &(dns_dbnode_t *){ NULL } DNS__DB_FLARG_PASS);
	*nsec = (dns_rdataset_t){
		.methods = &dns_rdataslab_rdatasetmethods,
		.rdclass = db->rdclass,
		.type = closest->type,
		.ttl = rdataset->ttl,
		.trust = rdataset->trust,
		.slab.db = db,
		.slab.node = node,
		.slab.raw = closest->neg,
		.link = nsec->link,
		.count = nsec->count,
		.attributes = nsec->attributes | DNS_RDATASETATTR_KEEPCASE,
		.magic = nsec->magic,
	};

	dns__db_attachnode(db, node,
			   &(dns_dbnode_t *){ NULL } DNS__DB_FLARG_PASS);
	*nsecsig = (dns_rdataset_t){
		.methods = &dns_rdataslab_rdatasetmethods,
		.rdclass = db->rdclass,
		.type = dns_rdatatype_rrsig,
		.covers = closest->type,
		.ttl = rdataset->ttl,
		.trust = rdataset->trust,
		.slab.db = db,
		.slab.node = node,
		.slab.raw = closest->negsig,
		.link = nsecsig->link,
		.count = nsecsig->count,
		.attributes = nsecsig->attributes | DNS_RDATASETATTR_KEEPCASE,
		.magic = nsecsig->magic,
	};

	dns_name_clone(&closest->name, name);

	return ISC_R_SUCCESS;
}

static void
rdataset_settrust(dns_rdataset_t *rdataset, dns_trust_t trust) {
	dns_slabheader_t *header = dns_slabheader_fromrdataset(rdataset);

	dns_db_locknode(header->db, header->node, isc_rwlocktype_write);
	header->trust = rdataset->trust = trust;
	dns_db_unlocknode(header->db, header->node, isc_rwlocktype_write);
}

static void
rdataset_expire(dns_rdataset_t *rdataset DNS__DB_FLARG) {
	dns_slabheader_t *header = dns_slabheader_fromrdataset(rdataset);

	dns_db_expiredata(header->db, header->node, header);
}

static void
rdataset_clearprefetch(dns_rdataset_t *rdataset) {
	dns_slabheader_t *header = dns_slabheader_fromrdataset(rdataset);

	dns_db_locknode(header->db, header->node, isc_rwlocktype_write);
	DNS_SLABHEADER_CLRATTR(header, DNS_SLABHEADERATTR_PREFETCH);
	dns_db_unlocknode(header->db, header->node, isc_rwlocktype_write);
}

static void
rdataset_setownercase(dns_rdataset_t *rdataset, const dns_name_t *name) {
	dns_slabheader_t *header = dns_slabheader_fromrdataset(rdataset);

	dns_db_locknode(header->db, header->node, isc_rwlocktype_write);
	dns_slabheader_setownercase(header, name);
	dns_db_unlocknode(header->db, header->node, isc_rwlocktype_write);
}

static void
rdataset_getownercase(const dns_rdataset_t *rdataset, dns_name_t *name) {
	dns_slabheader_t *header = dns_slabheader_fromrdataset(rdataset);
	uint8_t mask = (1 << 7);
	uint8_t bits = 0;

	dns_db_locknode(header->db, header->node, isc_rwlocktype_read);

	if (!CASESET(header)) {
		goto unlock;
	}

	if (CASEFULLYLOWER(header)) {
		isc_ascii_lowercopy(name->ndata, name->ndata, name->length);
	} else {
		uint8_t *nd = name->ndata;
		for (size_t i = 0; i < name->length; i++) {
			if (mask == (1 << 7)) {
				bits = header->upper[i / 8];
				mask = 1;
			} else {
				mask <<= 1;
			}
			nd[i] = (bits & mask) ? isc_ascii_toupper(nd[i])
					      : isc_ascii_tolower(nd[i]);
		}
	}

unlock:
	dns_db_unlocknode(header->db, header->node, isc_rwlocktype_read);
}
