#	$NetBSD: Makefile,v 1.6 1996/10/18 02:50:15 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	robots
CFLAGS+=-DMAX_PER_UID=5
SRCS=	extern.c init_field.c main.c make_level.c move.c move_robs.c \
	play_level.c query.c rnd_pos.c score.c flush_in.c
MAN=	robots.6
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermlib
HIDEGAME=hidegame

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
	    ${DESTDIR}/var/games/robots_roll

.include <bsd.prog.mk>
