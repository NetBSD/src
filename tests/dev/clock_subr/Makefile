# $NetBSD: Makefile,v 1.1.2.2 2016/09/14 03:04:19 pgoyette Exp $

NOMAN=		# defined

CPPFLAGS+= -I${.CURDIR}/../../../sys

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/clock_subr

.PATH:			${NETBSDSRCDIR}/sys/dev
TESTS_C+=		t_clock_subr
SRCS.t_clock_subr=	t_clock_subr.c clock_subr.c

.include <bsd.test.mk>
