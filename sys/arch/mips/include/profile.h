/*	$NetBSD: profile.h,v 1.15 2000/07/18 06:25:32 jeffs Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	@(#)profile.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _MIPS_PROFILE_H_
#define _MIPS_PROFILE_H_

#ifdef _KERNEL
 /*
  *  Declare non-profiled _splhigh() /_splx() entrypoints for _mcount.
  *  see MCOUNT_ENTER and MCOUNT_EXIT.
  */
#define	_KERNEL_MCOUNT_DECL			\
	int _splraise_noprof __P((int));	\
	int _splset_noprof __P((int));
#else   /* !_KERNEL */
/* Make __mcount static. */
#define	_KERNEL_MCOUNT_DECL	static
#endif	/* !_KERNEL */

#ifdef _KERNEL
# define _PROF_CPLOAD	""
#else
# define _PROF_CPLOAD	".cpload $25;"
#endif


#define	_MCOUNT_DECL \
    _KERNEL_MCOUNT_DECL \
    void __attribute__((unused)) __mcount

#define	MCOUNT \
	__asm__(".globl _mcount;" \
	".type _mcount,@function;" \
	"_mcount:;" \
	".set noreorder;" \
	".set noat;" \
	_PROF_CPLOAD \
	"addu $29,$29,-16;" \
	"sw $4,8($29);" \
	"sw $5,12($29);" \
	"sw $6,16($29);" \
	"sw $7,20($29);" \
	"sw $1,0($29);" \
	"sw $31,4($29);" \
	"move $5,$31;" \
	"jal __mcount;" \
	"move $4,$1;" \
	"lw $4,8($29);" \
	"lw $5,12($29);" \
	"lw $6,16($29);" \
	"lw $7,20($29);" \
	"lw $31,4($29);" \
	"lw $1,0($29);" \
	"addu $29,$29,24;" \
	"j $31;" \
	"move $31,$1;" \
	".set reorder;" \
	".set at");

#ifdef _KERNEL
/*
 * The following two macros do splhigh and splx respectively.
 * We use versions of _splraise() and _splset that don't
 * including profiling support.
 */

#define	MCOUNT_ENTER	s = _splraise_noprof(MIPS_INT_MASK)

#define	MCOUNT_EXIT	(void)_splset_noprof(s)
#endif /* _KERNEL */

#endif /* _MIPS_PROFILE_H_ */
