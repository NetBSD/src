# $NetBSD: Makefile,v 1.18 2011/01/03 19:01:47 pgoyette Exp $

.include <bsd.own.mk>
.include <bsd.sys.mk>

TESTS_SUBDIRS+=	gen hash locale setjmp stdlib stdio string

.if ${MACHINE_ARCH} != "vax" && ${MACHINE_ARCH} != "m68000"
TESTS_SUBDIRS+=	ieeefp
.endif

.if ${HAS_SSP} == "yes"
TESTS_SUBDIRS+=	ssp
.endif

TESTSDIR=	${TESTSBASE}/lib/libc

TESTS_C+=	t_clone
TESTS_C+=	t_context
TESTS_C+=	t_convfp
TESTS_C+=	t_gdtoa
TESTS_C+=	t_hsearch
TESTS_C+=	t_inet
TESTS_C+=	t_ptm
TESTS_C+=	t_randomid
TESTS_C+=	t_strptime

TESTS_SH+=	t_atexit
TESTS_SH+=	t_protoent
TESTS_SH+=	t_servent

BINDIR=		${TESTSDIR}
MKMAN=		no

PROGS+=		h_atexit
PROGS+=		h_protoent
PROGS+=		h_servent

.include <bsd.test.mk>
