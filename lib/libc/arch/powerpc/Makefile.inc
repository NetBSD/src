#	$NetBSD: Makefile.inc,v 1.2.10.4 2002/03/22 20:41:53 nathanw Exp $

KMINCLUDES=
KMSRCS=	

.if (${MKSOFTFLOAT} != "no")
CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS
.include <softfloat/Makefile.inc>
.endif
