#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:06 tv Exp $

PROG=		conftest
SRCS=		test.c
NOMAN=		# defined
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/conf
