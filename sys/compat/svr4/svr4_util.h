/*	$NetBSD: svr4_util.h,v 1.6 1995/04/22 19:49:00 christos Exp $	 */

/*
 * Copyright (c) 1994 Christos Zoulas
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
 */

#ifndef	_SVR4_UTIL_H_
#define	_SVR4_UTIL_H_

#include <machine/vmparam.h>
#include <sys/exec.h>

extern char     svr4_sigcode[], svr4_esigcode[];

static __inline caddr_t
stackgap_init()
{
#define szsigcode (svr4_esigcode - svr4_sigcode)
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

#ifdef DEBUG_SVR4
#define DPRINTF(a)	printf a;
#else
#define DPRINTF(a)
#endif

extern const char svr4_emul_path[];
extern int svr4_error[];

int svr4_emul_find __P((struct proc *, caddr_t *, 
			const char *, char *, char **));

#define CHECKALT(p, sgp, path) \
    svr4_emul_find(p, sgp, svr4_emul_path, path, &(path))

#endif /* !_SVR4_UTIL_H_ */
