#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:45 lukem Exp $

NOMAN=		# defined

PROG=		md5test
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/md5
