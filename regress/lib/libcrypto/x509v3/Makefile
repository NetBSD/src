#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:11 tv Exp $

PROG=		x509v3test
SRCS=		tabtest.c
NOMAN=		# defined
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/x509v3
