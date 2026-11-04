# $NetBSD: Makefile,v 1.2.4.1 2026/11/04 23:28:28 perseant Exp $

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
