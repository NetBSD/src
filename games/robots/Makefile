#	$NetBSD: Makefile,v 1.10 1997/11/20 00:12:38 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	robots
CPPFLAGS+=-DMAX_PER_UID=5
SRCS=	extern.c init_field.c main.c make_level.c move.c move_robs.c \
	play_level.c query.c rnd_pos.c score.c flush_in.c
MAN=	robots.6
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermlib
HIDEGAME=hidegame
SETGIDGAME=yes

FILES=	/dev/null
FILESNAME=robots_roll
FILESMODE=600
FILESDIR=/var/games

.include <bsd.prog.mk>
