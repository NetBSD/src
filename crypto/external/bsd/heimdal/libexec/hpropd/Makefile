# $NetBSD: Makefile,v 1.5 2023/06/22 22:39:37 christos Exp $

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
