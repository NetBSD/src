/*	$NetBSD: ctype.h,v 1.21 2001/04/18 01:45:18 thorpej Exp $	*/

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
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
 *	@(#)ctype.h	5.3 (Berkeley) 4/3/91
 */

#ifndef _CTYPE_H_
#define _CTYPE_H_

#include <sys/featuretest.h>

#include <sys/cdefs.h>

#define	_U	0x01
#define	_L	0x02
#define	_N	0x04
#define	_S	0x08
#define	_P	0x10
#define	_C	0x20
#define	_X	0x40
#define	_B	0x80

extern const unsigned char	*_ctype_;
extern const short	*_tolower_tab_;
extern const short	*_toupper_tab_;


__BEGIN_DECLS
int	isalnum __P ((int));
int	isalpha __P ((int));
int	iscntrl __P ((int));
int	isdigit __P ((int));
int	isgraph __P ((int));
int	islower __P ((int));
int	isprint __P ((int));
int	ispunct __P ((int));
int	isspace __P ((int));
int	isupper __P ((int));
int	isxdigit __P ((int));
int	tolower __P ((int));
int	toupper __P ((int));

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) || \
    defined(_XOPEN_SOURCE)
int	isascii __P ((int));
int	toascii __P ((int));
int	_tolower __P ((int));
int	_toupper __P ((int));
#endif

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE)
int	isblank __P ((int));
#endif
__END_DECLS

#define	isdigit(c)	((int)((_ctype_ + 1)[(int)(c)] & _N))
#define	islower(c)	((int)((_ctype_ + 1)[(int)(c)] & _L))
#define	isspace(c)	((int)((_ctype_ + 1)[(int)(c)] & _S))
#define	ispunct(c)	((int)((_ctype_ + 1)[(int)(c)] & _P))
#define	isupper(c)	((int)((_ctype_ + 1)[(int)(c)] & _U))
#define	isalpha(c)	((int)((_ctype_ + 1)[(int)(c)] & (_U|_L)))
#define	isxdigit(c)	((int)((_ctype_ + 1)[(int)(c)] & (_N|_X)))
#define	isalnum(c)	((int)((_ctype_ + 1)[(int)(c)] & (_U|_L|_N)))
#define	isprint(c)	((int)((_ctype_ + 1)[(int)(c)] & (_P|_U|_L|_N|_B)))
#define	isgraph(c)	((int)((_ctype_ + 1)[(int)(c)] & (_P|_U|_L|_N)))
#define	iscntrl(c)	((int)((_ctype_ + 1)[(int)(c)] & _C))
#define	tolower(c)	((int)((_tolower_tab_ + 1)[(int)(c)]))
#define	toupper(c)	((int)((_toupper_tab_ + 1)[(int)(c)]))

#if !defined(_ANSI_SOURCE) && !defined (_POSIX_C_SOURCE) || \
    defined(_XOPEN_SOURCE)
#define	isascii(c)	((unsigned)(c) <= 0177)
#define	toascii(c)	((c) & 0177)
#define _tolower(c)	((c) - 'A' + 'a')
#define _toupper(c)	((c) - 'a' + 'A')
#endif

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE)
#if notyet
/*
 * isblank() is implemented as C function, due to insufficient bitwidth in
 * _ctype_.  Note that _B does not mean isblank - it means isprint && !isgraph.
 */
#define isblank(c)	((int)((_ctype_ + 1)[(int)(c)] & _B))
#endif
#endif

#ifdef _CTYPE_PRIVATE
#include <machine/limits.h>	/* for CHAR_BIT */

#define _CTYPE_NUM_CHARS	(1 << CHAR_BIT)

#define _CTYPE_ID	 	"BSDCTYPE"
#define _CTYPE_REV		2

extern const u_int8_t _C_ctype_[];
extern const int16_t _C_toupper_[];
extern const int16_t _C_tolower_[];
#endif

#endif /* !_CTYPE_H_ */
