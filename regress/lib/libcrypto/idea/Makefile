#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:44 lukem Exp $

NOMAN=		# defined

PROG=		ideatest
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto_idea -lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/idea
