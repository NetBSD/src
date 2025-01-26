/*	$NetBSD: builtin.c,v 1.8 2025/01/26 16:24:33 christos Exp $	*/

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

/*! \file
 * \brief
 * The built-in "version", "hostname", "id", "authors" and "empty" databases.
 */

#include <stdio.h>
#include <string.h>

#include <isc/lex.h>
#include <isc/mem.h>
#include <isc/result.h>
#include <isc/util.h>

#include <dns/callbacks.h>
#include <dns/dbiterator.h>
#include <dns/rdatalist.h>
#include <dns/rdatasetiter.h>
#include <dns/types.h>

#include <named/builtin.h>
#include <named/globals.h>
#include <named/os.h>
#include <named/server.h>

#define BDBNODE_MAGIC	    ISC_MAGIC('B', 'D', 'B', 'N')
#define VALID_BDBNODE(bdbl) ISC_MAGIC_VALID(bdbl, BDBNODE_MAGIC)

/*%
 * Note that "impmagic" is not the first four bytes of the struct, so
 * ISC_MAGIC_VALID cannot be used here.
 */
#define BDB_MAGIC      ISC_MAGIC('B', 'D', 'B', '-')
#define VALID_BDB(bdb) ((bdb) != NULL && (bdb)->common.impmagic == BDB_MAGIC)

#define BDB_DNS64 0x00000001U

typedef struct bdbimplementation {
	unsigned int flags;
	dns_dbimplementation_t *dbimp;
} bdbimplementation_t;

typedef struct bdbnode bdbnode_t;
typedef struct bdb {
	dns_db_t common;
	bdbimplementation_t *implementation;
	isc_result_t (*lookup)(bdbnode_t *node);
	char *server;
	char *contact;
} bdb_t;

struct bdbnode {
	unsigned int magic;
	isc_refcount_t references;
	bdb_t *bdb;
	ISC_LIST(dns_rdatalist_t) lists;
	ISC_LIST(isc_buffer_t) buffers;
	dns_name_t *name;
	ISC_LINK(bdbnode_t) link;
	dns_rdatacallbacks_t callbacks;
};

typedef struct bdb_rdatasetiter {
	dns_rdatasetiter_t common;
	dns_rdatalist_t *current;
} bdb_rdatasetiter_t;

static isc_result_t
findrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	     dns_rdatatype_t type, dns_rdatatype_t covers, isc_stdtime_t now,
	     dns_rdataset_t *rdataset,
	     dns_rdataset_t *sigrdataset DNS__DB_FLARG);

static void
attachnode(dns_db_t *db, dns_dbnode_t *source,
	   dns_dbnode_t **targetp DNS__DB_FLARG);

static void
detachnode(dns_db_t *db, dns_dbnode_t **nodep DNS__DB_FLARG);

/*
 * Helper functions to convert text to wire forma.
 */
static isc_result_t
putrdata(bdbnode_t *node, dns_rdatatype_t typeval, dns_ttl_t ttl,
	 const unsigned char *rdatap, unsigned int rdlen) {
	dns_rdatalist_t *rdatalist = NULL;
	dns_rdata_t *rdata = NULL;
	isc_buffer_t *rdatabuf = NULL;
	isc_mem_t *mctx = NULL;
	isc_region_t region;

	mctx = node->bdb->common.mctx;

	rdatalist = ISC_LIST_HEAD(node->lists);
	while (rdatalist != NULL) {
		if (rdatalist->type == typeval) {
			break;
		}
		rdatalist = ISC_LIST_NEXT(rdatalist, link);
	}

	if (rdatalist == NULL) {
		rdatalist = isc_mem_get(mctx, sizeof(dns_rdatalist_t));
		dns_rdatalist_init(rdatalist);
		rdatalist->rdclass = node->bdb->common.rdclass;
		rdatalist->type = typeval;
		rdatalist->ttl = ttl;
		ISC_LIST_APPEND(node->lists, rdatalist, link);
	} else if (rdatalist->ttl != ttl) {
		return DNS_R_BADTTL;
	}

	rdata = isc_mem_get(mctx, sizeof(dns_rdata_t));

	isc_buffer_allocate(mctx, &rdatabuf, rdlen);
	region.base = UNCONST(rdatap);
	region.length = rdlen;
	isc_buffer_copyregion(rdatabuf, &region);
	isc_buffer_usedregion(rdatabuf, &region);
	dns_rdata_init(rdata);
	dns_rdata_fromregion(rdata, rdatalist->rdclass, rdatalist->type,
			     &region);
	ISC_LIST_APPEND(rdatalist->rdata, rdata, link);
	ISC_LIST_APPEND(node->buffers, rdatabuf, link);

	return ISC_R_SUCCESS;
}

