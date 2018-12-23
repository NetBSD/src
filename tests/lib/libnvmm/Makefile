# $NetBSD: Makefile,v 1.3 2018/12/23 16:40:16 jakllsch Exp $

NOMAN= # defined

.include <bsd.own.mk>
.if ${MACHINE} == "amd64"

TESTSDIR=	${TESTSBASE}/lib/libnvmm

COPTS+=	-Wall
LDADD+=	-lnvmm

BINDIR=         ${TESTSDIR}

# Mem Assist
TESTS_SH=		t_mem_assist
PROGS=			h_mem_assist
SRCS.h_mem_assist=	h_mem_assist.c h_mem_assist_asm.S
.endif

.include <bsd.test.mk>
