/*	$NetBSD: del.c,v 1.3 2015/11/02 02:45:25 kamil Exp $ */

/*
 * Copyright (c) 2015 Valery Ushakov
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: del.c,v 1.3 2015/11/02 02:45:25 kamil Exp $");

#include "panel_impl.h"

#include <assert.h>
#include <stdlib.h>


int
del_panel(PANEL *p)
{

	if (__predict_false(p == NULL))
		return ERR;

	(void) hide_panel(p);
	free(p);

	/*
	 * If the last panel is removed, remove the phantom stdscr
	 * panel as well.
	 *
	 * A program that wants to switch to a different screen with
	 * set_term(3), or ends and recreates curses session with
	 * endwin(3)/initscr(3), must delete all panels first, since
	 * their windows will become invalid.  When it will create its
	 * first new panel afterwards, it will pick up new stdscr.
	 */
	if (TAILQ_LAST(&_deck, deck) == &_stdscr_panel) {
		(void) hide_panel(&_stdscr_panel);
		assert(TAILQ_EMPTY(&_deck));
	}

	return OK;
}
