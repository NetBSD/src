#	$NetBSD: Makefile,v 1.2 2001/10/31 15:57:50 tv Exp $

PROG=	bntest
NOMAN=	yes

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>
