#	$NetBSD: Makefile.inc,v 1.2.8.1 1999/12/27 18:29:35 wrstuden Exp $

KMINCLUDES=
KMSRCS=

.if  (${MACHINE} == "hpcmips")
CPPFLAGS+=-DSOFTFLOAT

.include <fplib/Makefile.inc>
.endif
