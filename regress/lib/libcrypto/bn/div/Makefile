#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:43 lukem Exp $

NOMAN=		# defined

PROG=		divtest
CFLAGS+=	-g
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG} 1000

.include <bsd.prog.mk>
