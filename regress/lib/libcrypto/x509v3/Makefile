#	$NetBSD: Makefile,v 1.6 2005/03/09 03:11:22 christos Exp $

NOMAN=		# defined

PROG=		x509v3test
SRCS=		tabtest.c
DPADD+=		${LIBCRYPTO} ${LIBCRYPT}
LDADD+=		-lcrypto -lcrypt

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${OPENSSLSRC}/crypto/x509v3
