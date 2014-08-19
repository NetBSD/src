# $NetBSD: Makefile,v 1.2.12.2 2014/08/19 23:45:56 tls Exp $

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/atf/test-programs
ATFFILE=	yes
KYUAFILE=	yes

.PATH:		${SRCDIR}/test-programs

TESTS_C=	c_helpers

TESTS_CXX=	cpp_helpers

TESTS_SH=	sh_helpers
.for t in config_test expect_test meta_data_test result_test srcdir_test
TESTS_SH+=		${t}
TESTS_SH_SRC_${t}=	common.sh ${t}.sh
.endfor

.include <bsd.test.mk>
