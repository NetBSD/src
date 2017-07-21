/*	$NetBSD: citrus_iconv.h,v 1.5.64.2 2017/07/21 20:22:29 perseant Exp $	*/

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

#ifndef _CITRUS_ICONV_H_
#define _CITRUS_ICONV_H_

struct _citrus_iconv_shared;
struct _citrus_iconv_ops;
struct _citrus_iconv;

__BEGIN_DECLS
int	_citrus_iconv_open(struct _citrus_iconv * __restrict * __restrict,
			   const char * __restrict,
			   const char * __restrict, const char * __restrict);
void	_citrus_iconv_close(struct _citrus_iconv *);
__END_DECLS


#include "citrus_iconv_local.h"

#define _CITRUS_ICONV_F_HIDE_INVALID	0x0001

/*
 * _citrus_iconv_convert:
 *	convert a string.
 */
static __inline int
_citrus_iconv_convert(struct _citrus_iconv * __restrict cv,
		      const char * __restrict * __restrict in,
		      size_t * __restrict inbytes,
		      char * __restrict * __restrict out,
		      size_t * __restrict outbytes, uint32_t flags,
		      size_t * __restrict nresults)
{

	_DIAGASSERT(cv && cv->cv_shared && cv->cv_shared->ci_ops &&
	    cv->cv_shared->ci_ops->io_convert);
	_DIAGASSERT(out || outbytes == 0);

	return (*cv->cv_shared->ci_ops->io_convert)(cv, in, inbytes, out,
						    outbytes, flags, nresults);
}

/*
 * _citrus_iconv_wchar_convert:
 *     convert a wchar.
 */
static __inline int
_citrus_iconv_wchar_convert(struct _citrus_iconv * __restrict cv,
                           wchar_t in, wchar_t *out)
{
	_DIAGASSERT(cv && cv->cv_shared && cv->cv_shared->ci_ops &&
		cv->cv_shared->ci_ops->io_wchar_convert);
	_DIAGASSERT(out);

	return (*cv->cv_shared->ci_ops->io_wchar_convert)(cv, in, out);
}


#endif
