#	$NetBSD: Makefile,v 1.4 1998/02/18 22:37:31 jtc Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/24/94

PROG=	gomoku
SRCS=	bdinit.c bdisp.c main.c makemove.c pickmove.c stoc.c
MAN=	gomoku.6
DPADD=	${LIBCURSES}
LDADD=	-lcurses
HIDEGAME=hidegame

.include <bsd.prog.mk>
