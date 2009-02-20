# $NetBSD: Makefile,v 1.1 2009/02/20 21:42:59 jmmv Exp $

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
