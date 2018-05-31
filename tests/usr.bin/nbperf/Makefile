# $NetBSD: Makefile,v 1.2 2018/05/31 09:08:26 mrg Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/nbperf

TESTS_SH=	t_nbperf

SCRIPTSDIR=	${TESTSDIR}
SCRIPTS+=	h_nbperf
CLEANFILES+=	h_nbperf

FILESDIR=		${TESTSDIR}
FILES=			hash_driver.c

.include <bsd.test.mk>
