# $NetBSD: Makefile.inc,v 1.1.2.2 2002/06/21 18:17:58 nathanw Exp $

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

SOFTFLOAT_BITS=32
.include <softfloat/Makefile.inc>
