#	$NetBSD: Makefile,v 1.7 1997/03/24 22:15:49 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	robots
CFLAGS+=-DMAX_PER_UID=5
SRCS=	extern.c init_field.c main.c make_level.c move.c move_robs.c \
	play_level.c query.c rnd_pos.c score.c flush_in.c
MAN=	robots.6
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermlib
HIDEGAME=hidegame

FILES=/dev/null
FILESNAME=robots_roll
FILESMODE=600
FILESDIR=/var/games

.include <bsd.prog.mk>
