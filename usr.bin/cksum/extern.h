/*	$NetBSD: extern.h,v 1.9 2001/03/21 03:16:38 atatat Exp $	*/

/*-
 * Copyright (c) 1991, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)extern.h	8.1 (Berkeley) 6/6/93
 */

#include <sys/cdefs.h>

__BEGIN_DECLS
int	crc __P((int, u_int32_t *, u_int32_t *));
void	pcrc __P((char *, u_int32_t, u_int32_t));
void	psum1 __P((char *, u_int32_t, u_int32_t));
void	psum2 __P((char *, u_int32_t, u_int32_t));
int	csum1 __P((int, u_int32_t *, u_int32_t *));
int	csum2 __P((int, u_int32_t *, u_int32_t *));
int	md5 __P((int, u_int32_t *, u_int32_t *));

void	MD2String __P((const char *));
void	MD2TimeTrial __P((void));
void	MD2TestSuite __P((void));
void	MD2Filter __P((int));

void	MD4String __P((const char *));
void	MD4TimeTrial __P((void));
void	MD4TestSuite __P((void));
void	MD4Filter __P((int));

void	MD5String __P((const char *));
void	MD5TimeTrial __P((void));
void	MD5TestSuite __P((void));
void	MD5Filter __P((int));

void	SHA1String __P((const char *));
void	SHA1TimeTrial __P((void));
void	SHA1TestSuite __P((void));
void	SHA1Filter __P((int));

void	RMD160String __P((const char *));
void	RMD160TimeTrial __P((void));
void	RMD160TestSuite __P((void));
void	RMD160Filter __P((int));
__END_DECLS
