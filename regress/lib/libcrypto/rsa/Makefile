#	$NetBSD: Makefile,v 1.7 2005/03/09 03:11:22 christos Exp $

NOMAN=		# defined

PROG=		rsatest
SRCS=		rsa_test.c
DPADD+=		${LIBCRYPTO} ${LIBCRYPT}
LDADD+=		-lcrypto -lcrypt

CPPFLAGS+=	-I${OPENSSLSRC}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${OPENSSLSRC}/crypto/rsa
