#	from: @(#)Makefile	5.7 (Berkeley) 6/27/90
#	$Id: Makefile,v 1.5 1993/12/02 18:18:02 jtc Exp $

PROG=	atc
CFLAGS+=-DBSD -I${.CURDIR} -I.
SRCS=	extern.c grammar.c graphics.c input.c lex.c list.c log.c \
	main.c tunable.c update.c
MAN6=	atc.0
LDADD=	-ll -lm -lcurses -ltermcap
DPADD=	${LIBL} ${LIBM} ${LIBTERM} ${LIBCURSES}
GAMES=	ATC_scores Game_List Killer crossover default easy game_2
CLEANFILES=grammar.c y.tab.h lex.c
HIDEGAME=hidegame

beforeinstall: 
	@if [ ! -d ${DESTDIR}/usr/share/games/atc ]; then \
                /bin/rm -f ${DESTDIR}/usr/share/games/atc ; \
                mkdir -p ${DESTDIR}/usr/share/games/atc ; \
                chown root.wheel ${DESTDIR}/usr/share/games/atc ; \
                chmod 755 ${DESTDIR}/usr/share/games/atc ; \
        else \
                true ; \
        fi
	(cd ${.CURDIR}/games; install -c -o ${BINOWN} -g ${BINGRP} -m 400 \
	    ${GAMES} ${DESTDIR}/usr/share/games/atc)

.include <bsd.prog.mk>
