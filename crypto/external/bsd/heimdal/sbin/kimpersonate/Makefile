# $NetBSD: Makefile,v 1.1 2011/04/13 19:17:00 elric Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH: ${HEIMDIST}/kuser

PROG=	kimpersonate
SRCS=	kimpersonate.c
MAN=	kimpersonate.8

LDADD+=	-lkafs -lkrb5 -lhx509 -lheimntlm -lcrypto \
	-lasn1 -lcom_err ${LIBVERS} -lroken -lcrypt
DPADD+=	${LIBKAFS} ${LIBKRB5} ${LIBHX509} ${LIBHEIMNTLM} ${LIBCRYPTO} \
	${LIBASN1} ${LIBCOM_ERR} ${LIBVERS} ${LIBROKEN} ${LIBCRYPT}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
