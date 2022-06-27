#	$NetBSD: Makefile,v 1.17 2022/06/27 18:48:49 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	robots
CPPFLAGS+=-DMAX_PER_UID=5 -DFANCY
SRCS=	auto.c extern.c init_field.c main.c make_level.c move.c move_robs.c \
	play_level.c query.c rnd_pos.c score.c flush_in.c
MAN=	robots.6
DPADD=	${LIBCURSES} ${LIBTERMINFO}
LDADD=	-lcurses -lterminfo
HIDEGAME=hidegame
SETGIDGAME=yes

.include <bsd.prog.mk>
