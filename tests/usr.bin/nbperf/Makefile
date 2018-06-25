# $NetBSD: Makefile,v 1.1.32.1 2018/06/25 07:26:09 pgoyette Exp $

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
