#	$NetBSD: Makefile,v 1.4 2002/08/31 10:46:44 itojun Exp $

PROG=		rsatest
SRCS=		rsa_test.c
NOMAN=		# defined
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

CPPFLAGS+=	-I${CRYPTODIST}/..

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/rsa
