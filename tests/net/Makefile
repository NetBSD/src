# $NetBSD: Makefile,v 1.30.2.1 2017/01/15 20:29:01 bouyer Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass in_cksum net sys
.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE)
TESTS_SUBDIRS+=		arp bpf bpfilter can carp icmp if
TESTS_SUBDIRS+=		if_bridge if_gif if_loop if_pppoe if_tap if_tun
TESTS_SUBDIRS+=		mcast mpls ndp npf route if_vlan
.if (${MKSLJIT} != "no")
TESTS_SUBDIRS+=		bpfjit
.endif
.endif

.include <bsd.test.mk>
