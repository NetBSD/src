/*	$NetBSD: fenv.c,v 1.2.2.1 2017/04/26 02:52:55 pgoyette Exp $	*/

/*-
 * Copyright (c) 2004 David Schultz <das@FreeBSD.ORG>
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
 * $FreeBSD: head/lib/msun/mips/fenv.c 226415 2011-10-16 05:37:56Z das $
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: fenv.c,v 1.2.2.1 2017/04/26 02:52:55 pgoyette Exp $");

#include "namespace.h"

#define	__fenv_static
#include <fenv.h>

#ifdef __weak_alias
__weak_alias(feclearexcept,_feclearexcept)
__weak_alias(fedisableexcept,_fedisableexcept)
__weak_alias(feenableexcept,_feenableexcept)
__weak_alias(fegetenv,_fegetenv)
__weak_alias(fegetexcept,_fegetexcept)
__weak_alias(fegetexceptflag,_fegetexceptflag)
__weak_alias(fegetround,_fegetround)
__weak_alias(feholdexcept,_feholdexcept)
__weak_alias(feraiseexcept,_feraiseexcept)
__weak_alias(fesetenv,_fesetenv)
__weak_alias(fesetexceptflag,_fesetexceptflag)
__weak_alias(fesetround,_fesetround)
__weak_alias(fetestexcept,_fetestexcept)
__weak_alias(feupdateenv,_feupdateenv)
#endif

#if defined(__GNUC_GNU_INLINE__) && !defined(__lint__)
#error "This file must be compiled with C99 'inline' semantics"
#endif

/*
 * Hopefully the system ID byte is immutable, so it's valid to use
 * this as a default environment.
 */
const fenv_t __fe_dfl_env = 0;

extern inline int feclearexcept(int __excepts);
extern inline int fegetexceptflag(fexcept_t *__flagp, int __excepts);
extern inline int fesetexceptflag(const fexcept_t *__flagp, int __excepts);
extern inline int feraiseexcept(int __excepts);
extern inline int fetestexcept(int __excepts);
extern inline int fegetround(void);
extern inline int fesetround(int __round);
extern inline int fegetenv(fenv_t *__envp);
extern inline int feholdexcept(fenv_t *__envp);
extern inline int fesetenv(const fenv_t *__envp);
extern inline int feupdateenv(const fenv_t *__envp);
extern inline int feenableexcept(int __excepts);
extern inline int fedisableexcept(int __excepts);
extern inline int fegetexcept(void);
