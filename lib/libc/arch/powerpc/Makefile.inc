#	$NetBSD: Makefile.inc,v 1.7 2003/09/25 22:46:22 matt Exp $

KMINCLUDES=
KMSRCS=	

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S __sigtramp2.S

.if (${MKSOFTFLOAT} != "no")
CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS
.include <softfloat/Makefile.inc>
.endif
