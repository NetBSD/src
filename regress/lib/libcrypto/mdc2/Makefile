#	$NetBSD: Makefile,v 1.6 2003/07/24 14:22:56 itojun Exp $

NOMAN=		# defined

PROG=		mdc2test
DPADD+=		${LIBCRYPTO_MDC2} ${LIBCRYPTO}
LDADD+=		-lcrypto_mdc2 -lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${OPENSSLSRC}/crypto/mdc2
