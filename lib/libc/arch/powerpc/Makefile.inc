#	$NetBSD: Makefile.inc,v 1.2.10.1 2001/10/08 20:17:52 nathanw Exp $

KMINCLUDES=
KMSRCS=

.if (${MKSOFTFLOAT} != "no")
CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS
.include <softfloat/Makefile.inc>
.endif
