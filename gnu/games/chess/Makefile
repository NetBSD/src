#	$NetBSD: Makefile,v 1.17 1999/02/13 02:54:22 lukem Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

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
