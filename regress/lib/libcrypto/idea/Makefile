#	$NetBSD: Makefile,v 1.6 2005/03/09 03:11:22 christos Exp $

NOMAN=		# defined

PROG=		ideatest
DPADD+=		${LIBCRYPTO} ${LIBCRYPT}
LDADD+=		-lcrypto_idea -lcrypto -lcrypt

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${OPENSSLSRC}/crypto/idea
