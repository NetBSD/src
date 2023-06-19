# $NetBSD: Makefile,v 1.3 2023/06/19 23:11:44 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/kdc ${HEIMDIST}/lib/hcrypto

PROG=	digest-service
SRCS=	digest-service.c ui.c
MAN=

CPPFLAGS+= 				\
	-I${HEIMBASE}/include/krb5	\
	-I${HEIMDIST}/lib/krb5		\
	-I${HEIMDIST}/lib/ipc		\
	-I${HEIMDIST}/lib

LDADD+= -lkdc -lhdb
DPADD+= ${LIBKDC} ${LIBHDB}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
