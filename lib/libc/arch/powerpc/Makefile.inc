#	$NetBSD: Makefile.inc,v 1.6 2002/07/10 04:29:08 thorpej Exp $

KMINCLUDES=
KMSRCS=	

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S

.if (${MKSOFTFLOAT} != "no")
CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS
.include <softfloat/Makefile.inc>
.endif
