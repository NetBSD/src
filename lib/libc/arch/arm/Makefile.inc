# $NetBSD: Makefile.inc,v 1.3 2001/03/08 18:56:19 bjh21 Exp $

.include <bsd.own.mk>

KMINCLUDES=
KMSRCS=arch/arm/gen/divsi3.S arch/arm/string/memmove.S \
	arch/arm/string/memcpy.S

CPPFLAGS += -DSOFTFLOAT

.if ${OBJECT_FMT} != "ELF"
# arm32 a.out libc contained __fixunssfsi() and __fixunsdfsi().  Be
# compatible.
CPPFLAGS+=	-DSOFTFLOAT_NEED_FIXUNS
.endif

.include <softfloat/Makefile.inc>
