#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.6 1993/12/28 11:48:14 davidb Exp $

PROG=	chess
SRCS=	gnuchess.c uxdsp.c move.c
CFLAGS+=-DNEWMOVE=12
MAN6=	chess.0
DPADD=	${LIBCURSES} ${LIBTERM} /usr/lib/libgnumalloc.a
LDADD=	-lcurses -ltermlib -lgnumalloc
HIDEGAME=hidegame
MANALL+= chess.6
CLEANFILES += chess.6

chess.6:
	/bin/rm -rf ${.OBJDIR}/chess.6
	ln -s ${.CURDIR}/DOCUMENTATION/MAN-PAGE ${.OBJDIR}/chess.6

beforeinstall: 
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/gnuchess.book \
	    ${DESTDIR}/usr/share/games

.include <bsd.prog.mk>
