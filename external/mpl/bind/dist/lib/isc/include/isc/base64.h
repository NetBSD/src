/*	$NetBSD: base64.h,v 1.1.1.1 2018/08/12 12:08:26 christos Exp $	*/

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


#ifndef ISC_BASE64_H
#define ISC_BASE64_H 1

/*! \file isc/base64.h */

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

/***
 *** Functions
 ***/

isc_result_t
isc_base64_totext(isc_region_t *source, int wordlength,
		  const char *wordbreak, isc_buffer_t *target);
/*!<
 * \brief Convert data into base64 encoded text.
 *
 * Notes:
 *\li	The base64 encoded text in 'target' will be divided into
 *	words of at most 'wordlength' characters, separated by
 * 	the 'wordbreak' string.  No parentheses will surround
 *	the text.
 *
 * Requires:
 *\li	'source' is a region containing binary data
 *\li	'target' is a text buffer containing available space
 *\li	'wordbreak' points to a null-terminated string of
 *		zero or more whitespace characters
 *
 * Ensures:
 *\li	target will contain the base64 encoded version of the data
 *	in source.  The 'used' pointer in target will be advanced as
 *	necessary.
 */

isc_result_t
isc_base64_decodestring(const char *cstr, isc_buffer_t *target);
/*!<
 * \brief Decode a null-terminated base64 string.
 *
 * Requires:
 *\li	'cstr' is non-null.
 *\li	'target' is a valid buffer.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS	-- the entire decoded representation of 'cstring'
 *			   fit in 'target'.
 *\li	#ISC_R_BADBASE64 -- 'cstr' is not a valid base64 encoding.
 *
 * 	Other error returns are any possible error code from:
 *\li		isc_lex_create(),
 *\li		isc_lex_openbuffer(),
 *\li		isc_base64_tobuffer().
 */

isc_result_t
isc_base64_tobuffer(isc_lex_t *lexer, isc_buffer_t *target, int length);
/*!<
 * \brief Convert base64 encoded text from a lexer context into data.
 *
 * Requires:
 *\li	'lex' is a valid lexer context
 *\li	'target' is a buffer containing binary data
 *\li	'length' is an integer
 *
 * Ensures:
 *\li	target will contain the data represented by the base64 encoded
 *	string parsed by the lexer.  No more than length bytes will be read,
 *	if length is positive.  The 'used' pointer in target will be
 *	advanced as necessary.
 */



ISC_LANG_ENDDECLS

#endif /* ISC_BASE64_H */
