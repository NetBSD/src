#	$NetBSD: Makefile.inc,v 1.5 2002/02/20 22:45:04 eeh Exp $

KMINCLUDES=
KMSRCS=	

.if (${MKSOFTFLOAT} != "no")
CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS
.include <softfloat/Makefile.inc>
.endif
