#	$NetBSD: Makefile,v 1.2 2001/10/31 15:57:52 tv Exp $

PROG=	ideatest
NOMAN=	yes

LDADD+=	-lcrypto_idea -lcrypto
DPADD+=	${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/idea
