# $NetBSD: Makefile,v 1.16 2014/03/18 18:20:44 riastradh Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass net route sys
.if (${MKRUMP} != "no")
TESTS_SUBDIRS+=		bpf bpfilter carp icmp if if_loop mpls npf
.endif

.include <bsd.test.mk>
