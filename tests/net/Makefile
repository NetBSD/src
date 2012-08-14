# $NetBSD: Makefile,v 1.12 2012/08/14 19:13:54 alnsn Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass route sys
.if (${MKRUMP} != "no")
TESTS_SUBDIRS+=		bpf bpfilter carp icmp if if_loop net
.endif

.include <bsd.test.mk>
