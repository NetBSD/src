#	$NetBSD: Makefile,v 1.6.26.1 2019/06/10 22:05:11 christos Exp $

PROG=	huntd
SRCS=	answer.c draw.c driver.c execute.c expl.c \
	extern.c makemaze.c shots.c support.c terminal.c
MAN=	huntd.6

CPPFLAGS+=-I${.CURDIR}/../include

.include <bsd.own.mk>

# Has fallthru comment that is missed
.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.shots.c+=        -Wno-error=implicit-fallthrough
.endif

.include <bsd.prog.mk>
