# $NetBSD: Makefile,v 1.10 2012/08/08 13:57:06 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		route sys
.if (${MKRUMP} != "no")
TESTS_SUBDIRS+=		bpf carp icmp if if_loop net
.endif

.include <bsd.test.mk>
