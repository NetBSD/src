#	$NetBSD: Makefile.inc,v 1.8 2004/12/09 00:39:41 simonb Exp $

KMINCLUDES=
KMSRCS=	

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S __sigtramp2.S

.if defined(MKSOFTFLOAT) && (${MKSOFTFLOAT} != "no")
.include <softfloat/Makefile.inc>
.endif
