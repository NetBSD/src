#	$NetBSD: Makefile,v 1.1 2000/10/03 15:13:22 itojun Exp $

PROG=	bftest
NOMAN=	yes

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@${.OBJDIR}/${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/bf
