#	$NetBSD: Makefile.inc,v 1.3.50.1 2014/08/20 00:02:10 tls Exp $

SRCS+=	strtof.c

.if ${MACHINE_ARCH} == "m68k"
SRCS+=	strtold_pxL.c
SRCS+=	strtopxL.c
.endif
