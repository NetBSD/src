# $NetBSD: Makefile,v 1.9.4.1 2012/10/30 19:00:05 yamt Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass route sys
.if (${MKRUMP} != "no")
TESTS_SUBDIRS+=		bpf bpfilter carp icmp if if_loop net npf
.endif

.include <bsd.test.mk>
