# $NetBSD: Makefile,v 1.1.2.2 2009/05/13 19:19:23 jym Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/ipf

TESTS_SH=	t_ipf

CLEANFILES+= t_ipf.sh
t_ipf.sh: t_ipf.awk t_ipf.in
	${TOOL_AWK} -f ${.CURDIR}/t_ipf.awk ${.CURDIR}/t_ipf.in >t_ipf.sh

SUBDIR=		regress input expected

.include <bsd.test.mk>
.include <bsd.subdir.mk>
