/*	$NetBSD: citrus_bcs.h,v 1.2 2004/01/02 21:49:35 itojun Exp $	*/

/*-
 * Copyright (c)2003 Citrus Project,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _CITRUS_BCS_H_
#define _CITRUS_BCS_H_

/*
 * predicate/conversion for basic character set.
 */

#define _CITRUS_BCS_PRED(_name_, _cond_) \
static __inline int _citrus_bcs_##_name_(u_int8_t c) { return (_cond_); }

_CITRUS_BCS_PRED(isblank, c == ' ' || c == '\t')
_CITRUS_BCS_PRED(iseol, c == '\n' || c == '\r')
_CITRUS_BCS_PRED(isspace,
		 _citrus_bcs_isblank(c) || _citrus_bcs_iseol(c) ||
		 c == '\f' || c == '\v')
_CITRUS_BCS_PRED(isdigit, c >= '0' && c <= '9')
_CITRUS_BCS_PRED(isupper, c >= 'A' && c <= 'Z')
_CITRUS_BCS_PRED(islower, c >= 'a' && c <= 'z')
_CITRUS_BCS_PRED(isalpha, _citrus_bcs_isupper(c) || _citrus_bcs_islower(c))
_CITRUS_BCS_PRED(isalnum, _citrus_bcs_isdigit(c) || _citrus_bcs_isalpha(c))
_CITRUS_BCS_PRED(isxdigit,
		 _citrus_bcs_isdigit(c) ||
		 (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))

static __inline u_int8_t
_citrus_bcs_toupper(u_int8_t c)
{
	return (_citrus_bcs_islower(c) ? (c - 'a' + 'A') : c);
}

static __inline u_int8_t
_citrus_bcs_tolower(u_int8_t c)
{
	return (_citrus_bcs_isupper(c) ? (c - 'A' + 'a') : c);
}

__BEGIN_DECLS
int _citrus_bcs_strcasecmp(const char * __restrict, const char * __restrict);
int _citrus_bcs_strncasecmp(const char * __restrict, const char * __restrict,
			    size_t);
const char *_citrus_bcs_skip_ws(const char * __restrict);
const char *_citrus_bcs_skip_nonws(const char * __restrict);
const char *_citrus_bcs_skip_ws_len(const char * __restrict,
				       size_t * __restrict);
const char *_citrus_bcs_skip_nonws_len(const char * __restrict,
				       size_t * __restrict);
void _citrus_bcs_trunc_rws_len(const char * __restrict, size_t * __restrict);
void _citrus_bcs_convert_to_lower(char *);
void _citrus_bcs_convert_to_upper(char *);
__END_DECLS

#endif
