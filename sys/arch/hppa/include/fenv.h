/*	$NetBSD: fenv.h,v 1.2.20.2 2017/12/03 11:36:16 jdolecek Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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
 *
 */

#ifndef	_HPPA_FENV_H_
#define	_HPPA_FENV_H_

#include <sys/stdint.h>

typedef unsigned fenv_t;
typedef unsigned fexcept_t;

#define	FE_INEXACT	0x01	/* imprecise (loss of precision) */
#define	FE_UNDERFLOW	0x02	/* underflow exception */
#define	FE_OVERFLOW	0x04	/* overflow exception */
#define	FE_DIVBYZERO	0x08	/* divide-by-zero exception */
#define	FE_INVALID	0x10	/* invalid operation exception */

#define	FE_ALL_EXCEPT	0x1f

#define	FE_TONEAREST	(0)	/* round to nearest representable number */
#define	FE_TOWARDZERO	(1<<9)	/* round to zero (truncate) */
#define	FE_UPWARD	(2<<9)	/* round toward positive infinity */
#define	FE_DOWNWARD	(3<<9)	/* round toward negative infinity */


__BEGIN_DECLS

/* Default floating-point environment */
extern const fenv_t	__fe_dfl_env;
#define	FE_DFL_ENV	(&__fe_dfl_env)

__END_DECLS

#endif	/* !_HPPA_FENV_H_ */
