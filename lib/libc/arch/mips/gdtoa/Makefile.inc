#	$NetBSD: Makefile.inc,v 1.2 2011/01/17 23:53:03 matt Exp $

CPUFLAGS?=
SRCS+=	strtof.c
.if (${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el")
.if ${CPUFLAGS:M-mabi=32} == ""
SRCS+=	strtold_pQ.c
SRCS+=	strtopQ.c
.endif
.endif
