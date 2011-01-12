# $NetBSD: Makefile,v 1.2 2011/01/12 17:32:27 pgoyette Exp $

.include <bsd.own.mk>

MKMAN=	no

TESTS_SUBDIRS+=	getaddrinfo

TESTSDIR=	${TESTSBASE}/lib/libc/net

TESTS_SH+=	t_protoent
TESTS_SH+=	t_servent

BINDIR=		${TESTSDIR}

PROGS+=		h_protoent
PROGS+=		h_servent

.include <bsd.test.mk>
