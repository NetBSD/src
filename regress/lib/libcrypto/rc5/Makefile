#	$NetBSD: Makefile,v 1.1 2000/10/03 15:13:26 itojun Exp $

PROG=	rc5test
NOMAN=	yes

LDADD+=	-lcrypto_rc5 -lcrypto
DPADD+=	${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@${.OBJDIR}/${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/rc5
