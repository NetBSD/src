# $NetBSD: Makefile,v 1.23 2015/06/22 00:05:23 matt Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass in_cksum net sys
.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE)
TESTS_SUBDIRS+=		bpf bpfilter carp icmp if if_bridge if_loop mcast
TESTS_SUBDIRS+=		mpls npf route
.if (${MKSLJIT} != "no")
TESTS_SUBDIRS+=		bpfjit
.endif
.endif

.include <bsd.test.mk>
