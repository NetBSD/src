#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:08 tv Exp $

PROG=		md4test
NOMAN=		# defined
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/md4
