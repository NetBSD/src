#	$NetBSD: Makefile,v 1.3 2015/11/22 04:16:22 kamil Exp $

WARNS=	4

LIB=	panel

INCS=	panel.h

SRCS=	#

SRCS+=	_deck.c

SRCS+=	above.c
SRCS+=	below.c
SRCS+=	bottom.c
SRCS+=	del.c
SRCS+=	getuser.c
SRCS+=	hidden.c
SRCS+=	hide.c
SRCS+=	move.c
SRCS+=	new.c
SRCS+=	replace.c
SRCS+=	setuser.c
SRCS+=	show.c
SRCS+=	top.c
SRCS+=	update.c
SRCS+=	window.c

#LIBDPLIBS+=	curses	${.CURDIR}/../libcurses

MAN=	#
MAN+=	move_panel.3
MAN+=	new_panel.3
MAN+=	panel.3
MAN+=	panel_above.3
MAN+=	panel_hidden.3
MAN+=	panel_userptr.3
MAN+=	update_panels.3

.include <bsd.lib.mk>
