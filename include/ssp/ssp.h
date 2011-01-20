/*	$NetBSD: ssp.h,v 1.5 2011/01/19 19:21:29 christos Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _SSP_SSP_H_
#define _SSP_SSP_H_

#include <sys/cdefs.h>

#if !defined(__cplusplus)
# if _FORTIFY_SOURCE > 0 && __OPTIMIZE__ > 0 && __GNUC_PREREQ__(4, 1)
#  if _FORTIFY_SOURCE > 1
#   define __SSP_FORTIFY_LEVEL 2
#  else
#   define __SSP_FORTIFY_LEVEL 1
#  endif
# endif
#endif

#define __ssp_weak_name(fun) _ ## fun
#define __ssp_inline static __inline __attribute__((__always_inline__))

#define __ssp_bos(ptr) __builtin_object_size(ptr, __SSP_FORTIFY_LEVEL > 1)
#define __ssp_bos0(ptr) __builtin_object_size(ptr, 0)

#define __ssp_redirect_raw(rtype, fun, args, call, bos) \
rtype __ssp_weak_name(fun) args; \
__ssp_inline rtype fun args; \
__ssp_inline rtype fun args { \
	if (bos(__buf) != (size_t)-1 && __len > bos(__buf)) \
		__chk_fail(); \
	return __ssp_weak_name(fun) call; \
}

#define __ssp_redirect(rtype, fun, args, call) \
    __ssp_redirect_raw(rtype, fun, args, call, __ssp_bos)
#define __ssp_redirect0(rtype, fun, args, call) \
    __ssp_redirect_raw(rtype, fun, args, call, __ssp_bos0)

__BEGIN_DECLS
void __stack_chk_fail(void) __dead;
void __chk_fail(void) __dead;
__END_DECLS

#endif /* _SSP_SSP_H_ */
