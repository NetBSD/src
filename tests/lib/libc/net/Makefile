# $NetBSD: Makefile,v 1.3 2011/01/13 01:56:44 pgoyette Exp $

.include <bsd.own.mk>

MKMAN=	no

TESTS_SUBDIRS+=	getaddrinfo

TESTSDIR=	${TESTSBASE}/lib/libc/net

TESTS_SH+=	t_nsdispatch
TESTS_SH+=	t_protoent
TESTS_SH+=	t_servent

BINDIR=		${TESTSDIR}

PROGS+=		h_nsd_recurse
PROGS+=		h_protoent
PROGS+=		h_servent

LDADD.h_nsd_dispatch+=	-lpthread

.include <bsd.test.mk>
