#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:48 lukem Exp $

NOMAN=		# defined

PROG=		threadstest
SRCS=		mttest.c
DPADD+=		${LIBSSL} ${LIBCRYPTO}
LDADD+=		-lssl -lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/threads
