#	$NetBSD: Makefile.inc,v 1.4 2001/11/25 01:09:58 mjl Exp $

KMINCLUDES=
KMSRCS=	arch/powerpc/string/ffs.S

.if (${MKSOFTFLOAT} != "no")
CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS
.include <softfloat/Makefile.inc>
.endif
