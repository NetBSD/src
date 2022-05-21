#	$NetBSD: Makefile,v 1.10 2022/05/21 14:55:26 rillig Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/24/94

PROG=	gomoku
SRCS=	bdinit.c bdisp.c main.c makemove.c pickmove.c stoc.c
MAN=	gomoku.6
DPADD=	${LIBCURSES} ${LIBTERMINFO}
LDADD=	-lcurses -lterminfo
HIDEGAME=hidegame
CPPFLAGS+=	${DEBUG:D-DDEBUG}

LINTFLAGS+=	-w	# treat warnings as errors
LINTFLAGS+=	-T	# strict bool mode
LINTFLAGS+=	-e	# strict enum checks

.include <bsd.prog.mk>
