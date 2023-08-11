# $NetBSD: Makefile,v 1.2.56.1 2023/08/11 13:40:08 martin Exp $

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

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+= -lutil ${KRB5LDADD}
DPADD+= ${LIBUTIL} ${KRB5DPADD}
