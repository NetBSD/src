/*	$NetBSD: fenv.h,v 1.19 2015/12/27 19:50:31 christos Exp $	*/
/*
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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
#ifndef _FENV_H_
#define _FENV_H_

#include <sys/featuretest.h>

#if !defined(__aarch64__) && !defined(__arm__) && !defined(__i386__) \
    && !defined(__hppa__) && !defined(__powerpc__) && !defined(__mips__) \
    && !defined(__or1k__) && !defined(__riscv__) && !defined(__sparc__) \
    && !defined(__x86_64__)
    && !(defined(__m68k__) && (defined(__mc68010__) || defined(__mcoldfire__)))
# ifndef __TEST_FENV
#  error	"fenv.h is currently not supported for this architecture"
# endif
typedef int fexcept_t;
typedef int fenv_t;
#else
# define __HAVE_FENV
# include <machine/fenv.h>
#endif

__BEGIN_DECLS

/* Function prototypes */
int	feclearexcept(int);
int	fegetexceptflag(fexcept_t *, int);
int	feraiseexcept(int);
int	fesetexceptflag(const fexcept_t *, int);
int	fetestexcept(int);
int	fegetround(void);
int	fesetround(int);
int	fegetenv(fenv_t *);
int	feholdexcept(fenv_t *);
int	fesetenv(const fenv_t *);
int	feupdateenv(const fenv_t *);

#if defined(_NETBSD_SOURCE) || defined(_GNU_SOURCE)

int	feenableexcept(int);
int	fedisableexcept(int);
int	fegetexcept(void);

#endif /* _NETBSD_SOURCE || _GNU_SOURCE */

__END_DECLS

#endif /* ! _FENV_H_ */
