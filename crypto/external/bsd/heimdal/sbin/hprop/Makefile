# $NetBSD: Makefile,v 1.4 2023/06/20 17:23:04 christos Exp $

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

PROGDPLIBS+= ${DPLIBHDB}
LDADD+= -lutil
DPADD+= ${LIBUTIL}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
