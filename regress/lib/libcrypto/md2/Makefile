#	$NetBSD: Makefile,v 1.5 2003/07/24 14:22:55 itojun Exp $

NOMAN=		# defined

PROG=		md2test
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${OPENSSLSRC}/crypto/md2
