#	$NetBSD: Makefile,v 1.5 2003/07/14 13:27:40 itojun Exp $

NOMAN=		# defined

PROG=		mdc2test
DPADD+=		${LIBCRYPTO_MDC2} ${LIBCRYPTO}
LDADD+=		-lcrypto_mdc2 -lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/mdc2
