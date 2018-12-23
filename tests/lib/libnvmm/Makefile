# $NetBSD: Makefile,v 1.2 2018/12/23 16:23:24 martin Exp $

NOMAN= # defined

.include <bsd.own.mk>
.if ${MACHINE} == "amd64"

TESTSDIR=	${TESTSBASE}/lib/libnvmm

COPTS+=	-Wall
LDADD+=	-lnvmm

# Mem Assist
TESTS_SH=		t_mem_assist
PROGS=			h_mem_assist
SRCS.h_mem_assist=	h_mem_assist.c h_mem_assist_asm.S
.endif

.include <bsd.test.mk>
