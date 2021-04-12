#	$NetBSD: Makefile,v 1.9 2021/04/12 02:54:08 mrg Exp $

PROG=	huntd
SRCS=	answer.c draw.c driver.c execute.c expl.c \
	extern.c makemaze.c shots.c support.c terminal.c
MAN=	huntd.6

CPPFLAGS+=-I${.CURDIR}/../include

.include <bsd.own.mk>

# Appears wrong - should not exceed bounds of array.
COPTS.answer.c+=	${GCC_NO_STRINGOP_OVERFLOW}
COPTS.shots.c+=		${GCC_NO_STRINGOP_OVERFLOW}

.include <bsd.prog.mk>
