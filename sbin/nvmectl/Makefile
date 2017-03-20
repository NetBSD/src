#	$NetBSD: Makefile,v 1.1.2.1 2017/03/20 06:57:02 pgoyette Exp $

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
