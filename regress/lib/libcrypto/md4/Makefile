#	$NetBSD: Makefile,v 1.1 2001/04/12 07:49:32 itojun Exp $

PROG=	md4test
NOMAN=	yes

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@${.OBJDIR}/${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/md4
