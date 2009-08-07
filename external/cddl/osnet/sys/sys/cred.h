/*	$NetBSD: cred.h,v 1.1 2009/08/07 20:57:57 haad Exp $	*/

/*-
 * Copyright (c) 2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/compat/opensolaris/sys/cred.h,v 1.1 2007/04/06 01:09:06 pjd Exp $
 */

#ifndef _OPENSOLARIS_SYS_CRED_H_
#define	_OPENSOLARIS_SYS_CRED_H_

#include <sys/param.h>
#include <sys/types.h>

#ifdef _KERNEL
#include <sys/kauth.h>

#define	CRED()		(kauth_cred_get())
#define KCRED()		(cred0)
#define	kcred		cred0

extern kauth_cred_t	cred0;

#define	crget(cr)		cr = kauth_cred_get()
#define	crgetuid(cr)		kauth_cred_getuid(cr)
#define	crgetgid(cr)		kauth_cred_getgid(cr)
#define	crgetngroups(cr) 	kauth_cred_ngroups(cr)
#define	cralloc()		kauth_cred_alloc()
#define	crfree(cr)		kauth_cred_free(cr)
#define	crsetugid(cr, u, g)	( \
	kauth_cred_setuid(cr, u), \
	kauth_cred_setgid(cr, g), \
	kauth_cred_seteuid(cr, u), \
	kauth_cred_setegid(cr, g), \
	kauth_cred_setsvuid(cr, u), \
	kauth_cred_setsvgid(cr, g), 0)
#define	crsetgroups(cr, gc, ga)	\
    kauth_cred_setgroups(cr, ga, gc, 0, UIO_SYSSPACE)
#define crgetsid(cr, i) (NULL)

static __inline gid_t *
crgetgroups(cred_t *cr)
{
	static gid_t gids[NGROUPS_MAX];

	memset(gids, 0, NGROUPS_MAX);
	if (kauth_cred_getgroups(cr, gids, NGROUPS_MAX, UIO_SYSSPACE) != 0) 
		return NULL;
	
	return gids;
}

static int
groupmember(gid_t gid, cred_t *cr) 
{
  int result;

  kauth_cred_ismember_gid(cr, gid, &result);

  return result;
}

#endif	/* _KERNEL */

#endif	/* _OPENSOLARIS_SYS_CRED_H_ */
