# $NetBSD: Makefile,v 1.21 2015/05/20 17:39:04 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass in_cksum mcast net sys
.if (${MKRUMP} != "no")
TESTS_SUBDIRS+=		bpf bpfilter carp icmp if if_bridge if_loop mpls npf
TESTS_SUBDIRS+=		route
.if (${MKSLJIT} != "no")
TESTS_SUBDIRS+=		bpfjit
.endif
.endif

.include <bsd.test.mk>
