#	$NetBSD: Makefile,v 1.1 2003/07/24 14:22:54 itojun Exp $

NOMAN=		# defined

PROG=		enginetest
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${OPENSSLSRC}/crypto/engine
