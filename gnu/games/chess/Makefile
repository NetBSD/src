#	$NetBSD: Makefile,v 1.14 1998/02/18 22:47:41 jtc Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

.if (${MACHINE_ARCH} != "alpha")
PROG=	chess
SRCS=	gnuchess.c uxdsp.c move.c
CPPFLAGS+=-DNEWMOVE=12
DPADD=	${LIBCURSES} ${LIBGNUMALLOC}
LDADD=	-lcurses -lgnumalloc
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
