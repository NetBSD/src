/*	$NetBSD: setjmp.h,v 1.20 2003/08/07 09:44:11 agc Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *
 *	@(#)setjmp.h	8.2 (Berkeley) 1/21/94
 */

#ifndef _SETJMP_H_
#define _SETJMP_H_

#include <sys/featuretest.h>

#include <machine/setjmp.h>

#ifndef _JB_ATTRIBUTES
#define _JB_ATTRIBUTES	/**/
#else
#endif

#if defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE) || \
    defined(_NETBSD_SOURCE)
typedef long sigjmp_buf[_JBLEN + 1] _JB_ATTRIBUTES;
#endif /* not ANSI */

typedef long jmp_buf[_JBLEN] _JB_ATTRIBUTES;

#include <sys/cdefs.h>

__BEGIN_DECLS
#ifdef __LIBC12_SOURCE__
int	__setjmp14 __P((jmp_buf));
void	__longjmp14 __P((jmp_buf, int));

int	__sigsetjmp14 __P((sigjmp_buf, int));
void	__siglongjmp14 __P((sigjmp_buf, int));
#else /* !__LIBC12_SOURCE__ */
int	setjmp __P((jmp_buf)) __RENAME(__setjmp14);
void	longjmp __P((jmp_buf, int)) __RENAME(__longjmp14);

#if defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE) || \
    defined(_NETBSD_SOURCE)
int	sigsetjmp __P((sigjmp_buf, int)) __RENAME(__sigsetjmp14);
void	siglongjmp __P((sigjmp_buf, int)) __RENAME(__siglongjmp14);
#endif /* not ANSI */
#endif /* __LIBC12_SOURCE__ */

#if defined(_XOPEN_SOURCE) || defined(_NETBSD_SOURCE)
int	_setjmp __P((jmp_buf));
void	_longjmp __P((jmp_buf, int));
#endif

#if defined(_NETBSD_SOURCE)
void	longjmperror __P((void));
#endif
__END_DECLS

#endif /* !_SETJMP_H_ */
