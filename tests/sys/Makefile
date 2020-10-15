# $NetBSD: Makefile,v 1.5 2020/10/15 17:44:44 mgorny Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sys

TESTS_SUBDIRS+=	crypto
TESTS_SUBDIRS+=	net
TESTS_SUBDIRS+=	netatalk
TESTS_SUBDIRS+=	netinet
TESTS_SUBDIRS+=	netinet6
TESTS_SUBDIRS+=	rc
.if ${MACHINE} == amd64 || ${MACHINE} == i386
TESTS_SUBDIRS+=	x86
.endif

.include <bsd.test.mk>
