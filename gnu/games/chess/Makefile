#	$NetBSD: Makefile,v 1.8 1996/10/18 02:52:25 thorpej Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

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
