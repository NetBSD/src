# $NetBSD: Makefile,v 1.3 2018/05/09 13:18:02 joerg Exp $

TESTSDIR=	${TESTSBASE}/lib/libobjc

.include <bsd.own.mk>

UNSUPPORTED_COMPILER.clang=	# defined
UNSUPPORTED_COMPILER.pcc=	# defined

.if ${HAVE_GCC:U0} > 0
TESTS_C=	t_threads
.endif

SRCS.t_threads=	t_threads.m
WARNS?=		4
CFLAGS+=	-pthread
LDFLAGS+=	-pthread
DPADD+=		${LIBOBJC}
LDADD+=		-lobjc

.include <bsd.test.mk>
