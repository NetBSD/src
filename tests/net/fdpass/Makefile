# $NetBSD: Makefile,v 1.1 2012/08/13 11:15:05 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/fdpass

TESTS_SH=	t_fdpass

PROGS=          fdpass64 fdpass32
SRCS.fdpass64=   fdpass.c
MAN.fdpass64=    # empty
BINDIR.fdpass64= ${TESTSDIR}

.if ${MACHINE} == "sparc64" || ${MACHINE} == "amd64"
CFLAGS.fdpass32= -m32
.endif
SRCS.fdpass32=   fdpass.c
MAN.fdpass32=    # empty
BINDIR.fdpass32= ${TESTSDIR}

.include <bsd.test.mk>
