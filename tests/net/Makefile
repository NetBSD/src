# $NetBSD: Makefile,v 1.14.2.1 2013/07/23 21:07:38 riastradh Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass net route sys
.if (${MKRUMP} != "no")
TESTS_SUBDIRS+=		bpf bpfilter carp icmp if if_loop mpls npf
.endif

.include <bsd.test.mk>
