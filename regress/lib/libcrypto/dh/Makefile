#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:44 lukem Exp $

NOMAN=		# defined

PROG=		dhtest
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/dh
