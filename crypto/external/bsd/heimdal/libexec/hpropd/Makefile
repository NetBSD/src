# $NetBSD: Makefile,v 1.3 2023/06/19 23:11:44 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/kdc ${HEIMDIST}/lib/hcrypto

PROG=	hpropd
SRCS=	hpropd.c ui.c
MAN=	hpropd.8

CPPFLAGS+= 				\
	-I${HEIMBASE}/include/krb5	\
	-I${HEIMDIST}/lib/asn1		\
	-I${HEIMDIST}/lib/krb5		\
	-I${HEIMDIST}/lib

LDADD+= -lhdb
DPADD+= ${LIBHDB}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
