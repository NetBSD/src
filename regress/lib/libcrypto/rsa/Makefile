#	$NetBSD: Makefile,v 1.6 2003/07/24 14:22:57 itojun Exp $

NOMAN=		# defined

PROG=		rsatest
SRCS=		rsa_test.c
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

CPPFLAGS+=	-I${OPENSSLSRC}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${OPENSSLSRC}/crypto/rsa
