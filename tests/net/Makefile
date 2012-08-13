# $NetBSD: Makefile,v 1.11 2012/08/13 11:15:05 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass route sys
.if (${MKRUMP} != "no")
TESTS_SUBDIRS+=		bpf carp icmp if if_loop net
.endif

.include <bsd.test.mk>
