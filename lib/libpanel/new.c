/*	$NetBSD: new.c,v 1.3 2015/11/02 02:45:25 kamil Exp $ */

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
__RCSID("$NetBSD: new.c,v 1.3 2015/11/02 02:45:25 kamil Exp $");

#include "panel_impl.h"

#include <assert.h>
#include <stdlib.h>


static PANEL *
_new_panel(WINDOW *w)
{
	PANEL *p;

	p = (PANEL *)malloc(sizeof(PANEL));
	if (__predict_false(p == NULL))
		return NULL;

	p->win = w;
	p->user = NULL;

	DECK_INSERT_TOP(p);
	return p;
}


PANEL *
new_panel(WINDOW *w)
{

	if (__predict_false(w == NULL))
		return NULL;

	if (__predict_false(w == stdscr))
		return NULL;

	/*
	 * Ensure there's phantom panel for stdscr at (below) the
	 * bottom.  We explicitly re-assign stdscr in case it changed.
	 */
	if (TAILQ_EMPTY(&_deck)) {
		assert(PANEL_HIDDEN(&_stdscr_panel));

		_stdscr_panel.win = stdscr;
		DECK_INSERT_TOP(&_stdscr_panel);
	}

	return _new_panel(w);
}
