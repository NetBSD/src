# $NetBSD: Makefile,v 1.2 2021/08/11 20:42:26 rillig Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/mkdep

TESTS_SH=	t_findcc
TESTS_SH+=	t_mkdep

BINDIR=		${TESTSDIR}
PROG=		h_findcc
.PATH:  	${NETBSDSRCDIR}/usr.bin/mkdep
SRCS=		h_findcc.c findcc.c
CPPFLAGS+=	-I${NETBSDSRCDIR}/usr.bin/mkdep
MAN.h_findcc=	# none

.include <bsd.test.mk>
