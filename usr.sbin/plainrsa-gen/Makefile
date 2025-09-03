# $NetBSD: Makefile,v 1.7 2025/07/19 16:14:55 christos Exp $

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
