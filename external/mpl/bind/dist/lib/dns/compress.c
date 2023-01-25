/*	$NetBSD: compress.c,v 1.8 2023/01/25 21:43:30 christos Exp $	*/

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

#define DNS_NAME_USEINLINE 1

#include <inttypes.h>
#include <stdbool.h>

#include <isc/mem.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/compress.h>
#include <dns/fixedname.h>
#include <dns/rbt.h>
#include <dns/result.h>

#define CCTX_MAGIC    ISC_MAGIC('C', 'C', 'T', 'X')
#define VALID_CCTX(x) ISC_MAGIC_VALID(x, CCTX_MAGIC)

#define DCTX_MAGIC    ISC_MAGIC('D', 'C', 'T', 'X')
#define VALID_DCTX(x) ISC_MAGIC_VALID(x, DCTX_MAGIC)

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

/*
 * The tableindex array below is of size 256, one entry for each
 * unsigned char value. The tableindex array elements are dependent on
 * DNS_COMPRESS_TABLESIZE. The table was created using the following
 * function.
 *
 * static void
 * gentable(unsigned char *table) {
 *         unsigned int i;
 *         const unsigned int left = DNS_COMPRESS_TABLESIZE - 38;
 *         long r;
 *
 *         for (i = 0; i < 26; i++) {
 *                 table['A' + i] = i;
 *                 table['a' + i] = i;
 *         }
 *
 *         for (i = 0; i <= 9; i++)
 *                 table['0' + i] = i + 26;
 *
 *         table['-'] = 36;
 *         table['_'] = 37;
 *
 *         for (i = 0; i < 256; i++) {
 *                 if ((i >= 'a' && i <= 'z') ||
 *                     (i >= 'A' && i <= 'Z') ||
 *                     (i >= '0' && i <= '9') ||
 *                     (i == '-') ||
 *                     (i == '_'))
 *                         continue;
 *                 r = random() % left;
 *                 table[i] = 38 + r;
 *         }
 * }
 */
static unsigned char tableindex[256] = {
	0x3e, 0x3e, 0x33, 0x2d, 0x30, 0x38, 0x31, 0x3c, 0x2b, 0x33, 0x30, 0x3f,
	0x2d, 0x3c, 0x36, 0x3a, 0x28, 0x2c, 0x2a, 0x37, 0x3d, 0x34, 0x35, 0x2d,
	0x39, 0x2b, 0x2f, 0x2c, 0x3b, 0x32, 0x2b, 0x39, 0x30, 0x38, 0x28, 0x3c,
	0x32, 0x33, 0x39, 0x38, 0x27, 0x2b, 0x39, 0x30, 0x27, 0x24, 0x2f, 0x2b,
	0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x3a, 0x29, 0x36,
	0x31, 0x3c, 0x35, 0x26, 0x31, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
	0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12,
	0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x3e, 0x3b, 0x39, 0x2f, 0x25,
	0x27, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
	0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
	0x17, 0x18, 0x19, 0x36, 0x3b, 0x2f, 0x2f, 0x2e, 0x29, 0x33, 0x2a, 0x36,
	0x28, 0x3f, 0x2e, 0x29, 0x2c, 0x29, 0x36, 0x2d, 0x32, 0x3d, 0x33, 0x2a,
	0x2e, 0x2f, 0x3b, 0x30, 0x3d, 0x39, 0x2b, 0x36, 0x2a, 0x2f, 0x2c, 0x26,
	0x3a, 0x37, 0x30, 0x3d, 0x2a, 0x36, 0x33, 0x2c, 0x38, 0x3d, 0x32, 0x3e,
	0x26, 0x2a, 0x2c, 0x35, 0x27, 0x39, 0x3b, 0x31, 0x2a, 0x37, 0x3c, 0x27,
	0x32, 0x29, 0x39, 0x37, 0x34, 0x3f, 0x39, 0x2e, 0x38, 0x2b, 0x2c, 0x3e,
	0x3b, 0x3b, 0x2d, 0x33, 0x3b, 0x3b, 0x32, 0x3d, 0x3f, 0x3a, 0x34, 0x26,
	0x35, 0x30, 0x31, 0x39, 0x27, 0x2f, 0x3d, 0x35, 0x35, 0x36, 0x2e, 0x29,
	0x38, 0x27, 0x34, 0x32, 0x2c, 0x3c, 0x31, 0x28, 0x37, 0x38, 0x37, 0x34,
	0x33, 0x29, 0x32, 0x34, 0x3f, 0x26, 0x34, 0x34, 0x32, 0x27, 0x30, 0x33,
	0x33, 0x2d, 0x2b, 0x28, 0x3f, 0x33, 0x2b, 0x39, 0x37, 0x39, 0x2c, 0x3d,
	0x35, 0x39, 0x27, 0x2f
};

