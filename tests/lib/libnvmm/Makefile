# $NetBSD: Makefile,v 1.1 2018/12/23 13:35:02 maxv Exp $

NOMAN= # defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libnvmm

COPTS+=	-Wall
LDADD+=	-lnvmm

.if ${MACHINE} == "amd64"
# Mem Assist
TESTS_SH=		t_mem_assist
PROGS=			h_mem_assist
SRCS.h_mem_assist=	h_mem_assist.c h_mem_assist_asm.S
.endif

.include <bsd.test.mk>
