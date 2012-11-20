# $NetBSD: Makefile,v 1.12.2.1 2012/11/20 03:02:56 tls Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass route sys
.if (${MKRUMP} != "no")
TESTS_SUBDIRS+=		bpf bpfilter carp icmp if if_loop net npf
.endif

.include <bsd.test.mk>
