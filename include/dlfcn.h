/*	$NetBSD: dlfcn.h,v 1.10 1999/05/19 14:50:49 kleink Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _DLFCN_H_
#define _DLFCN_H_

#include <sys/cdefs.h>

#if !defined(_XOPEN_SOURCE)
typedef struct _dl_info {
	const char	*dli_fname;	/* File defining the symbol */
	void		*dli_fbase;	/* Base address */
	const char	*dli_sname;	/* Symbol name */
	void		*dli_saddr;	/* Symbol address */
} Dl_info;
#endif /* !defined(_XOPEN_SOURCE) */

/*
 * User interface to the run-time linker.
 */
__BEGIN_DECLS
extern void	*dlopen __P((const char *, int));
extern int	dlclose __P((void *));
extern void	*dlsym __P((void *, const char *));
#if !defined(_XOPEN_SOURCE)
extern int	dladdr __P((void *, Dl_info *));
extern int	dlctl __P((void *, int, void *));
#endif
extern __aconst char *dlerror __P((void));
__END_DECLS

/* Values for dlopen `mode'. */
#define RTLD_LAZY	1
#define RTLD_NOW	2
#define RTLD_GLOBAL	0x100		/* Allow global searches in object */
#define RTLD_LOCAL	0x200
#if !defined(_XOPEN_SOURCE)
#define DL_LAZY		RTLD_LAZY	/* Compat */
#endif

/*
 * dlctl() commands
 */
#if !defined(_XOPEN_SOURCE)
#define DL_GETERRNO	1
#define DL_GETSYMBOL	2
#if 0
#define DL_SETSRCHPATH	x
#define DL_GETLIST	x
#define DL_GETREFCNT	x
#define DL_GETLOADADDR	x
#endif /* 0 */
#endif /* !defined(_XOPEN_SOURCE) */

#endif /* !defined(_DLFCN_H_) */
