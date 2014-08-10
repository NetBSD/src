/*	$NetBSD: loadfile_machdep.h,v 1.7.60.1 2014/08/10 06:53:58 tls Exp $	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and Jason R. Thorpe.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _HP300_LOADFILE_MACHDEP_H_
#define	_HP300_LOADFILE_MACHDEP_H_

#define	BOOT_AOUT
#define	BOOT_ELF32

#define	LOAD_KERNEL	LOAD_ALL
#define	COUNT_KERNEL	COUNT_ALL

#ifdef _STANDALONE

#define	LOADADDR(a)		((a) + offset)
#define	ALIGNENTRY(a)		0
#define	READ(f, b, c)		pread((f), (void *)LOADADDR(b), (c))
#define	BCOPY(s, d, c)		vpbcopy((s), (void *)LOADADDR(d), (c))
#define	BZERO(d, c)		pbzero((void *)LOADADDR(d), (c))
#define	WARN(a)			do { \
					(void)printf a; \
					if (errno) \
						(void)printf(": %s\n", \
						             strerror(errno)); \
					else \
						(void)printf("\n"); \
				} while(/* CONSTCOND */0)
#define	PROGRESS(a)		(void) printf a
#define	ALLOC(a)		alloc(a)
#define	DEALLOC(a, b)		dealloc(a, b)
#define	OKMAGIC(a)		((a) == OMAGIC || (a) == NMAGIC ||	\
				 (a) == ZMAGIC)

#define	vpbcopy bcopy
#define	pbzero  bzero
#define	pread   read

#else

#define	LOADADDR(a)		(((u_long)(a)) + offset)
#define	ALIGNENTRY(a)		((u_long)(a))
#define	READ(f, b, c)		read((f), (void *)LOADADDR(b), (c))
#define	BCOPY(s, d, c)		memcpy((void *)LOADADDR(d), (void *)(s), (c))
#define	BZERO(d, c)		memset((void *)LOADADDR(d), 0, (c))
#define	WARN(a)			warn a
#define	PROGRESS(a)		/* nothing */
#define	ALLOC(a)		malloc(a)
#define	DEALLOC(a, b)		free(a)
#define	OKMAGIC(a)		((a) == OMAGIC)

ssize_t	vread(int, u_long, u_long *, size_t);
void	vcopy(u_long, u_long, u_long *, size_t);
void	vzero(u_long, u_long *, size_t);

#endif
#endif /* ! _HP300_LOADFILE_MACHDEP_H_ */
