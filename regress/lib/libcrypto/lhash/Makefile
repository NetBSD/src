#	$NetBSD: Makefile,v 1.2 2001/10/31 15:57:52 tv Exp $

PROG=	lhashtest
SRCS=	lh_test.c
NOMAN=	yes

CPPFLAGS+=-DMalloc=malloc

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}

regress:	${PROG}
	@echo "==> running ${PROG}"
	@echo hoge | ./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/lhash
