#	$NetBSD: Makefile,v 1.10 2023/06/03 09:09:09 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	tetris
SRCS=	input.c screen.c shapes.c scores.c tetris.c
MAN=	tetris.6
DPADD=	${LIBTERMINFO}
LDADD=	-lterminfo
HIDEGAME=hidegame
SETGIDGAME=yes

COPTS.tetris.c+=	${CC_WNO_FORMAT_TRUNCATION}
COPTS.scores.c+=	${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
