#	$NetBSD: Makefile,v 1.10 1997/03/24 22:06:04 christos Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

PROG=	chess
SRCS=	gnuchess.c uxdsp.c move.c
CFLAGS+=-DNEWMOVE=12
MAN=	chess.6
DPADD=	${LIBCURSES} ${LIBTERM} ${LIBGNUMALLOC}
LDADD=	-lcurses -ltermlib -lgnumalloc
HIDEGAME=hidegame
CLEANFILES += chess.6
FILES=gnuchess.book
FILESDIR=/usr/share/games

chess.6:
	/bin/rm -rf ${.OBJDIR}/chess.6
	ln -s ${.CURDIR}/DOCUMENTATION/MAN-PAGE ${.OBJDIR}/chess.6

.include <bsd.prog.mk>
