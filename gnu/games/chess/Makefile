#	$NetBSD: Makefile,v 1.11 1997/07/21 04:48:06 mrg Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

.if (${MACHINE_ARCH} != "alpha")
PROG=	chess
SRCS=	gnuchess.c uxdsp.c move.c
CFLAGS+=-DNEWMOVE=12
DPADD=	${LIBCURSES} ${LIBTERM} ${LIBGNUMALLOC}
LDADD=	-lcurses -ltermlib -lgnumalloc
HIDEGAME=hidegame
FILES=gnuchess.book
FILESDIR=/usr/share/games
.endif

MAN=	chess.6
CLEANFILES += chess.6

chess.6:
	/bin/rm -rf ${.OBJDIR}/chess.6
	ln -s ${.CURDIR}/DOCUMENTATION/MAN-PAGE ${.OBJDIR}/chess.6

.include <bsd.prog.mk>
