# $NetBSD: Makefile,v 1.2 2009/01/20 10:29:59 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/test_programs

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tests/atf/test_programs

CPPFLAGS+=	-I${SRCDIR}/libs

TESTS_CXX=	h_cpp

TESTS_SH=	h_sh
.for t in t_cleanup t_config t_env t_fork t_meta_data t_srcdir \
          t_status t_workdir
TESTS_SH+=		${t}
TESTS_SH_SRC_${t}=	common.sh ${t}.sh
.endfor

.include <bsd.test.mk>
