#	$NetBSD: Makefile,v 1.7 2022/05/16 20:57:01 rillig Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/24/94

PROG=	gomoku
SRCS=	bdinit.c bdisp.c main.c makemove.c pickmove.c stoc.c
MAN=	gomoku.6
DPADD=	${LIBCURSES} ${LIBTERMINFO}
LDADD=	-lcurses -lterminfo
HIDEGAME=hidegame

LINTFLAGS+=	-w	# treat warnings as errors
#LINTFLAGS+=	-T	# strict bool mode

.include <bsd.prog.mk>
