#	$NetBSD: Makefile,v 1.2 2001/10/31 15:57:54 tv Exp $

PROG=	rc5test
NOMAN=	yes

LDADD+=	-lcrypto_rc5 -lcrypto
DPADD+=	${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/rc5
