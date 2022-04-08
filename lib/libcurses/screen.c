/*	$NetBSD: screen.c,v 1.37 2022/04/08 10:17:52 andvar Exp $	*/

/*
 * Copyright (c) 1981, 1993, 1994
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
 * 3. Neither the name of the University nor the names of its contributors
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
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)screen.c	8.2 (blymn) 11/27/2001";
#else
__RCSID("$NetBSD: screen.c,v 1.37 2022/04/08 10:17:52 andvar Exp $");
#endif
#endif					/* not lint */

#include <stdlib.h>

#include "curses.h"
#include "curses_private.h"

static int filtered;

static void	 __delscreen(SCREEN *);

/*
 * filter has to be called before either initscr or newterm.
 */
void
filter(void)
{

	filtered = TRUE;
}

/*
 * set_term --
 *      Change the term to the given screen.
 *
 */
SCREEN *
set_term(SCREEN *new)
{
	SCREEN *old_screen = _cursesi_screen;

	if (_cursesi_screen != NULL) {
		  /* save changes made to the current screen... */
		old_screen->echoit = __echoit;
		old_screen->pfast = __pfast;
		old_screen->rawmode = __rawmode;
		old_screen->noqch = __noqch;
		old_screen->COLS = COLS;
		old_screen->LINES = LINES;
		old_screen->ESCDELAY = ESCDELAY;
		old_screen->TABSIZE = TABSIZE;
		old_screen->COLORS = COLORS;
		old_screen->COLOR_PAIRS = COLOR_PAIRS;
		old_screen->GT = __GT;
		old_screen->NONL = __NONL;
		old_screen->UPPERCASE = __UPPERCASE;
	}

	_cursesi_screen = new;

	__echoit = new->echoit;
        __pfast = new->pfast;
	__rawmode = new->rawmode;
	__noqch = new->noqch;
	COLS = new->COLS;
	LINES = new->LINES;
	ESCDELAY = new->ESCDELAY;
	TABSIZE = new->TABSIZE;
	COLORS = new->COLORS;
	COLOR_PAIRS = new->COLOR_PAIRS;
	__GT = new->GT;
	__NONL = new->NONL;
	__UPPERCASE = new->UPPERCASE;

	_cursesi_resetterm(new);

	curscr = new->curscr;
	clearok(curscr, new->clearok);
	stdscr = new->stdscr;
	__virtscr = new->__virtscr;

	_cursesi_reset_acs(new);
#ifdef HAVE_WCHAR
	_cursesi_reset_wacs(new);
#endif /* HAVE_WCHAR */

	__CTRACE(__CTRACE_SCREEN, "set_term: LINES = %d, COLS = %d\n",
	    LINES, COLS);

	return old_screen;
}

/*
 * newterm --
 *      Set up a new screen.
 *
 */
SCREEN *
newterm(char *type, FILE *outfd, FILE *infd)
{
	SCREEN *new_screen;
	char *sp;

	sp = type;
	if (type == NULL && (sp = getenv("TERM")) == NULL)
		return NULL;

	if ((new_screen = calloc(1, sizeof(SCREEN))) == NULL)
		return NULL;

	__CTRACE(__CTRACE_INIT, "newterm\n");

	new_screen->infd = infd;
	/*
	 * POSIX standard says this should be set to infd by default,
	 * but this seems to break nvi by leaving an unrefreshed screen.
	 * Also, the line breakout optimisation advertised in ncurses
	 * doesn't actually do anything, so explicitly disabling it here makes
	 * sense for the time being.
	 * A caller can always enable it by calling typeahead(3) anyway.
	 */
	new_screen->checkfd = -1; // fileno(infd);
	new_screen->outfd = outfd;
	new_screen->echoit = new_screen->nl = 1;
	new_screen->pfast = new_screen->rawmode = new_screen->noqch = 0;
	new_screen->filtered = filtered;
	filtered = FALSE; /* filter() must precede each newterm() */
	new_screen->nca = A_NORMAL;
	new_screen->color_type = COLOR_NONE;
	new_screen->COLOR_PAIRS = 0;
	new_screen->old_mode = 2;
	new_screen->stdbuf = NULL;
	new_screen->stdscr = NULL;
	new_screen->curscr = NULL;
	new_screen->__virtscr = NULL;
	new_screen->curwin = 0;
	new_screen->notty = FALSE;
	new_screen->resized = 0;
	new_screen->unget_len = 32;

	if ((new_screen->unget_list =
	    malloc(sizeof(wchar_t) * new_screen->unget_len)) == NULL)
	{
		goto error_exit;
	}
	new_screen->unget_pos = 0;

	if (_cursesi_gettmode(new_screen) == ERR)
		goto error_exit;

	if (_cursesi_setterm((char *)sp, new_screen) == ERR)
		goto error_exit;

	/* Need either homing or cursor motion for refreshes */
	if (!t_cursor_home(new_screen->term) &&
	    !t_cursor_address(new_screen->term))
		goto error_exit;

	new_screen->winlistp = NULL;

	if ((new_screen->curscr = __newwin(new_screen, 0,
	    0, 0, 0, FALSE, FALSE)) == NULL)
		goto error_exit;

	if ((new_screen->__virtscr = __newwin(new_screen, 0,
	    0, 0, 0, FALSE, FALSE)) == NULL)
		goto error_exit;

	/* If Soft Label Keys are setup, they will ripoffline. */
	if (__slk_init(new_screen) == ERR)
		goto error_exit;

	if (__ripoffscreen(new_screen) == ERR)
		goto error_exit;

	new_screen->stdscr = __newwin(new_screen, 0, 0, 0, 0, FALSE, TRUE);
	if (new_screen->stdscr == NULL)
		goto error_exit;

	clearok(new_screen->stdscr, 1);

	__init_getch(new_screen);
	__init_acs(new_screen);
#ifdef HAVE_WCHAR
	__init_get_wch(new_screen);
	__init_wacs(new_screen);
#endif /* HAVE_WCHAR */

	__set_stophandler();
	__set_winchhandler();

	  /*
	   * bleh - it seems that apps expect the first newterm to set
	   * up the curses screen.... emulate this.
	   */
	if (_cursesi_screen == NULL || _cursesi_screen->endwin) {
		set_term(new_screen);
	}

	__CTRACE(__CTRACE_SCREEN, "newterm: LINES = %d, COLS = %d\n",
	    LINES, COLS);
	__startwin(new_screen);

	return new_screen;

  error_exit:
	__delscreen(new_screen);
	free(new_screen->unget_list);

	free(new_screen);
	return NULL;
}

/*
 * delscreen --
 *   Free resources used by the given screen and destroy it.
 *
 */
void
delscreen(SCREEN *screen)
{

	__CTRACE(__CTRACE_SCREEN, "delscreen(%p)\n", screen);

	__delscreen(screen);

	  /* free the storage of the keymaps */
	_cursesi_free_keymap(screen->base_keymap);

	  /* free the Soft Label Keys */
	__slk_free(screen);

	free(screen->stdbuf);
	free(screen->unget_list);
	if (_cursesi_screen == screen)
		_cursesi_screen = NULL;
	free(screen);
}

static void
__delscreen(SCREEN *screen)
{
        struct __winlist *list;

	  /* free up the terminfo stuff */
	if (screen->term != NULL)
		del_curterm(screen->term);

	  /* walk the window list and kill all the parent windows */
	while ((list = screen->winlistp) != NULL) {
		delwin(list->winp);
		if (list == screen->winlistp)
			/* sanity - abort if window didn't remove itself */
			break;
	}
}
