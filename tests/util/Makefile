# $NetBSD: Makefile,v 1.11 2012/03/11 22:19:53 martin Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/util

TESTS_SUBDIRS=	bzip2 config cut df grep id m4 make mtree ps sdiff sh sort \
		xlint

TESTS_SH=	t_basename
TESTS_SH+=	t_cp
TESTS_SH+=	t_dd
TESTS_SH+=	t_dirname
TESTS_SH+=	t_expr
TESTS_SH+=	t_gzip
TESTS_SH+=	t_pax
TESTS_SH+=	t_tar

.include <bsd.test.mk>
