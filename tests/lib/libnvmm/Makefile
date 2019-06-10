# $NetBSD: Makefile,v 1.5.2.2 2019/06/10 22:10:07 christos Exp $

NOMAN= # defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libnvmm

COPTS+=	-Wall
LDADD+=	-lnvmm

BINDIR=         ${TESTSDIR}

.if ${MACHINE} == "amd64"
# I/O Assist
TESTS_SH=		t_io_assist
PROGS=			h_io_assist
SRCS.h_io_assist=	h_io_assist.c h_io_assist_asm.S

# Mem Assist
TESTS_SH+=		t_mem_assist
PROGS+=			h_mem_assist
SRCS.h_mem_assist=	h_mem_assist.c h_mem_assist_asm.S
.endif

.include <bsd.test.mk>
