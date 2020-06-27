# $NetBSD: Makefile,v 1.8 2020/06/27 13:53:43 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sbin/ifconfig

TESTS_SH=	t_bridge \
		t_capabilities \
		t_nonexistent \
		t_random_garbage \
		t_repeated_scan \
		t_repeated_updown \
		t_tap \
		t_woptions

.include <bsd.test.mk>
