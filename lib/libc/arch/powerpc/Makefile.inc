#	$NetBSD: Makefile.inc,v 1.9 2006/02/18 00:21:27 matt Exp $

KMINCLUDES=
KMSRCS=	

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

.if defined(MKSOFTFLOAT) && (${MKSOFTFLOAT} != "no")
.include <softfloat/Makefile.inc>
.endif
