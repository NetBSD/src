/*	$NetBSD: linux_util.h,v 1.2 1995/03/05 23:23:50 fvdl Exp $	*/

/*
 * Copyright (c) 1994 Christos Zoulas
 * Copyright (c) 1995 Frank van der Linden
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
 *    derived from this software without specific prior written permission
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
 *
 * from: svr4_util.h,v 1.5 1994/11/18 02:54:31 christos Exp
 */

/*
 * This file is pretty much the same as Christos' svr4_util.h
 * (for now).
 */

#ifndef	_LINUX_UTIL_H_
#define	_LINUX_UTIL_H_

#include <machine/vmparam.h>
#include <sys/exec.h>
#include <sys/cdefs.h>

#define cvtto_linux_mask(flags,bmask,lmask) (((flags) & bmask) ? lmask : 0)
#define cvtto_bsd_mask(flags,lmask,bmask) (((flags) & lmask) ? bmask : 0)

static __inline caddr_t
stackgap_init()
{
	extern char     sigcode[], esigcode[];
#define szsigcode ((caddr_t)(esigcode - sigcode))
	return STACKGAPBASE;
}


static __inline void *
stackgap_alloc(sgp, sz)
	caddr_t	*sgp;
	size_t   sz;
{
	void	*p = (void *) *sgp;
	*sgp += ALIGN(sz);
	return p;
}

extern const char linux_emul_path[];

int linux_emul_find __P((struct proc *, caddr_t *, const char *, char *,
			char **, int));

#define CHECK_ALT_EXIST(p, sgp, path) \
    linux_emul_find(p, sgp, linux_emul_path, path, &(path), 0)

#define CHECK_ALT_CREAT(p, sgp, path) \
    linux_emul_find(p, sgp, linux_emul_path, path, &(path), 1)

#endif /* !_LINUX_UTIL_H_ */
