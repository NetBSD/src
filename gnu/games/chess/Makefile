#	$NetBSD: Makefile,v 1.18 1999/02/13 22:29:15 cjs Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

.include <bsd.own.mk>

PROG=	chess
SRCS=	gnuchess.c uxdsp.c move.c
CPPFLAGS+=-DNEWMOVE=12
DPADD=	${LIBCURSES} ${LIBGNUMALLOC}
LDADD=	-lcurses -lgnumalloc
HIDEGAME=hidegame
.if ${MKSHARE} != "no"
FILES=gnuchess.book
FILESDIR=/usr/share/games
.endif

MAN=	chess.6
CLEANFILES += chess.6

chess.6:
	/bin/rm -rf ${.OBJDIR}/chess.6
	ln -s ${.CURDIR}/DOCUMENTATION/MAN-PAGE ${.OBJDIR}/chess.6

.include <bsd.prog.mk>