/***
 ***	Compression
 ***/

isc_result_t
dns_compress_init(dns_compress_t *cctx, int edns, isc_mem_t *mctx) {
	REQUIRE(cctx != NULL);
	REQUIRE(mctx != NULL); /* See: rdataset.c:towiresorted(). */

	cctx->edns = edns;
	cctx->mctx = mctx;
	cctx->count = 0;
	cctx->allowed = DNS_COMPRESS_ENABLED;
	cctx->arena_off = 0;

	memset(&cctx->table[0], 0, sizeof(cctx->table));

	cctx->magic = CCTX_MAGIC;

	return (ISC_R_SUCCESS);
}

void
dns_compress_invalidate(dns_compress_t *cctx) {
	dns_compressnode_t *node;
	unsigned int i;

	REQUIRE(VALID_CCTX(cctx));

	for (i = 0; i < DNS_COMPRESS_TABLESIZE; i++) {
		while (cctx->table[i] != NULL) {
			node = cctx->table[i];
			cctx->table[i] = cctx->table[i]->next;
			if ((node->offset & 0x8000) != 0) {
				isc_mem_put(cctx->mctx, node->r.base,
					    node->r.length);
			}
			if (node->count < DNS_COMPRESS_INITIALNODES) {
				continue;
			}
			isc_mem_put(cctx->mctx, node, sizeof(*node));
		}
	}

	cctx->magic = 0;
	cctx->allowed = 0;
	cctx->edns = -1;
}

void
dns_compress_setmethods(dns_compress_t *cctx, unsigned int allowed) {
	REQUIRE(VALID_CCTX(cctx));

	cctx->allowed &= ~DNS_COMPRESS_ALL;
	cctx->allowed |= (allowed & DNS_COMPRESS_ALL);
}

unsigned int
dns_compress_getmethods(dns_compress_t *cctx) {
	REQUIRE(VALID_CCTX(cctx));
	return (cctx->allowed & DNS_COMPRESS_ALL);
}

void
dns_compress_disable(dns_compress_t *cctx) {
	REQUIRE(VALID_CCTX(cctx));
	cctx->allowed &= ~DNS_COMPRESS_ENABLED;
}

void
dns_compress_setsensitive(dns_compress_t *cctx, bool sensitive) {
	REQUIRE(VALID_CCTX(cctx));

	if (sensitive) {
		cctx->allowed |= DNS_COMPRESS_CASESENSITIVE;
	} else {
		cctx->allowed &= ~DNS_COMPRESS_CASESENSITIVE;
	}
}

bool
dns_compress_getsensitive(dns_compress_t *cctx) {
	REQUIRE(VALID_CCTX(cctx));

	return (cctx->allowed & DNS_COMPRESS_CASESENSITIVE);
}

int
dns_compress_getedns(dns_compress_t *cctx) {
	REQUIRE(VALID_CCTX(cctx));
	return (cctx->edns);
}

/*
 * Find the longest match of name in the table.
 * If match is found return true. prefix, suffix and offset are updated.
 * If no match is found return false.
 */
