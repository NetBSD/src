#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:44 lukem Exp $

NOMAN=		# defined

PROG=		lhashtest
SRCS=		lh_test.c
CPPFLAGS+=	-DMalloc=malloc
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

regress:	${PROG}
	@echo "==> running ${PROG}"
	@echo hoge | ./${PROG}

.include <bsd.prog.mk>

.PATH:	${CRYPTODIST}/lhash
