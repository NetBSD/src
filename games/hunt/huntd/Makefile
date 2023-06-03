#	$NetBSD: Makefile,v 1.11 2023/06/03 09:09:09 lukem Exp $

PROG=	huntd
SRCS=	answer.c draw.c driver.c execute.c expl.c \
	extern.c makemaze.c shots.c support.c terminal.c
MAN=	huntd.6

CPPFLAGS+=-I${.CURDIR}/../include

.include <bsd.own.mk>

# Appears wrong - should not exceed bounds of array.
COPTS.answer.c+=	${CC_WNO_STRINGOP_OVERFLOW}
COPTS.shots.c+=		${CC_WNO_STRINGOP_OVERFLOW}
COPTS.driver.c+=	${CC_WNO_STRINGOP_OVERFLOW}

.include <bsd.prog.mk>
