# $NetBSD: Makefile,v 1.6 2025/03/08 16:40:44 christos Exp $

WARNS?=	3	# XXX third-party program, many issues

.include <bsd.own.mk>

PROG=	plainrsa-gen
SRCS+=	plainrsa-gen.c
MAN=	plainrsa-gen.8

CPPFLAGS+=-DNOUSE_PRIVSEP
LDADD+= -lcrypto -lipsec
DPADD+= ${LIBCRYPTO} ${LIBIPSEC}

.include "${.CURDIR}/../racoon/Makefile.racoon"

.include <bsd.prog.mk>
