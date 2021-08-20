# $NetBSD: Makefile,v 1.3 2021/08/20 06:36:10 rillig Exp $

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
WARNS=		6

.include <bsd.test.mk>
