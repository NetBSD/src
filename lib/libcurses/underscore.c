/*	$NetBSD: underscore.c,v 1.1.6.1 2000/01/09 20:43:23 jdc Exp $	*/

/*
 * Copyright (c) 1999 Julian. D. Coleman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "curses.h"

/*
 * wunderscore
 *	Enter underscore mode.
 */
int
wunderscore(win)
	WINDOW *win;
{
	/*
	 * If can underscore, set the screen underscore bit.
	 */
	if ((US != NULL && UE != NULL) || UC != NULL) {
#ifdef DEBUG
		__CTRACE("wunderscore\n");
#endif
		win->wattr |= __UNDERSCORE;
	}
	return (1);
}

/*
 * wunderend --
 *	Exit underscore mode.
 */
int
wunderend(win)
	WINDOW *win;
{
	if (UE != NULL) {
#ifdef DEBUG
		__CTRACE("wunderuend\n");
#endif
		win->wattr &= ~__UNDERSCORE;
	}
        return 1;
}
