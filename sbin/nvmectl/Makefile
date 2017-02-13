#	$NetBSD: Makefile,v 1.2 2017/02/13 11:16:46 nonaka Exp $

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
SRCS+=	bignum.c
SRCS+=	humanize_bignum.c
MAN=	nvmectl.8

DPADD+= ${LIBUTIL}
LDADD+= -lutil

.include <bsd.prog.mk>
