# $NetBSD: Makefile,v 1.13 2012/09/12 14:06:31 martin Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass route sys
.if (${MKRUMP} != "no")
TESTS_SUBDIRS+=		bpf bpfilter carp icmp if if_loop net npf
.endif

.include <bsd.test.mk>
