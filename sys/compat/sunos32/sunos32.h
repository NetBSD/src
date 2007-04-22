/*	$NetBSD: sunos32.h,v 1.11 2007/04/22 08:29:59 dsl Exp $	 */

/*
 * Copyright (c) 2001 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _COMPAT_SUNOS32_SUNOS32_H_
#define _COMPAT_SUNOS32_SUNOS32_H_

/*
 * 32-bit SunOS 4.x compatibility module.
 */

#include <compat/sunos/sunos.h>

/*
 * Typedefs for pointer-types.
 */
/* stime() */
typedef u_int32_t sunos32_time_tp;

/* statfs(), fstatfs() */
typedef u_int32_t sunos32_statfsp_t;

/* ustat() */
typedef netbsd32_pointer_t sunos32_ustatp_t;

/* uname() */
typedef netbsd32_pointer_t sunos32_utsnamep_t;

/*
 * general prototypes
 */
__BEGIN_DECLS
/* Defined in arch/<arch>/sunos_machdep.c */
void	sunos32_sendsig(const ksiginfo_t *, const sigset_t *);
__END_DECLS

/*
 * here are some macros to convert between sunos32 and sparc64 types.
 * note that they do *NOT* act like good macros and put ()'s around all
 * arguments cuz this _breaks_ SCARG().
 */
#define SUNOS32TO64(s32uap, uap, name) \
	    SCARG(uap, name) = SCARG(s32uap, name)
#define SUNOS32TOP(s32uap, uap, name, type) \
	    SCARG(uap, name) = SCARG_P32(s32uap, name)
#define SUNOS32TOX(s32uap, uap, name, type) \
	    SCARG(uap, name) = (type)SCARG(s32uap, name)
#define SUNOS32TOX64(s32uap, uap, name, type) \
	    SCARG(uap, name) = (type)(u_long)SCARG(s32uap, name)

/* and some standard versions */
#define	SUNOS32TO64_UAP(name)		SUNOS32TO64(uap, &ua, name);
#define	SUNOS32TOP_UAP(name, type)	SUNOS32TOP(uap, &ua, name, type);
#define	SUNOS32TOX_UAP(name, type)	SUNOS32TOX(uap, &ua, name, type);
#define	SUNOS32TOX64_UAP(name, type)	SUNOS32TOX64(uap, &ua, name, type);

#endif /* _COMPAT_SUNOS32_SUNOS32_H_ */
