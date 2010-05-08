# $NetBSD: Makefile,v 1.1 2010/05/08 08:12:34 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-compile

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tests/atf/atf-compile

TESTS_SH=	t_integration

BINDIR=		${TESTSDIR}
PROGS_CXX=	h_mode
MAN.h_mode=	# empty

.include <bsd.test.mk>
