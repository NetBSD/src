/* 	$NetBSD: wsfont.h,v 1.6.10.1 1999/12/27 18:35:49 wrstuden Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andy Doran.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _WSFONT_H_
#define _WSFONT_H_ 1

/*
 * wsfont_find() can be called with any of the parameters as 0, meaning we
 * don't care about that aspect of the font. It returns a cookie which
 * we can use with the other functions. When more flexibility is required,
 * wsfont_enum() should be used. The last two parameters to wsfont_lock()
 * are the bit order and byte order required (WSFONT_L2R or WSFONT_R2L).
 *
 * Example:
 *
 *	struct wsdisplay_font *font;
 *	int cookie;
 *
 *	if ((cookie = wsfont_find(NULL, 8, 16, 0, 0)) <= 0)
 *		panic("unable to get 8x16 font");
 *
 *	if (wsfont_lock(cookie, &font, WSFONT_L2R, WSFONT_R2L) <= 0)
 *		panic("unable to lock font");
 *
 *	... do stuff ...
 *
 *	wsfont_unlock(cookie);
 */

struct wsdisplay_font;

/* Left to Right, Right to Left */
#define WSFONT_L2R	(1)
#define WSFONT_R2L	(2)

/* For wsfont_add() */
#define WSFONT_BUILTIN	(0x01)
#define WSFONT_STATIC	(0x02)
#define WSFONT_RDONLY	(0x04)

/* wsfont.c */
void	wsfont_init __P((void));
int	wsfont_find __P((char *, int, int, int));
int	wsfont_add __P((struct wsdisplay_font *, int));
int	wsfont_remove __P((int));
void	wsfont_enum __P((void (*) __P((char *, int, int, int))));
int	wsfont_lock __P((int, struct wsdisplay_font **, int, int));
int	wsfont_unlock __P((int));
int	wsfont_getflg __P((int, int *, int *));

#endif	/* !_WSFONT_H_ */
