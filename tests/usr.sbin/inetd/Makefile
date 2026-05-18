# $NetBSD: Makefile,v 1.2.6.1 2026/05/18 16:58:49 martin Exp $

WARNS=6
.include <bsd.own.mk>

TESTSDIR=${TESTSBASE}/usr.sbin/inetd

TESTS_C += t_inetd
TESTS_SH += t_accept_max

#inetd service, supports dgram and stream via args
MKMAN = no
PROGS += test_server
BINDIR=${TESTSDIR}

#Other files that should be copied to /usr/tests
FILESDIR=${TESTSDIR}
FILES=test_server inetd_ratelimit.conf

.include <bsd.test.mk>
