#	$NetBSD: Makefile,v 1.5 2003/07/24 14:22:57 itojun Exp $

NOMAN=		# defined

PROG=		rc5test
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto_rc5 -lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${OPENSSLSRC}/crypto/rc5
