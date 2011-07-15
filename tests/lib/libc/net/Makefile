# $NetBSD: Makefile,v 1.5 2011/07/15 06:41:29 jruoho Exp $

.include <bsd.own.mk>

MKMAN=	no

TESTS_SUBDIRS+=	getaddrinfo

TESTSDIR=	${TESTSBASE}/lib/libc/net

TESTS_C+=	t_getprotoent

TESTS_SH+=	t_nsdispatch
TESTS_SH+=	t_protoent
TESTS_SH+=	t_servent

BINDIR=		${TESTSDIR}

PROGS+=		h_nsd_recurse
PROGS+=		h_protoent
PROGS+=		h_servent

LDADD.h_nsd_recurse+=	-lpthread

.include <bsd.test.mk>
