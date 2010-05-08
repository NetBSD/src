# $NetBSD: Makefile,v 1.4 2010/05/08 08:12:34 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/test_programs

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tests/atf/test_programs

CPPFLAGS+=	-I${SRCDIR}/libs

TESTS_C=	h_c

TESTS_CXX=	h_cpp

TESTS_SH=	h_sh
.for t in t_config t_fork t_meta_data t_srcdir t_status 
TESTS_SH+=		${t}
TESTS_SH_SRC_${t}=	common.sh ${t}.sh
.endfor

.include <bsd.test.mk>
