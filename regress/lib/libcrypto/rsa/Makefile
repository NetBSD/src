#	$NetBSD: Makefile,v 1.5 2002/09/18 05:41:47 lukem Exp $

NOMAN=		# defined

PROG=		rsatest
SRCS=		rsa_test.c
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

CPPFLAGS+=	-I${CRYPTODIST}/..

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/rsa