static isc_result_t
putrr(bdbnode_t *node, const char *type, dns_ttl_t ttl, const char *data) {
	isc_result_t result;
	dns_rdatatype_t typeval;
	isc_lex_t *lex = NULL;
	isc_mem_t *mctx = NULL;
	const dns_name_t *origin = NULL;
	isc_buffer_t *rb = NULL;
	isc_buffer_t b;

	REQUIRE(VALID_BDBNODE(node));
	REQUIRE(type != NULL);
	REQUIRE(data != NULL);

	mctx = node->bdb->common.mctx;
	origin = &node->bdb->common.origin;

	isc_constregion_t r = { .base = type, .length = strlen(type) };
	result = dns_rdatatype_fromtext(&typeval, (isc_textregion_t *)&r);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	isc_lex_create(mctx, 64, &lex);

	size_t datalen = strlen(data);
	isc_buffer_constinit(&b, data, datalen);
	isc_buffer_add(&b, datalen);

	result = isc_lex_openbuffer(lex, &b);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	isc_buffer_allocate(mctx, &rb, DNS_RDATA_MAXLENGTH);
	result = dns_rdata_fromtext(NULL, node->bdb->common.rdclass, typeval,
				    lex, origin, 0, mctx, rb, &node->callbacks);
	isc_lex_destroy(&lex);

	if (result == ISC_R_SUCCESS) {
		result = putrdata(node, typeval, ttl, isc_buffer_base(rb),
				  isc_buffer_usedlength(rb));
	}

	isc_buffer_free(&rb);

	return result;
}

/* Reasonable default SOA values */
#define DEFAULT_REFRESH 28800U	/* 8 hours */
#define DEFAULT_RETRY	7200U	/* 2 hours */
#define DEFAULT_EXPIRE	604800U /* 7 days */
#define DEFAULT_MINIMUM 86400U	/* 1 day */
#define DEFAULT_TTL	(60 * 60 * 24)

static isc_result_t
putsoa(bdbnode_t *node, const char *mname, const char *rname, uint32_t serial) {
	char str[2 * DNS_NAME_MAXTEXT + 5 * (sizeof("2147483647")) + 7];
	int n;

	REQUIRE(mname != NULL);
	REQUIRE(rname != NULL);

	n = snprintf(str, sizeof(str), "%s %s %u %u %u %u %u", mname, rname,
		     serial, DEFAULT_REFRESH, DEFAULT_RETRY, DEFAULT_EXPIRE,
		     DEFAULT_MINIMUM);
	if (n >= (int)sizeof(str) || n < 0) {
		return ISC_R_NOSPACE;
	}
	return putrr(node, "SOA", DEFAULT_TTL, str);
}

static isc_result_t
puttxt(bdbnode_t *node, const char *text) {
	unsigned char buf[256];
	unsigned int len = strlen(text);

	if (len > 255) {
		len = 255; /* Silently truncate */
	}
	buf[0] = len;
	memmove(&buf[1], text, len);
	return putrdata(node, dns_rdatatype_txt, 0, buf, len + 1);
}

/*
 * Builtin database implementation functions.
 */

/* Precomputed HEX * 16 or 1 table. */
static const unsigned char hex16[256] = {
	1, 1,	1,   1,	  1,   1,   1,	 1,   1,   1,	1, 1, 1, 1, 1, 1, /*00*/
	1, 1,	1,   1,	  1,   1,   1,	 1,   1,   1,	1, 1, 1, 1, 1, 1, /*10*/
	1, 1,	1,   1,	  1,   1,   1,	 1,   1,   1,	1, 1, 1, 1, 1, 1, /*20*/
	0, 16,	32,  48,  64,  80,  96,	 112, 128, 144, 1, 1, 1, 1, 1, 1, /*30*/
	1, 160, 176, 192, 208, 224, 240, 1,   1,   1,	1, 1, 1, 1, 1, 1, /*40*/
	1, 1,	1,   1,	  1,   1,   1,	 1,   1,   1,	1, 1, 1, 1, 1, 1, /*50*/
	1, 160, 176, 192, 208, 224, 240, 1,   1,   1,	1, 1, 1, 1, 1, 1, /*60*/
	1, 1,	1,   1,	  1,   1,   1,	 1,   1,   1,	1, 1, 1, 1, 1, 1, /*70*/
	1, 1,	1,   1,	  1,   1,   1,	 1,   1,   1,	1, 1, 1, 1, 1, 1, /*80*/
	1, 1,	1,   1,	  1,   1,   1,	 1,   1,   1,	1, 1, 1, 1, 1, 1, /*90*/
	1, 1,	1,   1,	  1,   1,   1,	 1,   1,   1,	1, 1, 1, 1, 1, 1, /*A0*/
	1, 1,	1,   1,	  1,   1,   1,	 1,   1,   1,	1, 1, 1, 1, 1, 1, /*B0*/
	1, 1,	1,   1,	  1,   1,   1,	 1,   1,   1,	1, 1, 1, 1, 1, 1, /*C0*/
	1, 1,	1,   1,	  1,   1,   1,	 1,   1,   1,	1, 1, 1, 1, 1, 1, /*D0*/
	1, 1,	1,   1,	  1,   1,   1,	 1,   1,   1,	1, 1, 1, 1, 1, 1, /*E0*/
	1, 1,	1,   1,	  1,   1,   1,	 1,   1,   1,	1, 1, 1, 1, 1, 1  /*F0*/
};

static const unsigned char decimal[] = "0123456789";
static const unsigned char ipv4only[] = "\010ipv4only\004arpa";

static size_t
dns64_rdata(unsigned char *v, size_t start, unsigned char *rdata) {
	size_t i, j = 0;

	for (i = 0; i < 4U; i++) {
		unsigned char c = v[start++];
		if (start == 7U) {
			start++;
		}
		if (c > 99) {
			rdata[j++] = 3;
			rdata[j++] = decimal[c / 100];
			c = c % 100;
			rdata[j++] = decimal[c / 10];
			c = c % 10;
			rdata[j++] = decimal[c];
		} else if (c > 9) {
			rdata[j++] = 2;
			rdata[j++] = decimal[c / 10];
			c = c % 10;
			rdata[j++] = decimal[c];
		} else {
			rdata[j++] = 1;
			rdata[j++] = decimal[c];
		}
	}
	memmove(&rdata[j], "\07in-addr\04arpa", 14);
	return j + 14;
}

