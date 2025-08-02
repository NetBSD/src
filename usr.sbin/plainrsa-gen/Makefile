# $NetBSD: Makefile,v 1.2.2.1 2025/08/02 05:58:54 perseant Exp $

WARNS?=	3	# XXX third-party program, many issues

.include <bsd.own.mk>

PROG=	plainrsa-gen
SRCS+=	plainrsa-gen.c
MAN=	plainrsa-gen.8

CPPFLAGS+=-DNOUSE_PRIVSEP
LDADD+= -lcrypto -lipsec -lpthread
DPADD+= ${LIBCRYPTO} ${LIBIPSEC} ${LIBPTHREAD}

.include "${.CURDIR}/../racoon/Makefile.racoon"

.include <bsd.prog.mk>
