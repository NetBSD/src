# $NetBSD: Makefile.inc,v 1.6 2003/10/05 19:48:39 matt Exp $

.include <bsd.own.mk>

KMINCLUDES=
KMSRCS=arch/arm/gen/divsi3.S arch/arm/string/memmove.S \
	arch/arm/string/memcpy.S

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S __sigtramp2.S

CPPFLAGS += -DSOFTFLOAT

.if ${OBJECT_FMT} != "ELF"
# arm32 a.out libc contained __fixunssfsi() and __fixunsdfsi().  Be
# compatible.
CPPFLAGS+=	-DSOFTFLOAT_NEED_FIXUNS
.endif

SOFTFLOAT_BITS=32
.include <softfloat/Makefile.inc>
