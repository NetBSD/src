/* $NetBSD: extern.h,v 1.17 2002/06/10 14:28:14 fredette Exp $ */

/*-
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1992, 1993
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
 *	from: @(#)extern.h	8.1 (Berkeley) 6/6/93
 */

#if defined(__alpha__)
#  define	NLIST_ECOFF
#  define	NLIST_ELF64
#elif defined(__x86_64)
#  define	NLIST_ELF64
#elif defined(__mips__)
#  define	NLIST_AOUT
#  define	NLIST_ECOFF
#  define	NLIST_ELF32
#elif defined(__arm__) || defined(__i386__) || defined(__m68k__) || \
    defined(__powerpc__) || defined(__vax__)
#  define	NLIST_AOUT 
#  define	NLIST_ELF32
#elif defined(__sparc__)
#  define	NLIST_AOUT
#  define	NLIST_ELF32
#  define	NLIST_ELF64
#elif defined(__sh__)
#  define	NLIST_COFF
#  define	NLIST_ELF32
#elif defined(__hppa__)
#  define	NLIST_ELF32
#else 
#  define	NLIST_AOUT
/* #define	NLIST_ECOFF */
/* #define	NLIST_ELF32 */ 
/* #define	NLIST_ELF64 */
#endif

void	create_knlist __P((const char *, DB *));
void	punt __P((void));
int	testdb __P((void));

#ifdef NLIST_AOUT
int	create_knlist_aout __P((const char *, DB *));
#endif
#ifdef NLIST_COFF
int	create_knlist_coff __P((const char *, DB *));
#endif
#ifdef NLIST_ECOFF
int	create_knlist_ecoff __P((const char *, DB *));
#endif
#ifdef NLIST_ELF32
int	create_knlist_elf32 __P((const char *, DB *));
#endif
#ifdef NLIST_ELF64
int	create_knlist_elf64 __P((const char *, DB *));
#endif
