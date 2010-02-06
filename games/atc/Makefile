#	$NetBSD: Makefile,v 1.29 2010/02/06 23:45:24 he Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

PROG=	atc
CPPFLAGS+=-DBSD -I${.CURDIR} -I.
SRCS=	extern.c grammar.y graphics.c input.c lex.l list.c log.c \
	main.c tunable.c update.c
YHEADER=1
MAN=	atc.6
LDADD=	-ll -lm -lcurses -lterminfo
DPADD=	${LIBL} ${LIBM} ${LIBCURSES} ${LIBTERMINFO}
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
