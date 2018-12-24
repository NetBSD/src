# $NetBSD: Makefile,v 1.4 2018/12/24 05:06:45 kre Exp $

NOMAN= # defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libnvmm

COPTS+=	-Wall
LDADD+=	-lnvmm

BINDIR=         ${TESTSDIR}

.if ${MACHINE} == "amd64"
# Mem Assist
TESTS_SH=		t_mem_assist
PROGS=			h_mem_assist
SRCS.h_mem_assist=	h_mem_assist.c h_mem_assist_asm.S
.endif

.include <bsd.test.mk>
