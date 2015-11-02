/*	$NetBSD: panel.h,v 1.2 2015/11/02 01:06:15 kamil Exp $ */

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

#ifndef	_PANEL_H_
#define	_PANEL_H_

#include <sys/cdefs.h>
#include <curses.h>

typedef struct __panel PANEL;

__BEGIN_DECLS
PANEL  *new_panel(WINDOW *);
int     del_panel(PANEL *);

int     replace_panel(PANEL *, WINDOW *);
WINDOW *panel_window(PANEL *);

int     set_panel_userptr(PANEL *, char *);
char   *panel_userptr(PANEL *);

int     hide_panel(PANEL *);
int     show_panel(PANEL *);
int     panel_hidden(PANEL *);

int     top_panel(PANEL *);
int     bottom_panel(PANEL *);

PANEL  *panel_above(PANEL *);
PANEL  *panel_below(PANEL *);

int     move_panel(PANEL *, int, int);

void    update_panels(void);
__END_DECLS

#endif	/* _PANEL_H_ */
