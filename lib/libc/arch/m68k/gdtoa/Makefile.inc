#	$NetBSD: Makefile.inc,v 1.5.38.1 2026/11/04 23:19:12 perseant Exp $

SRCS+=	strtof.c

.if ${MACHINE_ARCH:S/m68ksf/m68k/} == "m68k"
SRCS+=	strtold_pxL.c
SRCS+=	strtopxL.c
.endif
