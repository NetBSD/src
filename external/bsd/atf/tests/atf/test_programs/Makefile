# $NetBSD: Makefile,v 1.1 2009/01/19 07:13:06 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/test_programs

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tests/atf/test_programs

CPPFLAGS+=	-I${SRCDIR}/libs

TESTS_CXX=	h_cpp
TESTS_SH=	h_sh
TESTS_SH+=	t_cleanup
TESTS_SH+=	t_config
TESTS_SH+=	t_env
TESTS_SH+=	t_fork
TESTS_SH+=	t_meta_data
TESTS_SH+=	t_srcdir
TESTS_SH+=	t_status
TESTS_SH+=	t_workdir

.include <bsd.test.mk>
