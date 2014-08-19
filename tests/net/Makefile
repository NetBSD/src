# $NetBSD: Makefile,v 1.12.2.2 2014/08/20 00:04:51 tls Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass net route sys
.if (${MKRUMP} != "no")
TESTS_SUBDIRS+=		bpf bpfilter carp icmp if if_loop mpls npf
.if (${MKSLJIT} != "no")
TESTS_SUBDIRS+=		bpfjit
.endif
.endif

.include <bsd.test.mk>
