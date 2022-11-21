# $NetBSD: Makefile,v 1.1 2022/11/21 22:01:33 christos Exp $

NOMAN=			# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/lua/libm
TESTS_SH=	t_lualibm 

BINDIR=		${TESTSDIR}
PROGS+=		h_lualibm 
DPADD+=		${LIBM}
LDADD+=		-lm

FILESDIR=	${TESTSDIR}
FILESMODE=	0555
FILES+=		lualibm.lua


.include <bsd.test.mk>
