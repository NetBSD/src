#	$NetBSD: Makefile,v 1.2 2001/10/31 15:57:55 tv Exp $

PROG=	sha1test
NOMAN=	yes

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/sha
