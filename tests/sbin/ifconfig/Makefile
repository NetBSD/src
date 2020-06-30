# $NetBSD: Makefile,v 1.9 2020/06/30 11:48:20 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sbin/ifconfig

TESTS_SH=	t_bridge \
		t_capabilities \
		t_nonexistent \
		t_random_garbage \
		t_repeated_link_addr \
		t_repeated_mtu \
		t_repeated_scan \
		t_repeated_updown \
		t_tap \
		t_woptions

.include <bsd.test.mk>
