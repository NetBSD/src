# $NetBSD: Makefile,v 1.1 2016/08/14 14:55:42 jakllsch Exp $

NOMAN=		# defined

CPPFLAGS+= -I${.CURDIR}/../../../sys

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/clock_subr

.PATH:			${NETBSDSRCDIR}/sys/dev
TESTS_C+=		t_clock_subr
SRCS.t_clock_subr=	t_clock_subr.c clock_subr.c

.include <bsd.test.mk>
