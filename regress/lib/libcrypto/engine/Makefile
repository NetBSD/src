#	$NetBSD: Makefile,v 1.2 2005/03/09 03:11:21 christos Exp $

NOMAN=		# defined

PROG=		enginetest
DPADD+=		${LIBCRYPTO} ${LIBCRYPT}
LDADD+=		-lcrypto -lcrypt

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${OPENSSLSRC}/crypto/engine
