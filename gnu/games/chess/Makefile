#	$NetBSD: Makefile,v 1.15 1998/04/18 00:48:18 ross Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

PROG=	chess
SRCS=	gnuchess.c uxdsp.c move.c
CPPFLAGS+=-DNEWMOVE=12
DPADD=	${LIBCURSES} ${LIBGNUMALLOC}
LDADD=	-lcurses -lgnumalloc
HIDEGAME=hidegame
FILES=gnuchess.book
FILESDIR=/usr/share/games

MAN=	chess.6
CLEANFILES += chess.6

chess.6:
	/bin/rm -rf ${.OBJDIR}/chess.6
	ln -s ${.CURDIR}/DOCUMENTATION/MAN-PAGE ${.OBJDIR}/chess.6

.include <bsd.prog.mk>
