#	$NetBSD: Makefile,v 1.1.2.2 2000/10/03 15:13:24 itojun Exp $

PROG=	divtest
NOMAN=	yes

CFLAGS+=	-g
LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@${.OBJDIR}/${PROG} 1000

.include <bsd.prog.mk>
