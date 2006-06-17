#	$NetBSD: Makefile.inc,v 1.10 2006/06/17 18:04:23 uwe Exp $

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

.if defined(MKSOFTFLOAT) && (${MKSOFTFLOAT} != "no")
.include <softfloat/Makefile.inc>
.endif
