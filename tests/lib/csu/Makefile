# $NetBSD: Makefile,v 1.9 2025/04/27 14:04:19 riastradh Exp $

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

TESTS_SH+=		t_hello

PROGS+=		${"${MKPIC}" != no:?h_hello_dyn:}
PROGS+=		${"${MKPIE}" != no:?h_hello_dynpie:}
PROGS+=		h_hello_sta
PROGS+=		${"${MKPIE}" != no:?h_hello_stapie:}

SRCS.h_hello_dyn+=		h_hello.c
SRCS.h_hello_dynpie+=		h_hello.c
SRCS.h_hello_sta+=		h_hello.c
SRCS.h_hello_stapie+=		h_hello.c
COPTS.h_hello.c+=		${"${MKPIE}" != no:?-fPIE:}
LDFLAGS.h_hello_dyn=		-no-pie
LDFLAGS.h_hello_dynpie=		-pie
LDFLAGS.h_hello_sta=		-no-pie
LDFLAGS.h_hello_stapie=		-pie
LDSTATIC.h_hello_sta=		-static
LDSTATIC.h_hello_stapie=	-static

.include <bsd.test.mk>
