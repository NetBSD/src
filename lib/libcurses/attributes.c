/*	$NetBSD: attributes.c,v 1.1.6.1 2000/01/09 20:43:17 jdc Exp $	*/

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
 * wattron
 *	Test and set attributes.
 *
 *	Modes are blinking, bold (extra bright), dim (half-bright),
 *	blanking (invisible), protected and reverse video
 */

int
wattron(win, attr)
	WINDOW	*win;
	int	 attr;
{
	if ((attr_t) attr & __BLINK) {
		/*
	 	 * If can do blink, set the screen blink bit.
	 	 */
		if (MB != NULL && ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattron: BLINK\n");
#endif
			win->wattr |= __BLINK;
		}
	}
	if ((attr_t) attr & __BOLD) {
		/*
		 * If can do bold, set the screen bold bit.
		 */
		if (MD != NULL && ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattron: BOLD\n");
#endif
			win->wattr |= __BOLD;
		}
	}
	if ((attr_t) attr & __DIM) {
		/*
		 * If can do dim, set the screen dim bit.
		 */
		if (MH != NULL && ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattron: DIM\n");
#endif
			win->wattr |= __DIM;
		}
	}
	if ((attr_t) attr & __BLANK) {
		/*
		 * If can do blink, set the screen blink bit.
		 */
		if (MK != NULL && ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattron: BLANK\n");
#endif
			win->wattr |= __BLANK;
		}
	}
	if ((attr_t) attr & __PROTECT) {
		/*
		 * If can do protected, set the screen protected bit.
		 */
		if (MP != NULL && ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattron: PROTECT\n");
#endif
			win->wattr |= __PROTECT;
		}
	}
	if ((attr_t) attr & __REVERSE) {
		/*
		 * If can do reverse video, set the screen reverse video bit.
		 */
		if (MR != NULL && ME != NULL)
		{
#ifdef DEBUG
			__CTRACE("wattron: REVERSE\n");
#endif
			win->wattr |= __REVERSE;
		}
	}
	if ((attr_t) attr & __STANDOUT) {
		wstandout(win);
	}
	if (attr & __UNDERSCORE) {
		wunderscore(win);
	}
	return (1);
}

/*
 * wattroff
 *	Test and unset attributes.
 *
 *	Note that the 'me' sequence unsets all attributes.  We handle
 *	which attributes should really be set in refresh.c:makech().
 */
int
wattroff(win, attr)
	WINDOW	*win;
	int	 attr;
{
	/*
	 * If can do exit modes, unset the relevent attribute bits.
	 */
	if ((attr_t) attr & __BLINK) {
		if (ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattroff: BLINK\n");
#endif
			win->wattr &= ~__BLINK;
		}
	}
	if ((attr_t) attr & __BOLD) {
		if (ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattroff: BOLD\n");
#endif
			win->wattr &= ~__BOLD;
		}
	}
	if ((attr_t) attr & __DIM) {
		if (ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattroff: DIM\n");
#endif
			win->wattr &= ~__DIM;
		}
	}
	if ((attr_t) attr & __BLANK) {
		if (ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattroff: BLANK\n");
#endif
			win->wattr &= ~__BLANK;
		}
	}
	if ((attr_t) attr & __PROTECT) {
		if (ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattroff: PROTECT\n");
#endif
			win->wattr &= ~__PROTECT;
		}
	}
	if ((attr_t) attr & __REVERSE) {
		if (ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattroff: REVERSE\n");
#endif
			win->wattr &= ~__REVERSE;
		}
	}
	if ((attr_t) attr & __STANDOUT) {
		wstandend(win);
	}
	if (attr & __UNDERSCORE) {
		wunderend(win);
	}
	return (1);
}

/*
 * wattrset
 *	Set specific attribute modes.
 *	Unset others.
 */
int
wattrset(win, attr)
	WINDOW	*win;
	int	 attr;
{
	if ((attr_t) attr & __BLINK) {
		if (MB != NULL && ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattrset: BLINK\n");
#endif
			win->wattr |= __BLINK;
		}
	} else {
		if (ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattrset: !BLINK\n");
#endif
			win->wattr &= ~__BLINK;
		}
	}
	if ((attr_t) attr & __BOLD) {
		if (MD != NULL && ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattrset: BOLD\n");
#endif
			win->wattr |= __BOLD;
		}
	} else {
		if (ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattrset: !BOLD\n");
#endif
			win->wattr &= ~__BOLD;
		}
	}
	if ((attr_t) attr & __DIM) {
		if (MH != NULL && ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattrset: DIM\n");
#endif
			win->wattr |= __DIM;
		}
	} else {
		if (ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattrset: !DIM\n");
#endif
			win->wattr &= ~__DIM;
		}
	}
	if ((attr_t) attr & __BLANK) {
		if (MK != NULL && ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattrset: BLANK\n");
#endif
			win->wattr |= __BLANK;
		}
	} else {
		if (ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattrset: !BLANK\n");
#endif
			win->wattr &= ~__BLANK;
		}
	}
	if ((attr_t) attr & __PROTECT) {
		if (MP != NULL && ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattrset: PROTECT\n");
#endif
			win->wattr |= __PROTECT;
		}
	} else {
		if (ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattrset: !PROTECT\n");
#endif
			win->wattr &= ~__PROTECT;
		}
	}
	if ((attr_t) attr & __REVERSE) {
		if (MR != NULL && ME != NULL)
		{
#ifdef DEBUG
			__CTRACE("wattrset: REVERSE\n");
#endif
			win->wattr |= __REVERSE;
		}
	} else {
		if (ME != NULL) {
#ifdef DEBUG
			__CTRACE("wattrset: !REVERSE\n");
#endif
			win->wattr &= ~__REVERSE;
		}
	}
	if ((attr_t) attr & __STANDOUT) {
		wstandout(win);
	} else {
		wstandend(win);
	}
	if (attr & __UNDERSCORE) {
		wunderscore(win);
	} else {
		wunderend(win);
	}
	return (1);
}
