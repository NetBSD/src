#	$NetBSD: Makefile,v 1.5 2003/07/24 14:22:57 itojun Exp $

NOMAN=		# defined

PROG=		ripemdtest
SRCS=		rmdtest.c
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${OPENSSLSRC}/crypto/ripemd
