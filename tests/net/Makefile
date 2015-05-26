# $NetBSD: Makefile,v 1.22 2015/05/26 00:42:07 ozaki-r Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass in_cksum net sys
.if (${MKRUMP} != "no")
TESTS_SUBDIRS+=		bpf bpfilter carp icmp if if_bridge if_loop mcast
TESTS_SUBDIRS+=		mpls npf route
.if (${MKSLJIT} != "no")
TESTS_SUBDIRS+=		bpfjit
.endif
.endif

.include <bsd.test.mk>
