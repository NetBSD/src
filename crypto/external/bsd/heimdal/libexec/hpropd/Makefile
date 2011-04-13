# $NetBSD: Makefile,v 1.1 2011/04/13 19:16:56 elric Exp $

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

LDADD+=		-lhdb -lkrb5 -lhx509 -lcrypto \
		-lasn1 -lcom_err ${LIBVERS} -lroken \
		-lcrypt
DPADD+=		${LIBHDB} ${LIBKRB5} ${LIBHX509} ${LIBCRYPTO} \
		${LIBASN1} ${LIBCOM_ERR} ${LIBVERS} ${LIBROKEN} \
		${LIBCRYPT}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