static isc_result_t
dns64_cname(const dns_name_t *zone, const dns_name_t *name, bdbnode_t *node) {
	size_t zlen, nlen, j, len;
	unsigned char v[16], n;
	unsigned int i;
	unsigned char rdata[sizeof("123.123.123.123.in-addr.arpa.")];
	unsigned char *ndata;

	/*
	 * The combined length of the zone and name is 74.
	 *
	 * The minimum zone length is 10 ((3)ip6(4)arpa(0)).
	 *
	 * The length of name should always be even as we are expecting
	 * a series of nibbles.
	 */
	zlen = zone->length;
	nlen = name->length;
	if ((zlen + nlen) > 74U || zlen < 10U || (nlen % 2) != 0U) {
		return ISC_R_NOTFOUND;
	}

	/*
	 * Check that name is a series of nibbles.
	 * Compute the byte values that correspond to the nibbles as we go.
	 *
	 * Shift the final result 4 bits, by setting 'i' to 1, if we if we
	 * have a odd number of nibbles so that "must be zero" tests below
	 * are byte aligned and we correctly return ISC_R_NOTFOUND or
	 * ISC_R_SUCCESS.  We will not generate a CNAME in this case.
	 */
	ndata = name->ndata;
	i = (nlen % 4) == 2U ? 1 : 0;
	j = nlen;
	memset(v, 0, sizeof(v));
	while (j != 0U) {
		INSIST((i / 2) < sizeof(v));
		if (ndata[0] != 1) {
			return ISC_R_NOTFOUND;
		}
		n = hex16[ndata[1] & 0xff];
		if (n == 1) {
			return ISC_R_NOTFOUND;
		}
		v[i / 2] = n | (v[i / 2] >> 4);
		j -= 2;
		ndata += 2;
		i++;
	}

	/*
	 * If we get here then we know name only consisted of nibbles.
	 * Now we need to determine if the name exists or not and whether
	 * it corresponds to a empty node in the zone or there should be
	 * a CNAME.
	 */
#define ZLEN(x) (10 + (x) / 2)
	switch (zlen) {
	case ZLEN(32): /* prefix len 32 */
		/*
		 * The nibbles that map to this byte must be zero for 'name'
		 * to exist in the zone.
		 */
		if (nlen > 16U && v[(nlen - 1) / 4 - 4] != 0) {
			return ISC_R_NOTFOUND;
		}
		/*
		 * If the total length is not 74 then this is a empty node
		 * so return success.
		 */
		if (nlen + zlen != 74U) {
			return ISC_R_SUCCESS;
		}
		len = dns64_rdata(v, 8, rdata);
		break;
	case ZLEN(40): /* prefix len 40 */
		/*
		 * The nibbles that map to this byte must be zero for 'name'
		 * to exist in the zone.
		 */
		if (nlen > 12U && v[(nlen - 1) / 4 - 3] != 0) {
			return ISC_R_NOTFOUND;
		}
		/*
		 * If the total length is not 74 then this is a empty node
		 * so return success.
		 */
		if (nlen + zlen != 74U) {
			return ISC_R_SUCCESS;
		}
		len = dns64_rdata(v, 6, rdata);
		break;
	case ZLEN(48): /* prefix len 48 */
		/*
		 * The nibbles that map to this byte must be zero for 'name'
		 * to exist in the zone.
		 */
		if (nlen > 8U && v[(nlen - 1) / 4 - 2] != 0) {
			return ISC_R_NOTFOUND;
		}
		/*
		 * If the total length is not 74 then this is a empty node
		 * so return success.
		 */
		if (nlen + zlen != 74U) {
			return ISC_R_SUCCESS;
		}
		len = dns64_rdata(v, 5, rdata);
		break;
	case ZLEN(56): /* prefix len 56 */
		/*
		 * The nibbles that map to this byte must be zero for 'name'
		 * to exist in the zone.
		 */
		if (nlen > 4U && v[(nlen - 1) / 4 - 1] != 0) {
			return ISC_R_NOTFOUND;
		}
		/*
		 * If the total length is not 74 then this is a empty node
		 * so return success.
		 */
		if (nlen + zlen != 74U) {
			return ISC_R_SUCCESS;
		}
		len = dns64_rdata(v, 4, rdata);
		break;
	case ZLEN(64): /* prefix len 64 */
		/*
		 * The nibbles that map to this byte must be zero for 'name'
		 * to exist in the zone.
		 */
		if (v[(nlen - 1) / 4] != 0) {
			return ISC_R_NOTFOUND;
		}
		/*
		 * If the total length is not 74 then this is a empty node
		 * so return success.
		 */
		if (nlen + zlen != 74U) {
			return ISC_R_SUCCESS;
		}
		len = dns64_rdata(v, 3, rdata);
		break;
	case ZLEN(96): /* prefix len 96 */
		/*
		 * If the total length is not 74 then this is a empty node
		 * so return success.
		 */
		if (nlen + zlen != 74U) {
			return ISC_R_SUCCESS;
		}
		len = dns64_rdata(v, 0, rdata);
		break;
	default:
		/*
		 * This should never be reached unless someone adds a
		 * zone declaration with this internal type to named.conf.
		 */
		return ISC_R_NOTFOUND;
	}

	/*
	 * Reverse of 192.0.0.170 or 192.0.0.171 maps to ipv4only.arpa.
	 */
	if ((v[0] == 170 || v[0] == 171) && v[1] == 0 && v[2] == 0 &&
	    v[3] == 192)
	{
		return putrdata(node, dns_rdatatype_ptr, 3600, ipv4only,
				sizeof(ipv4only));
	}

	return putrdata(node, dns_rdatatype_cname, 600, rdata,
			(unsigned int)len);
}

