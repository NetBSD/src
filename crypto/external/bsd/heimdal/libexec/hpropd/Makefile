# $NetBSD: Makefile,v 1.2.56.1 2023/08/11 13:40:08 martin Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/kdc

PROG=	hpropd
SRCS=	hpropd.c
MAN=	hpropd.8

CPPFLAGS+= 				\
	-I${HEIMBASE}/include/krb5	\
	-I${HEIMDIST}/lib/asn1		\
	-I${HEIMDIST}/lib/krb5

PROGDPLIBS+= ${DPLIBHDB}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+= ${KRB5LDADD}
DPADD+= ${KRB5DPADD}
