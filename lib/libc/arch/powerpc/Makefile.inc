#	$NetBSD: Makefile.inc,v 1.3 2001/06/18 17:05:32 simonb Exp $

KMINCLUDES=
KMSRCS=

.if (${MKSOFTFLOAT} != "no")
CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS
.include <softfloat/Makefile.inc>
.endif
