# $NetBSD: Makefile,v 1.2 2014/06/17 06:36:01 alnsn Exp $

TESTSDIR=	${TESTSBASE}/lib/libsljit

TESTS_SH=	t_sljit

.include <bsd.own.mk>
.include <../../../external/bsd/sljit/Makefile.inc>

.PATH:		${SLJITDIST}/test_src

BINDIR=		${TESTSDIR}

PROG=		h_sljit
MAN=		# defined
SRCS=		sljitMain.c sljitTest.c

WARNS=		3

LDADD+=		-L${LIBSLJITDIR} -lsljit
DPADD+=		${LIBSLJITDIR}/libsljit.a

# I-cache sync routines for arm and mips
LDADD+=		-l${MACHINE_CPU}

.include <bsd.test.mk>
