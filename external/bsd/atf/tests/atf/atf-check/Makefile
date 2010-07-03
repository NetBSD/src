# $NetBSD: Makefile,v 1.2 2010/07/03 08:14:12 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-check

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-check

CPPFLAGS+=	-DHAVE_CONFIG_H

TESTS_SH=	integration_test

.include <bsd.test.mk>
