#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:47 lukem Exp $

NOMAN=		# defined

PROG=		rc5test
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto_rc5 -lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/rc5
