# $NetBSD: Makefile,v 1.9.4.2 2014/05/22 11:42:22 yamt Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass net route sys
.if (${MKRUMP} != "no")
TESTS_SUBDIRS+=		bpf bpfilter carp icmp if if_loop mpls npf
.endif

.include <bsd.test.mk>
