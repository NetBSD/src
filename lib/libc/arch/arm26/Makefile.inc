# $NetBSD: Makefile.inc,v 1.1 2000/05/09 21:55:45 bjh21 Exp $

KMINCLUDES=
KMSRCS=arch/arm26/gen/divsi3.S arch/arm26/string/memmove.S \
	arch/arm26/string/memcpy.S
# __APCS_26__ required for SYS.h stuff
CPPFLAGS+=-DSOFTFLOAT -D__APCS_26__

.include <fplib/Makefile.inc>
