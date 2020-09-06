#	$NetBSD: Makefile,v 1.5 2020/09/06 07:20:28 mrg Exp $

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

CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.prog.mk>
