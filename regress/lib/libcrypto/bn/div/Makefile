#	$NetBSD: Makefile,v 1.2 2001/10/31 15:57:50 tv Exp $

PROG=	divtest
NOMAN=	yes

CFLAGS+=	-g
LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG} 1000

.include <bsd.prog.mk>
