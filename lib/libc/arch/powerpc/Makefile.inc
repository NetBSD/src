#	$NetBSD: Makefile.inc,v 1.2.10.3 2002/03/08 21:34:49 nathanw Exp $

KMINCLUDES=
KMSRCS=	

.if (${MKSOFTFLOAT} != "no")
CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS
.include <softfloat/Makefile.inc>
.endif
