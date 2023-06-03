#	$NetBSD: Makefile,v 1.6 2023/06/03 08:52:56 lukem Exp $

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
SRCS+=	util.c
SRCS+=	wdc.c
SRCS+=	bignum.c
SRCS+=	humanize_bignum.c
MAN=	nvmectl.8

DPADD+= ${LIBUTIL}
LDADD+= -lutil

CWARNFLAGS.gcc+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

.include <bsd.prog.mk>
