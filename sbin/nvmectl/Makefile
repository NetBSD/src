#	$NetBSD: Makefile,v 1.1 2016/06/04 16:29:35 nonaka Exp $

.include <bsd.own.mk>

PROG=	nvmectl
SRCS=	nvmectl.c
SRCS+=	devlist.c
SRCS+=	firmware.c
SRCS+=	identify.c
SRCS+=	logpage.c
SRCS+=	perftest.c
SRCS+=	power.c
SRCS+=	reset.c
MAN=	nvmectl.8

DPADD+= ${LIBUTIL}
LDADD+= -lutil

.include <bsd.prog.mk>
