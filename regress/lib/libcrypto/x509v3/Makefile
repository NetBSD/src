#	$NetBSD: Makefile,v 1.2 2001/10/31 15:57:55 tv Exp $

PROG=	x509v3test
SRCS=	tabtest.c
NOMAN=	yes

LDADD+=	-lcrypto
DPADD+=	${LIBSSL} ${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/x509v3
