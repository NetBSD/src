#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:09 tv Exp $

PROG=		rc5test
NOMAN=		# defined
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto_rc5 -lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/rc5
