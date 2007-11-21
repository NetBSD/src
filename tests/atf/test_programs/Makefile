# $NetBSD: Makefile,v 1.2 2007/11/21 15:39:34 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/test_programs

SRCDIR=		${NETBSDSRCDIR}/dist/atf
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
