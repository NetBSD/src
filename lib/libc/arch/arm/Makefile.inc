# $NetBSD: Makefile.inc,v 1.2 2001/03/07 00:49:48 bjh21 Exp $

KMINCLUDES=
KMSRCS=arch/arm/gen/divsi3.S arch/arm/string/memmove.S \
	arch/arm/string/memcpy.S

CPPFLAGS += -DSOFTFLOAT

.include <softfloat/Makefile.inc>
