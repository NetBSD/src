#	$NetBSD: Makefile.inc,v 1.2.10.2 2002/01/28 20:50:03 nathanw Exp $

KMINCLUDES=
KMSRCS=	arch/powerpc/string/ffs.S

.if (${MKSOFTFLOAT} != "no")
CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS
.include <softfloat/Makefile.inc>
.endif
