#	$NetBSD: Makefile,v 1.6 2005/03/09 03:11:22 christos Exp $

NOMAN=		# defined

PROG=		lhashtest
SRCS=		lh_test.c
CPPFLAGS+=	-DMalloc=malloc
DPADD+=		${LIBCRYPTO} ${LIBCRYPT}
LDADD+=		-lcrypto -lcrypt

regress:	${PROG}
	@echo "==> running ${PROG}"
	@echo hoge | ./${PROG}

.include <bsd.prog.mk>

.PATH:	${OPENSSLSRC}/crypto/lhash
