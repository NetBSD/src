#	$NetBSD: Makefile,v 1.5 2003/07/24 14:22:58 itojun Exp $

NOMAN=		# defined

PROG=		x509v3test
SRCS=		tabtest.c
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${OPENSSLSRC}/crypto/x509v3
