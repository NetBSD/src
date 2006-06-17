# $NetBSD: Makefile.inc,v 1.8 2006/06/17 18:04:23 uwe Exp $

.include <bsd.own.mk>

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

CPPFLAGS += -DSOFTFLOAT

.if ${OBJECT_FMT} != "ELF"
# arm32 a.out libc contained __fixunssfsi() and __fixunsdfsi().  Be
# compatible.
CPPFLAGS+=	-DSOFTFLOAT_NEED_FIXUNS
.endif

SOFTFLOAT_BITS=32
.include <softfloat/Makefile.inc>
