#	$NetBSD: Makefile,v 1.7 2005/03/09 03:11:22 christos Exp $

NOMAN=		# defined

PROG=		mdc2test
DPADD+=		${LIBCRYPTO_MDC2} ${LIBCRYPTO} ${LIBCRYPT}
LDADD+=		-lcrypto_mdc2 -lcrypto -lcrypt

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${OPENSSLSRC}/crypto/mdc2
