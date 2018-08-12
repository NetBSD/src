/*	$NetBSD: compress.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef DNS_COMPRESS_H
#define DNS_COMPRESS_H 1

#include <isc/lang.h>
#include <isc/region.h>

#include <dns/types.h>
#include <dns/name.h>

ISC_LANG_BEGINDECLS

/*! \file dns/compress.h
 * Direct manipulation of the structures is strongly discouraged.
 *
 * A name compression context handles compression of multiple DNS names
 * in relation to a single DNS message. The context can be used to
 * selectively turn on/off compression for specific names (depending on
 * the RR type) by using \c dns_compress_setmethods(). Alternately,
 * compression can be disabled completely using \c
 * dns_compress_disable().
 *
 * \c dns_compress_setmethods() is intended for use by RDATA towire()
 * implementations, whereas \c dns_compress_disable() is intended to be
 * used by a nameserver's configuration manager.
 */

#define DNS_COMPRESS_NONE		0x00	/*%< no compression */
#define DNS_COMPRESS_GLOBAL14		0x01	/*%< "normal" compression. */
#define DNS_COMPRESS_ALL		0x01	/*%< all compression. */
#define DNS_COMPRESS_CASESENSITIVE	0x02	/*%< case sensitive compression. */
#define DNS_COMPRESS_ENABLED		0x04

/*
 * DNS_COMPRESS_TABLESIZE must be a power of 2. The compress code
 * utilizes this assumption.
 */
#define DNS_COMPRESS_TABLEBITS 6
#define DNS_COMPRESS_TABLESIZE (1U << DNS_COMPRESS_TABLEBITS)
#define DNS_COMPRESS_TABLEMASK (DNS_COMPRESS_TABLESIZE - 1)
#define DNS_COMPRESS_INITIALNODES 16

typedef struct dns_compressnode dns_compressnode_t;

struct dns_compressnode {
	dns_compressnode_t	*next;
	isc_uint16_t		offset;
	isc_uint16_t		count;
	isc_region_t            r;
	dns_name_t              name;
};

struct dns_compress {
	unsigned int		magic;		/*%< Magic number. */
	unsigned int		allowed;	/*%< Allowed methods. */
	int			edns;		/*%< Edns version or -1. */
	/*% Global compression table. */
	dns_compressnode_t	*table[DNS_COMPRESS_TABLESIZE];
	/*% Preallocated nodes for the table. */
	dns_compressnode_t	initialnodes[DNS_COMPRESS_INITIALNODES];
	isc_uint16_t		count;		/*%< Number of nodes. */
	isc_mem_t		*mctx;		/*%< Memory context. */
};

typedef enum {
	DNS_DECOMPRESS_ANY,			/*%< Any compression */
	DNS_DECOMPRESS_STRICT,			/*%< Allowed compression */
	DNS_DECOMPRESS_NONE			/*%< No compression */
} dns_decompresstype_t;

struct dns_decompress {
	unsigned int		magic;		/*%< Magic number. */
	unsigned int		allowed;	/*%< Allowed methods. */
	int			edns;		/*%< Edns version or -1. */
	dns_decompresstype_t	type;		/*%< Strict checking */
};

isc_result_t
dns_compress_init(dns_compress_t *cctx, int edns, isc_mem_t *mctx);
/*%<
 *	Initialise the compression context structure pointed to by
 *	'cctx'. A freshly initialized context has name compression
 *	enabled, but no methods are set. Please use \c
 *	dns_compress_setmethods() to set a compression method.
 *
 *	Requires:
 *	\li	'cctx' is a valid dns_compress_t structure.
 *	\li	'mctx' is an initialized memory context.
 *	Ensures:
 *	\li	cctx->global is initialized.
 *
 *	Returns:
 *	\li	#ISC_R_SUCCESS
 */

void
dns_compress_invalidate(dns_compress_t *cctx);

/*%<
 *	Invalidate the compression structure pointed to by cctx.
 *
 *	Requires:
 *\li		'cctx' to be initialized.
 */

void
dns_compress_setmethods(dns_compress_t *cctx, unsigned int allowed);

/*%<
 *	Sets allowed compression methods.
 *
 *	Requires:
 *\li		'cctx' to be initialized.
 */

