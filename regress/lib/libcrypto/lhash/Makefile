#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:07 tv Exp $

PROG=		lhashtest
SRCS=		lh_test.c
NOMAN=		# defined
CPPFLAGS+=	-DMalloc=malloc
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@echo hoge | ./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/lhash
