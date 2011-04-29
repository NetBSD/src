#	$NetBSD: Makefile.inc,v 1.1.34.1 2011/04/29 07:48:33 matt Exp $

SRCS+=	strtof.c
.if (${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el")
.if !defined(CPUFLAGS) || empty(CPUFLAGS:M-mabi=32)
SRCS+=	strtold_pQ.c
SRCS+=	strtopQ.c
.endif
.endif
