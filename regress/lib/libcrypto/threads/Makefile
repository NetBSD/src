#	$NetBSD: Makefile,v 1.1 2000/10/03 15:13:27 itojun Exp $

PROG=	threadstest
SRCS=	mttest.c
NOMAN=	yes

LDADD+=	-lssl -lcrypto
DPADD+=	${LIBSSL} ${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@${.OBJDIR}/${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/threads
