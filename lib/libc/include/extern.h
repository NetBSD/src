/*	$NetBSD: extern.h,v 1.26.6.2 2024/10/08 11:16:16 martin Exp $	*/

/*
 * Copyright (c) 1997 Christos Zoulas.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdarg.h>
#include <ucontext.h>

#ifndef __LOCALE_T_DECLARED
typedef struct _locale		*locale_t;
#define __LOCALE_T_DECLARED
#endif /* __LOCALE_T_DECLARED */

__BEGIN_DECLS
extern char *__minbrk;
int __getcwd(char *, size_t);
int __getlogin(char *, size_t);
int __setlogin(const char *);
void _resumecontext(void) __dead;
__dso_hidden int	_strerror_lr(int, char *, size_t, locale_t);
const char *__strerror(int , char *, size_t);
const char *__strsignal(int , char *, size_t);
char *__dtoa(double, int, int, int *, int *, char **);
void __freedtoa(char *);
int __sysctl(const int *, unsigned int, void *, size_t *, const void *, size_t);

struct sigaction;
int __sigaction_sigtramp(int, const struct sigaction *,
    struct sigaction *, const void *, int);

/* is "long double" and "double" different? */
#if (__LDBL_MANT_DIG__ != __DBL_MANT_DIG__) || \
    (__LDBL_MAX_EXP__ != __DBL_MAX_EXP__)
#define WIDE_DOUBLE
#endif

#ifdef WIDE_DOUBLE
char *__hldtoa(long double, const char *, int, int *, int *,  char **);
char *__ldtoa(long double *, int, int, int *, int *, char **);
#endif
char *__hdtoa(double, const char *, int, int *, int *, char **);

void	_malloc_prefork(void);
void	_malloc_postfork(void);
void	_malloc_postfork_child(void);

int	_sys_setcontext(const ucontext_t *);

int	strerror_r_ss(int, char *, size_t);
__aconst char *strerror_ss(int);

__END_DECLS
