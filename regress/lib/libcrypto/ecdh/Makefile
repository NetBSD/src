#	$NetBSD: Makefile,v 1.1 2005/11/25 20:07:14 christos Exp $

NOMAN=		# defined

PROG=		ecdhtest
DPADD+=		${LIBCRYPTO} ${LIBCRYPT}
LDADD+=		-lcrypto -lcrypt

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${OPENSSLSRC}/crypto/ecdh
