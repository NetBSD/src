#	$NetBSD: Makefile.inc,v 1.3 2019/04/16 07:32:30 mrg Exp $

SRCS+=	strtof.c
SRCS+=	strtopQ.c

.if ${MACHINE_ARCH} == "riscv64"
SRCS+=	strtold_pQ.c
.endif