static isc_result_t
builtin_lookup(bdb_t *bdb, const dns_name_t *name, bdbnode_t *node) {
	if (name->labels == 0 && name->length == 0) {
		return bdb->lookup(node);
	} else if ((node->bdb->implementation->flags & BDB_DNS64) != 0) {
		return dns64_cname(&bdb->common.origin, name, node);
	} else {
		return ISC_R_NOTFOUND;
	}
}

static isc_result_t
builtin_authority(bdb_t *bdb, bdbnode_t *node) {
	isc_result_t result;
	const char *contact = "hostmaster";
	const char *server = "@";

	if (bdb->server != NULL) {
		server = bdb->server;
	}
	if (bdb->contact != NULL) {
		contact = bdb->contact;
	}

	result = putsoa(node, server, contact, 0);
	if (result != ISC_R_SUCCESS) {
		return ISC_R_FAILURE;
	}

	result = putrr(node, "NS", 0, server);
	if (result != ISC_R_SUCCESS) {
		return ISC_R_FAILURE;
	}

	return ISC_R_SUCCESS;
}

static isc_result_t
version_lookup(bdbnode_t *node) {
	if (named_g_server->version_set) {
		if (named_g_server->version == NULL) {
			return ISC_R_SUCCESS;
		} else {
			return puttxt(node, named_g_server->version);
		}
	} else {
		return puttxt(node, PACKAGE_VERSION);
	}
}

static isc_result_t
hostname_lookup(bdbnode_t *node) {
	if (named_g_server->hostname_set) {
		if (named_g_server->hostname == NULL) {
			return ISC_R_SUCCESS;
		} else {
			return puttxt(node, named_g_server->hostname);
		}
	} else {
		char buf[256];
		if (gethostname(buf, sizeof(buf)) != 0) {
			return ISC_R_FAILURE;
		}
		return puttxt(node, buf);
	}
}

static isc_result_t
authors_lookup(bdbnode_t *node) {
	isc_result_t result;
	const char **p = NULL;
	static const char *authors[] = {
		"Mark Andrews",	      "Curtis Blackburn",
		"James Brister",      "Ben Cottrell",
		"John H. DuBois III", "Francis Dupont",
		"Michael Graff",      "Andreas Gustafsson",
		"Bob Halley",	      "Evan Hunt",
		"JINMEI Tatuya",      "Witold Krecicki",
		"David Lawrence",     "Scott Mann",
		"Danny Mayer",	      "Aydin Mercan",
		"Damien Neil",	      "Matt Nelson",
		"Jeremy C. Reed",     "Michael Sawyer",
		"Brian Wellington",   NULL
	};

	/*
	 * If a version string is specified, disable the authors.bind zone.
	 */
	if (named_g_server->version_set) {
		return ISC_R_SUCCESS;
	}

	for (p = authors; *p != NULL; p++) {
		result = puttxt(node, *p);
		if (result != ISC_R_SUCCESS) {
			return result;
		}
	}
	return ISC_R_SUCCESS;
}

static isc_result_t
id_lookup(bdbnode_t *node) {
	if (named_g_server->sctx->usehostname) {
		char buf[256];
		if (gethostname(buf, sizeof(buf)) != 0) {
			return ISC_R_FAILURE;
		}
		return puttxt(node, buf);
	} else if (named_g_server->sctx->server_id != NULL) {
		return puttxt(node, named_g_server->sctx->server_id);
	} else {
		return ISC_R_SUCCESS;
	}
}

static isc_result_t
empty_lookup(bdbnode_t *node) {
	UNUSED(node);

	return ISC_R_SUCCESS;
}

static isc_result_t
ipv4only_lookup(bdbnode_t *node) {
	isc_result_t result;
	unsigned char data[2][4] = { { 192, 0, 0, 170 }, { 192, 0, 0, 171 } };

	for (int i = 0; i < 2; i++) {
		result = putrdata(node, dns_rdatatype_a, 3600, data[i], 4);
		if (result != ISC_R_SUCCESS) {
			return result;
		}
	}
	return ISC_R_SUCCESS;
}

static isc_result_t
ipv4reverse_lookup(bdbnode_t *node) {
	isc_result_t result;

	result = putrdata(node, dns_rdatatype_ptr, 3600, ipv4only,
			  sizeof(ipv4only));
	return result;
}

/*
 * Rdataset implementation methods. An rdataset in the builtin databases is
 * implemented as an rdatalist which holds a reference to the dbnode,
 * to prevent the node being freed while the rdataset is still in use, so
 * we need local implementations of clone and disassociate but the rest of
 * the implementation can be the same as dns_rdatalist..
 */