unsigned int
dns_compress_getmethods(dns_compress_t *cctx);

/*%<
 *	Gets allowed compression methods.
 *
 *	Requires:
 *\li		'cctx' to be initialized.
 *
 *	Returns:
 *\li		allowed compression bitmap.
 */

void
dns_compress_disable(dns_compress_t *cctx);
/*%<
 *	Disables all name compression in the context. Once disabled,
 *	name compression cannot currently be re-enabled.
 *
 *	Requires:
 *\li		'cctx' to be initialized.
 *
 */

void
dns_compress_setsensitive(dns_compress_t *cctx, isc_boolean_t sensitive);

/*
 *	Preserve the case of compressed domain names.
 *
 *	Requires:
 *		'cctx' to be initialized.
 */

isc_boolean_t
dns_compress_getsensitive(dns_compress_t *cctx);
/*
 *	Return whether case is to be preserved when compressing
 *	domain names.
 *
 *	Requires:
 *		'cctx' to be initialized.
 */

int
dns_compress_getedns(dns_compress_t *cctx);

/*%<
 *	Gets edns value.
 *
 *	Requires:
 *\li		'cctx' to be initialized.
 *
 *	Returns:
 *\li		-1 .. 255
 */

isc_boolean_t
dns_compress_findglobal(dns_compress_t *cctx, const dns_name_t *name,
			dns_name_t *prefix, isc_uint16_t *offset);
/*%<
 *	Finds longest possible match of 'name' in the global compression table.
 *
 *	Requires:
 *\li		'cctx' to be initialized.
 *\li		'name' to be a absolute name.
 *\li		'prefix' to be initialized.
 *\li		'offset' to point to an isc_uint16_t.
 *
 *	Ensures:
 *\li		'prefix' and 'offset' are valid if ISC_TRUE is 	returned.
 *
 *	Returns:
 *\li		#ISC_TRUE / #ISC_FALSE
 */

void
dns_compress_add(dns_compress_t *cctx, const dns_name_t *name,
		 const dns_name_t *prefix, isc_uint16_t offset);
/*%<
 *	Add compression pointers for 'name' to the compression table,
 *	not replacing existing pointers.
 *
 *	Requires:
 *\li		'cctx' initialized
 *
 *\li		'name' must be initialized and absolute, and must remain
 *		valid until the message compression is complete.
 *
 *\li		'prefix' must be a prefix returned by
 *		dns_compress_findglobal(), or the same as 'name'.
 */

void
dns_compress_rollback(dns_compress_t *cctx, isc_uint16_t offset);

/*%<
 *	Remove any compression pointers from global table >= offset.
 *
 *	Requires:
 *\li		'cctx' is initialized.
 */

void
dns_decompress_init(dns_decompress_t *dctx, int edns,
		    dns_decompresstype_t type);

/*%<
 *	Initializes 'dctx'.
 *	Records 'edns' and 'type' into the structure.
 *
 *	Requires:
 *\li		'dctx' to be a valid pointer.
 */

void
dns_decompress_invalidate(dns_decompress_t *dctx);

/*%<
 *	Invalidates 'dctx'.
 *
 *	Requires:
 *\li		'dctx' to be initialized
 */

void
dns_decompress_setmethods(dns_decompress_t *dctx, unsigned int allowed);

/*%<
 *	Sets 'dctx->allowed' to 'allowed'.
 *
 *	Requires:
 *\li		'dctx' to be initialized
 */

unsigned int
dns_decompress_getmethods(dns_decompress_t *dctx);

/*%<
 *	Returns 'dctx->allowed'
 *
 *	Requires:
 *\li		'dctx' to be initialized
 */

int
dns_decompress_edns(dns_decompress_t *dctx);

/*%<
 *	Returns 'dctx->edns'
 *
 *	Requires:
 *\li		'dctx' to be initialized
 */

dns_decompresstype_t
dns_decompress_type(dns_decompress_t *dctx);

/*%<
 *	Returns 'dctx->type'
 *
 *	Requires:
 *\li		'dctx' to be initialized
 */

ISC_LANG_ENDDECLS

#endif /* DNS_COMPRESS_H */
