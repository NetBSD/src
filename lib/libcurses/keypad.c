/*	$NetBSD: keypad.c,v 1.5 2000/05/17 16:23:49 jdc Exp $  */

/*-
 * Copyright (c) 1998-1999 Brett Lymn (blymn@baea.com.au, brett_lymn@yahoo.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
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
 *
 *
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: keypad.c,v 1.5 2000/05/17 16:23:49 jdc Exp $");
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

/*
 * keypad --
 *	Turn on and off interpretation of function/keypad keys in the
 *	given window.
 */
void
keypad(WINDOW *win, bool bf)
{
#ifdef DEBUG
	__CTRACE("keypad: win %0.2o, %s\n", win, bf ? "TRUE" : "FALSE");
#endif
	if (bf) {
		win->flags |= __KEYPAD;
		/* Be compatible with SysV curses. */
		if (!(curscr->flags & __KEYPAD)) {
			tputs (KS, 0, __cputchar);
			curscr->flags |= __KEYPAD;
		}
	} else {
		win->flags &= ~__KEYPAD;
		/* Be compatible with SysV curses. */
		if (curscr->flags & __KEYPAD) {
			tputs (KE, 0, __cputchar);
			curscr->flags &= ~__KEYPAD;
		}
	}
}
