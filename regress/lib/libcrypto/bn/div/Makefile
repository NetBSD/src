#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:05 tv Exp $

PROG=		divtest
NOMAN=		# defined
CFLAGS+=	-g
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG} 1000

.include <bsd.prog.mk>
