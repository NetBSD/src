#	$NetBSD: Makefile.inc,v 1.3 1999/09/29 13:19:15 takemura Exp $

KMINCLUDES=
KMSRCS=

.if  (${MACHINE} == "hpcmips")
CPPFLAGS+=-DSOFTFLOAT

.include <fplib/Makefile.inc>
.endif
