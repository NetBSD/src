#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:11 tv Exp $

PROG=		threadstest
SRCS=		mttest.c
NOMAN=		# defined
DPADD+=		${LIBSSL} ${LIBCRYPTO}
LDADD+=		-lssl -lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/threads
