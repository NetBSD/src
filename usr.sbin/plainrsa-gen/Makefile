# $NetBSD: Makefile,v 1.7.2.1 2025/11/20 18:46:41 martin Exp $

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
