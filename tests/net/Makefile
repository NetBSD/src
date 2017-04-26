# $NetBSD: Makefile,v 1.28.2.3 2017/04/26 02:53:33 pgoyette Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass in_cksum net sys
.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE)
TESTS_SUBDIRS+=		arp bpf bpfilter carp icmp if if_bridge if_gif if_l2tp
TESTS_SUBDIRS+=		 if_loop if_pppoe if_tap if_tun ipsec mcast mpls ndp npf
TESTS_SUBDIRS+=		 route if_vlan
.if (${MKSLJIT} != "no")
TESTS_SUBDIRS+=		bpfjit
.endif
.endif

.include <bsd.test.mk>
