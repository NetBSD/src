/*	$NetBSD: panel_impl.h,v 1.2 2015/11/02 01:06:15 kamil Exp $ */

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

#ifndef _PANEL_IMPL_H_
#define _PANEL_IMPL_H_

#include "panel.h"

#include <sys/queue.h>

#define DECK_HEAD(head)		TAILQ_HEAD(head, __panel)
#define DECK_ENTRY		TAILQ_ENTRY(__panel)


/*
 * Panels are just curses windows with Z-order added.
 * See update_panels() for details.
 */
struct __panel {
	WINDOW *win;
	char *user;
	DECK_ENTRY zorder;
};


/* Deck of panels in Z-order from bottom to top. */
DECK_HEAD(deck);
extern struct deck _deck __dso_hidden;

/* Fake stdscr panel at the bottom, not user visible */
extern PANEL _stdscr_panel __dso_hidden;


/*
 * Hidden panels are not in the deck.  <sys/queue.h> macros don't have
 * a concept of an entry not on the list, so provide a kludge that
 * digs into internals.
 */
#define TAILQ_REMOVE_NP(head, elm, field) do {	\
	TAILQ_REMOVE((head), (elm), field);	\
	(elm)->field.tqe_next = NULL;		\
	(elm)->field.tqe_prev = NULL;		\
} while (/*CONSTCOND*/ 0)

#define TAILQ_LINKED_NP(elm, field) \
	(((elm)->field.tqe_prev) != NULL)


#define DECK_INSERT_TOP(p) do {					\
	TAILQ_INSERT_TAIL(&_deck, (p), zorder);			\
} while (/*CONSTCOND*/ 0)

#define DECK_INSERT_BOTTOM(p) do {				\
	TAILQ_INSERT_AFTER(&_deck, &_stdscr_panel, (p), zorder); \
} while (/*CONSTCOND*/ 0)

#define DECK_REMOVE(p) do {					\
	TAILQ_REMOVE_NP(&_deck, (p), zorder);			\
} while (/*CONSTCOND*/ 0)


#define PANEL_ABOVE(p)		(TAILQ_NEXT((p), zorder))
#define PANEL_BELOW(p)		(TAILQ_PREV((p), deck, zorder))
#define PANEL_HIDDEN(p)		(!TAILQ_LINKED_NP((p), zorder))

#define FOREACH_PANEL(var)	TAILQ_FOREACH(var, &_deck, zorder)

#endif	/* _PANEL_IMPL_H_ */
