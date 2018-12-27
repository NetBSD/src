# $NetBSD: Makefile,v 1.7 2018/12/27 19:33:52 christos Exp $

NOMAN=			# defined

.include <bsd.own.mk>

TESTSDIR=		${TESTSBASE}/lib/csu
TESTS_SH=		t_crt0 t_ifunc_static
TESTS_SUBDIRS=
SUBDIR+=		dso

.include "Makefile.check_stack"

CPPFLAGS+=		${CPPFLAGS_CHECK_STACK}

BINDIR=			${TESTSDIR}
PROG_CXX=		h_initfini1 h_initfini2 h_initfini3
SRCS.h_initfini1=	h_initfini1.cxx h_initfini_common.cxx \
			${SRCS_CHECK_STACK}
SRCS.h_initfini2=	h_initfini1.cxx h_initfini_common.cxx \
			${SRCS_CHECK_STACK}
LDSTATIC.h_initfini2+=	-static
SRCS.h_initfini3=	h_initfini3.cxx h_initfini_common.cxx \
			${SRCS_CHECK_STACK}
LDADD.h_initfini3+=	-Wl,-rpath,${TESTSDIR}

PROGS+=			h_ifunc_static h_initfini_array
SRCS.h_ifunc_static=	h_ifunc_static.c
SRCS.h_initfini_array=	h_initfini_array.c
LDSTATIC.h_ifunc_static=-static

.include <bsd.test.mk>
