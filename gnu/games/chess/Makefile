#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.7 1994/12/22 09:31:26 cgd Exp $

PROG=	chess
SRCS=	gnuchess.c uxdsp.c move.c
CFLAGS+=-DNEWMOVE=12
MAN=	chess.6
DPADD=	${LIBCURSES} ${LIBTERM} /usr/lib/libgnumalloc.a
LDADD=	-lcurses -ltermlib -lgnumalloc
HIDEGAME=hidegame
CLEANFILES += chess.6

chess.6:
	/bin/rm -rf ${.OBJDIR}/chess.6
	ln -s ${.CURDIR}/DOCUMENTATION/MAN-PAGE ${.OBJDIR}/chess.6

beforeinstall: 
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/gnuchess.book \
	    ${DESTDIR}/usr/share/games

.include <bsd.prog.mk>
