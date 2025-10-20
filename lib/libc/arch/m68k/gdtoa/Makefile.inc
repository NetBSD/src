#	$NetBSD: Makefile.inc,v 1.6 2025/10/20 14:31:39 nat Exp $

SRCS+=	strtof.c

.if ${MACHINE_ARCH:S/m68ksf/m68k/} == "m68k"
SRCS+=	strtold_pxL.c
SRCS+=	strtopxL.c
.endif
