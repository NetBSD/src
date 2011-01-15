#	$NetBSD: Makefile.inc,v 1.11 2011/01/15 07:31:12 matt Exp $

SRCS+=		__sigaction14_sigtramp.c __sigtramp2.S
CPPFLAGS+=	-D_NOREGNAMES

.if defined(MKSOFTFLOAT) && (${MKSOFTFLOAT} != "no")
.include <softfloat/Makefile.inc>
.endif