bool
dns_compress_findglobal(dns_compress_t *cctx, const dns_name_t *name,
			dns_name_t *prefix, uint16_t *offset) {
	dns_name_t tname;
	dns_compressnode_t *node = NULL;
	unsigned int labels, i, n;
	unsigned int numlabels;
	unsigned char *p;

	REQUIRE(VALID_CCTX(cctx));
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(offset != NULL);

	if (ISC_UNLIKELY((cctx->allowed & DNS_COMPRESS_ENABLED) == 0)) {
		return (false);
	}

	if (cctx->count == 0) {
		return (false);
	}

	labels = dns_name_countlabels(name);
	INSIST(labels > 0);

	dns_name_init(&tname, NULL);

	numlabels = labels > 3U ? 3U : labels;
	p = name->ndata;

	for (n = 0; n < numlabels - 1; n++) {
		unsigned char ch, llen;
		unsigned int firstoffset, length;

		firstoffset = (unsigned int)(p - name->ndata);
		length = name->length - firstoffset;

		/*
		 * We calculate the table index using the first
		 * character in the first label of the suffix name.
		 */
		ch = p[1];
		i = tableindex[ch];
		if (ISC_LIKELY((cctx->allowed & DNS_COMPRESS_CASESENSITIVE) !=
			       0))
		{
			for (node = cctx->table[i]; node != NULL;
			     node = node->next)
			{
				if (ISC_UNLIKELY(node->name.length != length)) {
					continue;
				}

				if (ISC_LIKELY(memcmp(node->name.ndata, p,
						      length) == 0))
				{
					goto found;
				}
			}
		} else {
			for (node = cctx->table[i]; node != NULL;
			     node = node->next)
			{
				unsigned int l, count;
				unsigned char c;
				unsigned char *label1, *label2;

				if (ISC_UNLIKELY(node->name.length != length)) {
					continue;
				}

				l = labels - n;
				if (ISC_UNLIKELY(node->name.labels != l)) {
					continue;
				}

				label1 = node->name.ndata;
				label2 = p;
				while (ISC_LIKELY(l-- > 0)) {
					count = *label1++;
					if (count != *label2++) {
						goto cont1;
					}

					/* no bitstring support */
					INSIST(count <= 63);

					/* Loop unrolled for performance */
					while (ISC_LIKELY(count > 3)) {
						c = maptolower[label1[0]];
						if (c != maptolower[label2[0]])
						{
							goto cont1;
						}
						c = maptolower[label1[1]];
						if (c != maptolower[label2[1]])
						{
							goto cont1;
						}
						c = maptolower[label1[2]];
						if (c != maptolower[label2[2]])
						{
							goto cont1;
						}
						c = maptolower[label1[3]];
						if (c != maptolower[label2[3]])
						{
							goto cont1;
						}
						count -= 4;
						label1 += 4;
						label2 += 4;
					}
					while (ISC_LIKELY(count-- > 0)) {
						c = maptolower[*label1++];
						if (c != maptolower[*label2++])
						{
							goto cont1;
						}
					}
				}
				break;
			cont1:
				continue;
			}
		}

		if (node != NULL) {
			break;
		}

		llen = *p;
		p += llen + 1;
	}

found:
	/*
	 * If node == NULL, we found no match at all.
	 */
	if (node == NULL) {
		return (false);
	}

	if (n == 0) {
		dns_name_reset(prefix);
	} else {
		dns_name_getlabelsequence(name, 0, n, prefix);
	}

	*offset = (node->offset & 0x7fff);
	return (true);
}

static unsigned int
name_length(const dns_name_t *name) {
	isc_region_t r;
	dns_name_toregion(name, &r);
	return (r.length);
}

void
dns_compress_add(dns_compress_t *cctx, const dns_name_t *name,
		 const dns_name_t *prefix, uint16_t offset) {
	dns_name_t tname, xname;
	unsigned int start;
	unsigned int n;
	unsigned int count;
	unsigned int i;
	dns_compressnode_t *node;
	unsigned int length;
	unsigned int tlength;
	uint16_t toffset;
	unsigned char *tmp;
	isc_region_t r;
	bool allocated = false;

	REQUIRE(VALID_CCTX(cctx));
	REQUIRE(dns_name_isabsolute(name));

	if (ISC_UNLIKELY((cctx->allowed & DNS_COMPRESS_ENABLED) == 0)) {
		return;
	}

	if (offset >= 0x4000) {
		return;
	}
	dns_name_init(&tname, NULL);
	dns_name_init(&xname, NULL);

	n = dns_name_countlabels(name);
	count = dns_name_countlabels(prefix);
	if (dns_name_isabsolute(prefix)) {
		count--;
	}
	if (count == 0) {
		return;
	}
	start = 0;
	dns_name_toregion(name, &r);
	length = r.length;
	if (cctx->arena_off + length < DNS_COMPRESS_ARENA_SIZE) {
		tmp = &cctx->arena[cctx->arena_off];
		cctx->arena_off += length;
	} else {
		allocated = true;
		tmp = isc_mem_get(cctx->mctx, length);
	}
	/*
	 * Copy name data to 'tmp' and make 'r' use 'tmp'.
	 */
	memmove(tmp, r.base, r.length);
	r.base = tmp;
	dns_name_fromregion(&xname, &r);

	if (count > 2U) {
		count = 2U;
	}

	while (count > 0) {
		unsigned char ch;

		dns_name_getlabelsequence(&xname, start, n, &tname);
		/*
		 * We calculate the table index using the first
		 * character in the first label of tname.
		 */
		ch = tname.ndata[1];
		i = tableindex[ch];
		tlength = name_length(&tname);
		toffset = (uint16_t)(offset + (length - tlength));
		if (toffset >= 0x4000) {
			break;
		}
		/*
		 * Create a new node and add it.
		 */
		if (cctx->count < DNS_COMPRESS_INITIALNODES) {
			node = &cctx->initialnodes[cctx->count];
		} else {
			node = isc_mem_get(cctx->mctx,
					   sizeof(dns_compressnode_t));
		}
		node->count = cctx->count++;
		/*
		 * 'node->r.base' becomes 'tmp' when start == 0.
		 * Record this by setting 0x8000 so it can be freed later.
		 */
		if (start == 0 && allocated) {
			toffset |= 0x8000;
		}
		node->offset = toffset;
		dns_name_toregion(&tname, &node->r);
		dns_name_init(&node->name, NULL);
		node->name.length = node->r.length;
		node->name.ndata = node->r.base;
		node->name.labels = tname.labels;
		node->name.attributes = DNS_NAMEATTR_ABSOLUTE;
		node->next = cctx->table[i];
		cctx->table[i] = node;
		start++;
		n--;
		count--;
	}

	if (start == 0) {
		if (!allocated) {
			cctx->arena_off -= length;
		} else {
			isc_mem_put(cctx->mctx, tmp, length);
		}
	}
}

