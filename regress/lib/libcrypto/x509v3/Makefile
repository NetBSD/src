#	$NetBSD: Makefile,v 1.1 2000/10/03 15:13:27 itojun Exp $

PROG=	x509v3test
SRCS=	tabtest.c
NOMAN=	yes

LDADD+=	-lcrypto
DPADD+=	${LIBSSL} ${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@${.OBJDIR}/${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/x509v3
