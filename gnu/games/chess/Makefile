#	$NetBSD: Makefile,v 1.11.2.2 1998/05/08 05:33:49 mycroft Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

.if (${MACHINE_ARCH} != "alpha")
PROG=	chess
SRCS=	gnuchess.c uxdsp.c move.c
CPPFLAGS+=-DNEWMOVE=12
DPADD=	${LIBCURSES} ${LIBTERM} ${LIBGNUMALLOC}
LDADD=	-lcurses -ltermlib -lgnumalloc
HIDEGAME=hidegame
FILES=gnuchess.book
FILESDIR=/usr/share/games

MAN=	chess.6
CLEANFILES += chess.6

chess.6:
	/bin/rm -rf ${.OBJDIR}/chess.6
	ln -s ${.CURDIR}/DOCUMENTATION/MAN-PAGE ${.OBJDIR}/chess.6

.include <bsd.prog.mk>
