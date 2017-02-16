# $NetBSD: Makefile,v 1.31 2017/02/16 08:44:47 knakahara Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass in_cksum net sys
.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE)
TESTS_SUBDIRS+=		arp bpf bpfilter carp icmp if if_bridge if_gif if_l2tp
TESTS_SUBDIRS+=		 if_loop if_pppoe if_tap if_tun mcast mpls ndp npf route
TESTS_SUBDIRS+=		 if_vlan
.if (${MKSLJIT} != "no")
TESTS_SUBDIRS+=		bpfjit
.endif
.endif

.include <bsd.test.mk>
