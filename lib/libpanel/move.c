/*	$NetBSD: move.c,v 1.1 2015/10/26 23:09:49 uwe Exp $ */

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
__RCSID("$NetBSD: move.c,v 1.1 2015/10/26 23:09:49 uwe Exp $");

#include "panel_impl.h"


int
move_panel(PANEL *p, int y, int x)
{
	int oldy, oldx;

	if (__predict_false(p == NULL))
		return ERR;

	getbegyx(p->win, oldy, oldx);
	if (__predict_false(y == oldy && x == oldx))
		return OK;

	if (!PANEL_HIDDEN(p)) {
		PANEL *other;

		/* touch exposed areas at the old location now */
		FOREACH_PANEL (other) {
			if (other != p) {
				touchoverlap(p->win, other->win);
			}
		}
	}

	return mvwin(p->win, y, x);
}