static void
disassociate(dns_rdataset_t *rdataset DNS__DB_FLARG) {
	dns_dbnode_t *node = rdataset->rdlist.node;
	bdbnode_t *bdbnode = (bdbnode_t *)node;
	dns_db_t *db = (dns_db_t *)bdbnode->bdb;

	detachnode(db, &node DNS__DB_FLARG_PASS);
	dns_rdatalist_disassociate(rdataset DNS__DB_FLARG_PASS);
}

static void
rdataset_clone(dns_rdataset_t *source, dns_rdataset_t *target DNS__DB_FLARG) {
	dns_dbnode_t *node = source->rdlist.node;
	bdbnode_t *bdbnode = (bdbnode_t *)node;
	dns_db_t *db = (dns_db_t *)bdbnode->bdb;

	dns_rdatalist_clone(source, target DNS__DB_FLARG_PASS);
	attachnode(db, node, &target->rdlist.node DNS__DB_FLARG_PASS);
}

static dns_rdatasetmethods_t bdb_rdataset_methods = {
	.disassociate = disassociate,
	.first = dns_rdatalist_first,
	.next = dns_rdatalist_next,
	.current = dns_rdatalist_current,
	.clone = rdataset_clone,
	.count = dns_rdatalist_count,
	.addnoqname = dns_rdatalist_addnoqname,
	.getnoqname = dns_rdatalist_getnoqname,
};

static void
new_rdataset(dns_rdatalist_t *rdatalist, dns_db_t *db, dns_dbnode_t *node,
	     dns_rdataset_t *rdataset) {
	dns_rdatalist_tordataset(rdatalist, rdataset);

	rdataset->methods = &bdb_rdataset_methods;
	dns_db_attachnode(db, node, &rdataset->rdlist.node);
}

/*
 * Rdataset iterator methods
 */

static void
rdatasetiter_destroy(dns_rdatasetiter_t **iteratorp DNS__DB_FLARG) {
	bdb_rdatasetiter_t *bdbiterator = (bdb_rdatasetiter_t *)(*iteratorp);
	detachnode(bdbiterator->common.db,
		   &bdbiterator->common.node DNS__DB_FLARG_PASS);
	isc_mem_put(bdbiterator->common.db->mctx, bdbiterator,
		    sizeof(bdb_rdatasetiter_t));
	*iteratorp = NULL;
}

static isc_result_t
rdatasetiter_first(dns_rdatasetiter_t *iterator DNS__DB_FLARG) {
	bdb_rdatasetiter_t *bdbiterator = (bdb_rdatasetiter_t *)iterator;
	bdbnode_t *bdbnode = (bdbnode_t *)iterator->node;

	if (ISC_LIST_EMPTY(bdbnode->lists)) {
		return ISC_R_NOMORE;
	}
	bdbiterator->current = ISC_LIST_HEAD(bdbnode->lists);
	return ISC_R_SUCCESS;
}

static isc_result_t
rdatasetiter_next(dns_rdatasetiter_t *iterator DNS__DB_FLARG) {
	bdb_rdatasetiter_t *bdbiterator = (bdb_rdatasetiter_t *)iterator;

	bdbiterator->current = ISC_LIST_NEXT(bdbiterator->current, link);
	if (bdbiterator->current == NULL) {
		return ISC_R_NOMORE;
	} else {
		return ISC_R_SUCCESS;
	}
}

static void
rdatasetiter_current(dns_rdatasetiter_t *iterator,
		     dns_rdataset_t *rdataset DNS__DB_FLARG) {
	bdb_rdatasetiter_t *bdbiterator = (bdb_rdatasetiter_t *)iterator;

	new_rdataset(bdbiterator->current, iterator->db, iterator->node,
		     rdataset);
}

static dns_rdatasetitermethods_t rdatasetiter_methods = {
	rdatasetiter_destroy, rdatasetiter_first, rdatasetiter_next,
	rdatasetiter_current
};

/*
 * Database implementation methods
 */
static void
destroy(dns_db_t *db) {
	bdb_t *bdb = (bdb_t *)db;
	isc_refcount_destroy(&bdb->common.references);

	if (bdb->server != NULL) {
		isc_mem_free(named_g_mctx, bdb->server);
	}
	if (bdb->contact != NULL) {
		isc_mem_free(named_g_mctx, bdb->contact);
	}

	bdb->common.magic = 0;
	bdb->common.impmagic = 0;

	dns_name_free(&bdb->common.origin, bdb->common.mctx);

	isc_mem_putanddetach(&bdb->common.mctx, bdb, sizeof(bdb_t));
}

/*
 * A dummy 'version' value is used so that dns_db_createversion()
 * can return a non-NULL version to the caller, but there can only be
 * one version of these databases, so the version value is never used.
 */
static int dummy;

static void
currentversion(dns_db_t *db, dns_dbversion_t **versionp) {
	bdb_t *bdb = (bdb_t *)db;

	REQUIRE(VALID_BDB(bdb));

	*versionp = (void *)&dummy;
	return;
}

static void
attachversion(dns_db_t *db, dns_dbversion_t *source,
	      dns_dbversion_t **targetp) {
	bdb_t *bdb = (bdb_t *)db;

	REQUIRE(VALID_BDB(bdb));
	REQUIRE(source != NULL && source == (void *)&dummy);
	REQUIRE(targetp != NULL && *targetp == NULL);

	*targetp = source;
	return;
}

static void
closeversion(dns_db_t *db, dns_dbversion_t **versionp,
	     bool commit DNS__DB_FLARG) {
	bdb_t *bdb = (bdb_t *)db;

	REQUIRE(VALID_BDB(bdb));
	REQUIRE(versionp != NULL && *versionp == (void *)&dummy);
	REQUIRE(!commit);

	*versionp = NULL;
}

