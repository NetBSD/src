#	$NetBSD: Makefile,v 1.6 2005/03/09 03:11:21 christos Exp $

NOMAN=		# defined

PROG=		conftest
SRCS=		test.c
DPADD+=		${LIBCRYPTO} ${LIBCRYPT}
LDADD+=		-lcrypto -lcrypt

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${OPENSSLSRC}/crypto/conf
