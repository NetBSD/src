#	$NetBSD: Makefile,v 1.11 1997/03/24 22:15:37 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	atc
CFLAGS+=-DBSD -I${.CURDIR} -I.
SRCS=	extern.c grammar.c graphics.c input.c lex.c list.c log.c \
	main.c tunable.c update.c
MAN=	atc.6
LDADD=	-ll -lm -lcurses -ltermcap
DPADD=	${LIBL} ${LIBM} ${LIBTERM} ${LIBCURSES}
GAMES=	ATC_scores Game_List Killer crossover default easy game_2 \
	Atlantis OHare Tic-Tac-Toe airports box crosshatch game_3 \
	game_4 novice two-corners
CLEANFILES=grammar.c y.tab.h lex.c
HIDEGAME=hidegame
FILES=${GAMES:S@^@${.CURDIR}/games/@g}
FILESDIR=/usr/share/games/atc
FILESMODE=400

.include <bsd.prog.mk>
