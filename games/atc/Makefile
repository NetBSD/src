#	$NetBSD: Makefile,v 1.26.12.1 2009/05/13 19:18:04 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

PROG=	atc
CPPFLAGS+=-DBSD -I${.CURDIR} -I. -DYY_NO_UNPUT -DYY_NO_INPUT
SRCS=	extern.c grammar.y graphics.c input.c lex.l list.c log.c \
	main.c tunable.c update.c
YHEADER=1
MAN=	atc.6
LDADD=	-ll -lm -lcurses
DPADD=	${LIBL} ${LIBM} ${LIBCURSES}
GAMES=	Game_List Killer crossover default easy game_2 \
	Atlantis OHare Tic-Tac-Toe airports box crosshatch game_3 \
	game_4 novice two-corners
HIDEGAME=hidegame
SETGIDGAME=yes
.if ${MKSHARE} != "no"
FILES=${GAMES:S@^@${.CURDIR}/games/@g}
FILESDIR=/usr/share/games/atc
FILESMODE=444
.endif

.include <bsd.prog.mk>
