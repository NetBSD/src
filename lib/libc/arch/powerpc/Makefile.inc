#	$NetBSD: Makefile.inc,v 1.2.10.5 2002/08/01 03:27:59 nathanw Exp $

KMINCLUDES=
KMSRCS=	

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S

.if (${MKSOFTFLOAT} != "no")
CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS
.include <softfloat/Makefile.inc>
.endif
