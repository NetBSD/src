# $NetBSD: Makefile,v 1.4 2014/02/11 16:11:28 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-config

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-config

CPPFLAGS+=	-DHAVE_CONFIG_H

TESTS_SH=	integration_test

.include <bsd.test.mk>
