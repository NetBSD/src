#	$NetBSD: Makefile,v 1.2 2001/10/31 15:57:55 tv Exp $

PROG=	threadstest
SRCS=	mttest.c
NOMAN=	yes

LDADD+=	-lssl -lcrypto
DPADD+=	${LIBSSL} ${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/threads