static isc_result_t
createnode(bdb_t *bdb, bdbnode_t **nodep) {
	bdbnode_t *node = NULL;

	REQUIRE(VALID_BDB(bdb));

	node = isc_mem_get(bdb->common.mctx, sizeof(bdbnode_t));
	*node = (bdbnode_t){
		.lists = ISC_LIST_INITIALIZER,
		.buffers = ISC_LIST_INITIALIZER,
		.link = ISC_LINK_INITIALIZER,
	};

	dns_db_attach((dns_db_t *)bdb, (dns_db_t **)&node->bdb);
	dns_rdatacallbacks_init(&node->callbacks);

	isc_refcount_init(&node->references, 1);
	node->magic = BDBNODE_MAGIC;

	*nodep = node;
	return ISC_R_SUCCESS;
}

static void
destroynode(bdbnode_t *node) {
	dns_rdatalist_t *list = NULL;
	dns_rdata_t *rdata = NULL;
	isc_buffer_t *b = NULL;
	bdb_t *bdb = NULL;
	isc_mem_t *mctx = NULL;

	bdb = node->bdb;
	mctx = bdb->common.mctx;

	while (!ISC_LIST_EMPTY(node->lists)) {
		list = ISC_LIST_HEAD(node->lists);
		while (!ISC_LIST_EMPTY(list->rdata)) {
			rdata = ISC_LIST_HEAD(list->rdata);
			ISC_LIST_UNLINK(list->rdata, rdata, link);
			isc_mem_put(mctx, rdata, sizeof(dns_rdata_t));
		}
		ISC_LIST_UNLINK(node->lists, list, link);
		isc_mem_put(mctx, list, sizeof(dns_rdatalist_t));
	}

	while (!ISC_LIST_EMPTY(node->buffers)) {
		b = ISC_LIST_HEAD(node->buffers);
		ISC_LIST_UNLINK(node->buffers, b, link);
		isc_buffer_free(&b);
	}

	if (node->name != NULL) {
		dns_name_free(node->name, mctx);
		isc_mem_put(mctx, node->name, sizeof(dns_name_t));
	}

	node->magic = 0;
	isc_mem_put(mctx, node, sizeof(bdbnode_t));
	dns_db_detach((dns_db_t **)(void *)&bdb);
}

static isc_result_t
getoriginnode(dns_db_t *db, dns_dbnode_t **nodep DNS__DB_FLARG) {
	bdb_t *bdb = (bdb_t *)db;
	bdbnode_t *node = NULL;
	isc_result_t result;
	dns_name_t relname;
	dns_name_t *name = NULL;

	REQUIRE(VALID_BDB(bdb));
	REQUIRE(nodep != NULL && *nodep == NULL);

	dns_name_init(&relname, NULL);
	name = &relname;

	result = createnode(bdb, &node);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	result = builtin_lookup(bdb, name, node);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOTFOUND) {
		destroynode(node);
		return result;
	}

	result = builtin_authority(bdb, node);
	if (result != ISC_R_SUCCESS) {
		destroynode(node);
		return result;
	}

	*nodep = node;
	return ISC_R_SUCCESS;
}

static isc_result_t
findnode(dns_db_t *db, const dns_name_t *name, bool create,
	 dns_dbnode_t **nodep DNS__DB_FLARG) {
	bdb_t *bdb = (bdb_t *)db;
	bdbnode_t *node = NULL;
	isc_result_t result;
	bool isorigin;
	dns_name_t relname;
	unsigned int labels;

	REQUIRE(VALID_BDB(bdb));
	REQUIRE(nodep != NULL && *nodep == NULL);

	UNUSED(create);

	isorigin = dns_name_equal(name, &bdb->common.origin);

	labels = dns_name_countlabels(name) - dns_name_countlabels(&db->origin);
	dns_name_init(&relname, NULL);
	dns_name_getlabelsequence(name, 0, labels, &relname);
	name = &relname;

	result = createnode(bdb, &node);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	result = builtin_lookup(bdb, name, node);
	if (result != ISC_R_SUCCESS && (!isorigin || result != ISC_R_NOTFOUND))
	{
		destroynode(node);
		return result;
	}

	if (isorigin) {
		result = builtin_authority(bdb, node);
		if (result != ISC_R_SUCCESS) {
			destroynode(node);
			return result;
		}
	}

	*nodep = node;
	return ISC_R_SUCCESS;
}

