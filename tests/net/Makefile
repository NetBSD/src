# $NetBSD: Makefile,v 1.38 2021/05/17 04:07:44 yamaguchi Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass in_cksum net sys
.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE)
TESTS_SUBDIRS+=		arp bpf bpfilter can carp icmp if if_bridge if_gif
TESTS_SUBDIRS+=		if_ipsec if_l2tp if_lagg if_loop if_pppoe if_tap
TESTS_SUBDIRS+=		if_tun if_vether if_vlan if_wg ipsec mcast mpls
TESTS_SUBDIRS+=		ndp npf route
.if (${MKSLJIT} != "no")
TESTS_SUBDIRS+=		bpfjit
.endif
.endif

.include <bsd.test.mk>
