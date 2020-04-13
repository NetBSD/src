#	$NetBSD: Makefile,v 1.7.46.1 2020/04/13 08:03:07 martin Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	tetris
SRCS=	input.c screen.c shapes.c scores.c tetris.c
MAN=	tetris.6
DPADD=	${LIBTERMINFO}
LDADD=	-lterminfo
HIDEGAME=hidegame
SETGIDGAME=yes

COPTS.tetris.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
