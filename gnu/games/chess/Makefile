#	$NetBSD: Makefile,v 1.13 1998/02/05 12:25:48 christos Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

.if (${MACHINE_ARCH} != "alpha")
PROG=	chess
SRCS=	gnuchess.c uxdsp.c move.c
CPPFLAGS+=-DNEWMOVE=12
DPADD=	${LIBCURSES} ${LIBTERMCAP} ${LIBGNUMALLOC}
LDADD=	-lcurses -ltermcap -lgnumalloc
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
