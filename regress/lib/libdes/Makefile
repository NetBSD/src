#	$NetBSD: Makefile,v 1.1 2003/08/04 04:20:26 itojun Exp $

NOMAN=		# defined

PROG=		destest
DPADD+=		${LIBDES}
LDADD+=		-ldes

regress:	${PROG}
	@echo "==> running ${PROG}"
	@./${PROG}

.include <bsd.prog.mk>
