#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:48 lukem Exp $

NOMAN=		# defined

PROG=		x509v3test
SRCS=		tabtest.c
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/x509v3
