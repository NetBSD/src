# $NetBSD: Makefile,v 1.27 2011/01/10 04:57:56 christos Exp $

.include <bsd.own.mk>
.include <bsd.sys.mk>

TESTS_SUBDIRS+=	db gen hash ieeefp regex rpc setjmp stdlib stdio string sys
TESTS_SUBDIRS+=	ttyio

.if ${HAS_SSP} == "yes"
TESTS_SUBDIRS+=	ssp
.endif

TESTSDIR=	${TESTSBASE}/lib/libc

TESTS_C+=	t_cerror
TESTS_C+=	t_clone
TESTS_C+=	t_context
TESTS_C+=	t_convfp
TESTS_C+=	t_gdtoa
TESTS_C+=	t_hsearch
TESTS_C+=	t_inet
TESTS_C+=	t_mktime
TESTS_C+=	t_ptm
TESTS_C+=	t_randomid
TESTS_C+=	t_strptime

TESTS_SH+=	t_atexit
TESTS_SH+=	t_nsdispatch
TESTS_SH+=	t_protoent
TESTS_SH+=	t_servent

BINDIR=		${TESTSDIR}
MKMAN=		no

PROGS+=		h_atexit
PROGS+=		h_nsd_recurse
PROGS+=		h_protoent
PROGS+=		h_servent

LDADD.h_nsd_recurse+=	-lpthread
LDADD.t_mktime+=	-lpthread

.include <bsd.test.mk>
