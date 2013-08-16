/*	$NetBSD: sethostent.c,v 1.18 2013/08/16 15:27:12 christos Exp $	*/

/*
 * Copyright (c) 1985, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)sethostent.c	8.1 (Berkeley) 6/4/93";
static char rcsid[] = "Id: sethostent.c,v 8.5 1996/09/28 06:51:07 vixie Exp ";
#else
__RCSID("$NetBSD: sethostent.c,v 1.18 2013/08/16 15:27:12 christos Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <netdb.h>
#include <resolv.h>

#include "hostent.h"

#ifdef __weak_alias
__weak_alias(sethostent,_sethostent)
__weak_alias(endhostent,_endhostent)
#endif

#ifndef _REENTRANT
void	res_close(void);
#endif

void
/*ARGSUSED*/
sethostent(int stayopen)
{
#ifndef _REENTRANT
	if ((_res.options & RES_INIT) == 0 && res_init() == -1)
		return;
	if (stayopen)
		_res.options |= RES_STAYOPEN | RES_USEVC;
#endif
	sethostent_r(&_h_file);
}

void
endhostent(void)
{
#ifndef _REENTRANT
	_res.options &= ~(RES_STAYOPEN | RES_USEVC);
	res_close();
#endif
	endhostent_r(&_h_file);
}
