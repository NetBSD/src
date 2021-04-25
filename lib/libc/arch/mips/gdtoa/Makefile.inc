#	$NetBSD: Makefile.inc,v 1.5 2021/04/25 23:45:58 christos Exp $

SRCS+=	strtof.c
.if ${MACHINE_MIPS64}
.if !defined(CPUFLAGS) || empty(CPUFLAGS:M-mabi=32)
SRCS+=	strtold_pQ.c
SRCS+=	strtopQ.c
.endif
.endif
