/* $NetBSD: svr4_util.h,v 1.1 1994/10/24 17:37:59 deraadt Exp $	*/
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

static __inline void
stackgap_init()
{
    extern char sigcode[], esigcode[];
#define szsigcode (esigcode - sigcode)
    extern caddr_t svr4_edata;
    svr4_edata = (caddr_t) STACKGAPBASE;
}


static __inline void *
stackgap_alloc(sz)
    size_t sz;
{
    extern caddr_t svr4_edata;
    void *p = (void *) svr4_edata;
    svr4_edata += ALIGN(sz);
    return p;
}

#ifdef DEBUG_SVR4
# define DPRINTF(a)	printf a;
#else
# define DPRINTF(a)
#endif

extern const char svr4_emul_path[];

int svr4_emul_find __P((struct proc *, int, const char *,
			char *, char **));

#define CHECKALT(p, path) \
    svr4_emul_find(p, UIO_USERSPACE, svr4_emul_path, (path), &(path))

#endif /* !_SVR4_UTIL_H_ */
