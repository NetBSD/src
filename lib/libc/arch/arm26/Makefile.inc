# $NetBSD: Makefile.inc,v 1.2 2000/06/06 08:14:49 bjh21 Exp $

KMINCLUDES=
KMSRCS=arch/arm26/gen/divsi3.S arch/arm26/string/memmove.S \
	arch/arm26/string/memcpy.S
# __APCS_26__ required for SYS.h stuff
CPPFLAGS+=-DSOFTFLOAT -D__APCS_26__

.include <softfloat/Makefile.inc>
