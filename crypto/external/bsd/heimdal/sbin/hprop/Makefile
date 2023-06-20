# $NetBSD: Makefile,v 1.3 2023/06/20 00:30:17 mrg Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes		# network client

.PATH: ${HEIMDIST}/kdc

PROG= hprop

SRCS+= 	hprop.c mit_dump.c

MAN=	hprop.8

CPPFLAGS+= 				\
	-I${HEIMBASE}/include/krb5	\
	-I${HEIMBASE}/lib/libkrb5	\
	-I${HEIMDIST}/lib/asn1		\
	-I${HEIMDIST}/lib/hdb		\
	-I${HEIMDIST}/lib/krb5		\
	-I${HEIMDIST}/lib

LDADD+= -lhdb -lutil
DPADD+= ${LIBHDB} ${LIBUTIL}

# XXX: Pull this in from hcrypto.  Be sure to be last!
SRCS+=	ui.c
.PATH:	${HEIMDIST}/lib/hcrypto
CPPFLAGS.ui.c+= 		\
	-I${HEIMDIST}/lib/hcrypto

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
