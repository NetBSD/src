#	$NetBSD: Makefile,v 1.1 2000/10/03 15:13:23 itojun Exp $

PROG=	conftest
SRCS=	test.c
NOMAN=	yes

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@${.OBJDIR}/${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/conf
