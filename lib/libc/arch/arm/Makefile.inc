# $NetBSD: Makefile.inc,v 1.1.2.3 2002/08/01 03:27:55 nathanw Exp $

.include <bsd.own.mk>

KMINCLUDES=
KMSRCS=arch/arm/gen/divsi3.S arch/arm/string/memmove.S \
	arch/arm/string/memcpy.S

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S

CPPFLAGS += -DSOFTFLOAT

.if ${OBJECT_FMT} != "ELF"
# arm32 a.out libc contained __fixunssfsi() and __fixunsdfsi().  Be
# compatible.
CPPFLAGS+=	-DSOFTFLOAT_NEED_FIXUNS
.endif

SOFTFLOAT_BITS=32
.include <softfloat/Makefile.inc>
