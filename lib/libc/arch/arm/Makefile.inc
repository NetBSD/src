# $NetBSD: Makefile.inc,v 1.1 2000/12/29 20:13:44 bjh21 Exp $

KMINCLUDES=
KMSRCS=arch/arm/gen/divsi3.S arch/arm/string/memmove.S \
	arch/arm/string/memcpy.S

.if ${MACHINE_ARCH} == "arm26"
# __APCS_26__ required for SYS.h stuff, which uses ${CPP} and hence doesn't
# get the normal predefines.
CPPFLAGS+=-DSOFTFLOAT -D__APCS_26__

.include <softfloat/Makefile.inc>
.endif
