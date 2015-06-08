#	$NetBSD: Makefile.inc,v 1.5 2015/06/08 04:08:30 matt Exp $

.if (${MACHINE_ARCH} == "x86_64" && ${MLIBDIR:Unone} != "i386")
SRCS+=	x86_64_mtrr.c x86_64_iopl.c
.endif

MAN+=	x86_64_get_mtrr.2 x86_64_iopl.2

MLINKS+=x86_64_get_mtrr.2 x86_64_set_mtrr.2