static isc_result_t
find(dns_db_t *db, const dns_name_t *name, dns_dbversion_t *version,
     dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
     dns_dbnode_t **nodep, dns_name_t *foundname, dns_rdataset_t *rdataset,
     dns_rdataset_t *sigrdataset DNS__DB_FLARG) {
	bdb_t *bdb = (bdb_t *)db;
	isc_result_t result;
	dns_dbnode_t *node = NULL;
	dns_fixedname_t fname;
	dns_rdataset_t xrdataset;
	dns_name_t *xname = NULL;
	unsigned int nlabels, olabels, i;
	bool dns64;

	REQUIRE(VALID_BDB(bdb));
	REQUIRE(nodep == NULL || *nodep == NULL);
	REQUIRE(version == NULL || version == (void *)&dummy);

	if (!dns_name_issubdomain(name, &db->origin)) {
		return DNS_R_NXDOMAIN;
	}

	olabels = dns_name_countlabels(&db->origin);
	nlabels = dns_name_countlabels(name);

	xname = dns_fixedname_initname(&fname);

	if (rdataset == NULL) {
		dns_rdataset_init(&xrdataset);
		rdataset = &xrdataset;
	}

	result = DNS_R_NXDOMAIN;
	dns64 = ((bdb->implementation->flags & BDB_DNS64) != 0);
	for (i = (dns64 ? nlabels : olabels); i <= nlabels; i++) {
		/*
		 * Look up the next label.
		 */
		dns_name_getlabelsequence(name, nlabels - i, i, xname);
		result = findnode(db, xname, false, &node DNS__DB_FLARG_PASS);
		if (result == ISC_R_NOTFOUND) {
			/*
			 * No data at zone apex?
			 */
			if (i == olabels) {
				return DNS_R_BADDB;
			}
			result = DNS_R_NXDOMAIN;
			continue;
		}
		if (result != ISC_R_SUCCESS) {
			return result;
		}

		/*
		 * DNS64 zones don't have DNAME or NS records.
		 */
		if (dns64) {
			goto skip;
		}

		/*
		 * Look for a DNAME at the current label, unless this is
		 * the qname.
		 */
		if (i < nlabels) {
			result = findrdataset(
				db, node, version, dns_rdatatype_dname, 0, now,
				rdataset, sigrdataset DNS__DB_FLARG_PASS);
			if (result == ISC_R_SUCCESS) {
				result = DNS_R_DNAME;
				break;
			}
		}

		/*
		 * Look for an NS at the current label, unless this is the
		 * origin or glue is ok.
		 */
		if (i != olabels && (options & DNS_DBFIND_GLUEOK) == 0) {
			result = findrdataset(
				db, node, version, dns_rdatatype_ns, 0, now,
				rdataset, sigrdataset DNS__DB_FLARG_PASS);
			if (result == ISC_R_SUCCESS) {
				if (i == nlabels && type == dns_rdatatype_any) {
					result = DNS_R_ZONECUT;
					dns_rdataset_disassociate(rdataset);
					if (sigrdataset != NULL &&
					    dns_rdataset_isassociated(
						    sigrdataset))
					{
						dns_rdataset_disassociate(
							sigrdataset);
					}
				} else {
					result = DNS_R_DELEGATION;
				}
				break;
			}
		}

		/*
		 * If the current name is not the qname, add another label
		 * and try again.
		 */
		if (i < nlabels) {
			destroynode(node);
			node = NULL;
			continue;
		}

	skip:
		/*
		 * If we're looking for ANY, we're done.
		 */
		if (type == dns_rdatatype_any) {
			result = ISC_R_SUCCESS;
			break;
		}

		/*
		 * Look for the qtype.
		 */
		result = findrdataset(db, node, version, type, 0, now, rdataset,
				      sigrdataset DNS__DB_FLARG_PASS);
		if (result == ISC_R_SUCCESS) {
			break;
		}

		/*
		 * Look for a CNAME.
		 */
		if (type != dns_rdatatype_cname) {
			result = findrdataset(
				db, node, version, dns_rdatatype_cname, 0, now,
				rdataset, sigrdataset DNS__DB_FLARG_PASS);
			if (result == ISC_R_SUCCESS) {
				result = DNS_R_CNAME;
				break;
			}
		}

		result = DNS_R_NXRRSET;
		break;
	}

	if (rdataset == &xrdataset && dns_rdataset_isassociated(rdataset)) {
		dns_rdataset_disassociate(rdataset);
	}

	if (foundname != NULL) {
		dns_name_copy(xname, foundname);
	}

	if (nodep != NULL) {
		*nodep = node;
	} else if (node != NULL) {
		detachnode(db, &node DNS__DB_FLARG_PASS);
	}

	return result;
}

static void
attachnode(dns_db_t *db, dns_dbnode_t *source,
	   dns_dbnode_t **targetp DNS__DB_FLARG) {
	bdb_t *bdb = (bdb_t *)db;
	bdbnode_t *node = (bdbnode_t *)source;

	REQUIRE(VALID_BDB(bdb));

	isc_refcount_increment(&node->references);

	*targetp = source;
}

static void
detachnode(dns_db_t *db, dns_dbnode_t **nodep DNS__DB_FLARG) {
	bdb_t *bdb = (bdb_t *)db;
	bdbnode_t *node = NULL;

	REQUIRE(VALID_BDB(bdb));
	REQUIRE(nodep != NULL && *nodep != NULL);

	node = (bdbnode_t *)(*nodep);
	*nodep = NULL;

	if (isc_refcount_decrement(&node->references) == 1) {
		destroynode(node);
	}
}

static isc_result_t
findrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	     dns_rdatatype_t type, dns_rdatatype_t covers, isc_stdtime_t now,
	     dns_rdataset_t *rdataset,
	     dns_rdataset_t *sigrdataset DNS__DB_FLARG) {
	bdbnode_t *bdbnode = (bdbnode_t *)node;
	dns_rdatalist_t *list = NULL;

	REQUIRE(VALID_BDBNODE(bdbnode));

	UNUSED(version);
	UNUSED(covers);
	UNUSED(now);
	UNUSED(sigrdataset);

	if (type == dns_rdatatype_rrsig) {
		return ISC_R_NOTIMPLEMENTED;
	}

	list = ISC_LIST_HEAD(bdbnode->lists);
	while (list != NULL) {
		if (list->type == type) {
			break;
		}
		list = ISC_LIST_NEXT(list, link);
	}
	if (list == NULL) {
		return ISC_R_NOTFOUND;
	}

	new_rdataset(list, db, node, rdataset);

	return ISC_R_SUCCESS;
}

