#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:07 tv Exp $

PROG=		ideatest
NOMAN=		# defined
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto_idea -lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/idea
