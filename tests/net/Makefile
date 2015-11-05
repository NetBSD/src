# $NetBSD: Makefile,v 1.26 2015/11/05 00:02:59 knakahara Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass in_cksum net sys
.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE)
TESTS_SUBDIRS+=		arp bpf bpfilter carp icmp if if_bridge if_gif if_loop
TESTS_SUBDIRS+=		mcast mpls ndp npf route
.if (${MKSLJIT} != "no")
TESTS_SUBDIRS+=		bpfjit
.endif
.endif

.include <bsd.test.mk>