static isc_result_t
allrdatasets(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	     unsigned int options, isc_stdtime_t now,
	     dns_rdatasetiter_t **iteratorp DNS__DB_FLARG) {
	bdb_rdatasetiter_t *iterator = NULL;

	REQUIRE(version == NULL || version == &dummy);

	iterator = isc_mem_get(db->mctx, sizeof(bdb_rdatasetiter_t));
	*iterator = (bdb_rdatasetiter_t){
		.common.methods = &rdatasetiter_methods,
		.common.db = db,
		.common.version = version,
		.common.options = options,
		.common.now = now,
		.common.magic = DNS_RDATASETITER_MAGIC,
	};

	attachnode(db, node, &iterator->common.node DNS__DB_FLARG_PASS);

	*iteratorp = (dns_rdatasetiter_t *)iterator;

	return ISC_R_SUCCESS;
}

static dns_dbmethods_t bdb_methods = {
	.destroy = destroy,
	.currentversion = currentversion,
	.attachversion = attachversion,
	.closeversion = closeversion,
	.attachnode = attachnode,
	.detachnode = detachnode,
	.findrdataset = findrdataset,
	.allrdatasets = allrdatasets,
	.getoriginnode = getoriginnode,
	.findnode = findnode,
	.find = find,
};

static isc_result_t
create(isc_mem_t *mctx, const dns_name_t *origin, dns_dbtype_t type,
       dns_rdataclass_t rdclass, unsigned int argc, char *argv[],
       void *implementation, dns_db_t **dbp) {
	isc_result_t result;
	bool needargs = false;
	bdb_t *bdb = NULL;

	REQUIRE(implementation != NULL);

	if (type != dns_dbtype_zone) {
		return ISC_R_NOTIMPLEMENTED;
	}

	bdb = isc_mem_get(mctx, sizeof(*bdb));
	*bdb = (bdb_t){
		.common = { .methods = &bdb_methods, .rdclass = rdclass },
		.implementation = implementation,
	};

	isc_refcount_init(&bdb->common.references, 1);
	isc_mem_attach(mctx, &bdb->common.mctx);
	dns_name_init(&bdb->common.origin, NULL);
	dns_name_dupwithoffsets(origin, mctx, &bdb->common.origin);

	INSIST(argc >= 1);
	if (strcmp(argv[0], "authors") == 0) {
		bdb->lookup = authors_lookup;
	} else if (strcmp(argv[0], "hostname") == 0) {
		bdb->lookup = hostname_lookup;
	} else if (strcmp(argv[0], "id") == 0) {
		bdb->lookup = id_lookup;
	} else if (strcmp(argv[0], "version") == 0) {
		bdb->lookup = version_lookup;
	} else if (strcmp(argv[0], "dns64") == 0) {
		needargs = true;
		bdb->lookup = empty_lookup;
	} else if (strcmp(argv[0], "empty") == 0) {
		needargs = true;
		bdb->lookup = empty_lookup;
	} else if (strcmp(argv[0], "ipv4only") == 0) {
		needargs = true;
		bdb->lookup = ipv4only_lookup;
	} else {
		needargs = true;
		bdb->lookup = ipv4reverse_lookup;
	}

	if (needargs) {
		if (argc != 3) {
			result = DNS_R_SYNTAX;
			goto cleanup;
		}

		bdb->server = isc_mem_strdup(named_g_mctx, argv[1]);
		bdb->contact = isc_mem_strdup(named_g_mctx, argv[2]);
	} else if (argc != 1) {
		result = DNS_R_SYNTAX;
		goto cleanup;
	}

	bdb->common.magic = DNS_DB_MAGIC;
	bdb->common.impmagic = BDB_MAGIC;

	*dbp = (dns_db_t *)bdb;

	return ISC_R_SUCCESS;

cleanup:
	dns_name_free(&bdb->common.origin, mctx);
	if (bdb->server != NULL) {
		isc_mem_free(named_g_mctx, bdb->server);
	}
	if (bdb->contact != NULL) {
		isc_mem_free(named_g_mctx, bdb->contact);
	}

	isc_mem_putanddetach(&bdb->common.mctx, bdb, sizeof(bdb_t));
	return result;
}

/*
 * Builtin database registration functions
 */
static bdbimplementation_t builtin = { .flags = 0 };
static bdbimplementation_t dns64 = { .flags = BDB_DNS64 };

isc_result_t
named_builtin_init(void) {
	isc_result_t result;

	result = dns_db_register("_builtin", create, &builtin, named_g_mctx,
				 &builtin.dbimp);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	result = dns_db_register("_dns64", create, &dns64, named_g_mctx,
				 &dns64.dbimp);
	if (result != ISC_R_SUCCESS) {
		dns_db_unregister(&builtin.dbimp);
		return result;
	}

	return ISC_R_SUCCESS;
}

void
named_builtin_deinit(void) {
	if (builtin.dbimp != NULL) {
		dns_db_unregister(&builtin.dbimp);
	}
	if (dns64.dbimp != NULL) {
		dns_db_unregister(&dns64.dbimp);
	}
}
