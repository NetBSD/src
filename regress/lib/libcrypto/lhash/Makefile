#	$NetBSD: Makefile,v 1.5 2003/07/24 14:22:55 itojun Exp $

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

.PATH:	${OPENSSLSRC}/crypto/lhash