void
dns_compress_rollback(dns_compress_t *cctx, uint16_t offset) {
	unsigned int i;
	dns_compressnode_t *node;

	REQUIRE(VALID_CCTX(cctx));

	if (ISC_UNLIKELY((cctx->allowed & DNS_COMPRESS_ENABLED) == 0)) {
		return;
	}

	for (i = 0; i < DNS_COMPRESS_TABLESIZE; i++) {
		node = cctx->table[i];
		/*
		 * This relies on nodes with greater offsets being
		 * closer to the beginning of the list, and the
		 * items with the greatest offsets being at the end
		 * of the initialnodes[] array.
		 */
		while (node != NULL && (node->offset & 0x7fff) >= offset) {
			cctx->table[i] = node->next;
			if ((node->offset & 0x8000) != 0) {
				isc_mem_put(cctx->mctx, node->r.base,
					    node->r.length);
			}
			if (node->count >= DNS_COMPRESS_INITIALNODES) {
				isc_mem_put(cctx->mctx, node, sizeof(*node));
			}
			cctx->count--;
			node = cctx->table[i];
		}
	}
}

/***
 ***	Decompression
 ***/

void
dns_decompress_init(dns_decompress_t *dctx, int edns,
		    dns_decompresstype_t type) {
	REQUIRE(dctx != NULL);
	REQUIRE(edns >= -1 && edns <= 255);

	dctx->allowed = DNS_COMPRESS_NONE;
	dctx->edns = edns;
	dctx->type = type;
	dctx->magic = DCTX_MAGIC;
}

void
dns_decompress_invalidate(dns_decompress_t *dctx) {
	REQUIRE(VALID_DCTX(dctx));

	dctx->magic = 0;
}

void
dns_decompress_setmethods(dns_decompress_t *dctx, unsigned int allowed) {
	REQUIRE(VALID_DCTX(dctx));

	switch (dctx->type) {
	case DNS_DECOMPRESS_ANY:
		dctx->allowed = DNS_COMPRESS_ALL;
		break;
	case DNS_DECOMPRESS_NONE:
		dctx->allowed = DNS_COMPRESS_NONE;
		break;
	case DNS_DECOMPRESS_STRICT:
		dctx->allowed = allowed;
		break;
	}
}

unsigned int
dns_decompress_getmethods(dns_decompress_t *dctx) {
	REQUIRE(VALID_DCTX(dctx));

	return (dctx->allowed);
}

int
dns_decompress_edns(dns_decompress_t *dctx) {
	REQUIRE(VALID_DCTX(dctx));

	return (dctx->edns);
}

dns_decompresstype_t
dns_decompress_type(dns_decompress_t *dctx) {
	REQUIRE(VALID_DCTX(dctx));

	return (dctx->type);
}
