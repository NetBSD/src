#	$NetBSD: Makefile.inc,v 1.2 2006/03/20 09:52:23 kleink Exp $

SRCS+=	strtof.c

.if ${MACHINE_ARCH} != "m68000"
SRCS+=	strtold_pxL.c
SRCS+=	strtopxL.c
.fi
