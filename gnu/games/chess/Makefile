#	@(#)Makefile	5.4 (Berkeley) 5/11/90

PROG=	chess
SRCS=	gnuchess.c uxdsp.c move.c
CFLAGS+=-DNEWMOVE=12
MAN6=	chess.0
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermlib
HIDEGAME=hidegame
MANALL+= chess.6
CLEANFILES += chess.6

chess.6:
	@if [ -d ${.CURDIR}/obj ]; then \
		/bin/rm -rf ${.CURDIR}/obj/chess.6 ; \
		ln -s ${.CURDIR}/DOCUMENTATION/MAN-PAGE ${.CURDIR}/obj/chess.6 ; \
	else \
		true; \
		/bin/rm -rf ${.CURDIR}/chess.6 ; \
		ln -s ${.CURDIR}/DOCUMENTATION/MAN-PAGE ${.CURDIR}/chess.6 ; \
	fi

beforeinstall: 
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/gnuchess.book \
	    ${DESTDIR}/usr/share/games

.include <bsd.prog.mk>
