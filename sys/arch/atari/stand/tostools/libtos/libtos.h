/*	$NetBSD: libtos.h,v 1.1.1.1.44.2 2002/03/16 15:56:58 jdolecek Exp $	*/

/*
 * Copyright (c) 1995 Waldi Ravens.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Waldi Ravens.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef _LIBTOS_H
#define _LIBTOS_H

#ifdef __STDC__
#define	PROTO(x)	x
#define EXTERN
#else
#define	PROTO(x)	()
#define	EXTERN		extern
#endif

#ifdef __GNUC__
#if (__GNUC__ > 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ >= 5))
#define NORETURN	__attribute__((noreturn))
#else
#define	NORETURN
#endif
#endif

#include <sys/types.h>
#include <stdarg.h>

#ifndef int8_t
/*
 * My TOS/MiNT installation does not define these (Leo 09/10/2001).
 */
typedef	unsigned char	u_int8_t;
typedef	unsigned short	u_int16_t;
typedef	unsigned long	u_int32_t;
#endif /* int8_t */

struct kparamb;
struct osdsc;

EXTERN int	aout_load	 PROTO((int, struct osdsc *, char **, int));
EXTERN void	bsd_startup      PROTO((struct kparamb *)) NORETURN;
EXTERN int	elf_load	 PROTO((int, struct osdsc *, char **, int));
EXTERN int	eprintf          PROTO((char *, ...));
EXTERN void	error            PROTO((int, char *, ...));
EXTERN void	fatal            PROTO((int, char *, ...)) NORETURN;
EXTERN void	init_toslib      PROTO((char *));
EXTERN int	key_wait         PROTO((char *));
EXTERN void	press_any_key    PROTO((void));
EXTERN void	redirect_output  PROTO((char *));
EXTERN void	set_wait_for_key PROTO((void));
EXTERN void	sys_info	 PROTO((struct osdsc *));
EXTERN int	veprintf         PROTO((char *, va_list));
EXTERN void	xexit            PROTO((int)) NORETURN;
EXTERN void *	xmalloc          PROTO((size_t));
EXTERN void *	xrealloc         PROTO((void *, size_t));

#endif	/* !_LIBTOS_H */
