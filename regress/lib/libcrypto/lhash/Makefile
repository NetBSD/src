#	$NetBSD: Makefile,v 1.1 2000/10/03 15:13:24 itojun Exp $

PROG=	lhashtest
SRCS=	lh_test.c
NOMAN=	yes

CPPFLAGS+=-DMalloc=malloc

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@echo hoge | ${.OBJDIR}/${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/lhash
