/*	$NetBSD: citrus_iconv.h,v 1.1 2003/06/25 09:51:34 tshiozak Exp $	*/

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

struct _citrus_iconv;
struct _citrus_iconv_ops;

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
_citrus_iconv_convert(struct _citrus_iconv * __restrict ci,
		      const char * __restrict * __restrict in,
		      size_t * __restrict inbytes,
		      char * __restrict * __restrict out,
		      size_t * __restrict outbytes, u_int32_t flags,
		      size_t * __restrict nresults)
{
	int ret;

	_DIAGASSERT(ci & ci->ci_ops && dst);

	ret = (*ci->ci_ops->io_convert)(ci, in, inbytes, out, outbytes, flags,
					nresults);

	return ret;
}

#endif
