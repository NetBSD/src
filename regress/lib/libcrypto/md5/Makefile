#	$NetBSD: Makefile,v 1.2 2001/10/31 15:57:53 tv Exp $

PROG=	md5test
NOMAN=	yes

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/md5
