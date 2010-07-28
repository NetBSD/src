# $NetBSD: Makefile,v 1.2 2010/07/28 13:51:38 joerg Exp $

NOMAN=			# defined

.include <bsd.own.mk>

TESTSDIR=		${TESTSBASE}/lib/csu
TESTS_SH=		t_crt0
TESTS_SUBDIRS=
SUBDIR+=		dso

BINDIR=			${TESTSDIR}
PROG_CXX=		h_initfini1 h_initfini2 h_initfini3
SRCS.h_initfini1=	h_initfini1.cxx h_initfini_common.cxx
SRCS.h_initfini2=	h_initfini1.cxx h_initfini_common.cxx
LDADD.h_initfini2+=	-static
SRCS.h_initfini3=	h_initfini3.cxx h_initfini_common.cxx
LDADD.h_initfini3+=	-Wl,-rpath,${TESTSDIR}

.include <bsd.test.mk>
