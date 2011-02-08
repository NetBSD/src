#	$NetBSD: Makefile.inc,v 1.3.2.1 2011/02/08 16:18:57 bouyer Exp $

SRCS+=	strtof.c
.if (${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el")
.if !defined(CPUFLAGS) || empty(CPUFLAGS:M-mabi=32)
SRCS+=	strtold_pQ.c
SRCS+=	strtopQ.c
.endif
.endif
