# $NetBSD: Makefile.inc,v 1.11 2012/08/05 04:30:46 matt Exp $

.include <bsd.own.mk>

SRCS+=	__aeabi_read_tp.S __sigaction14_sigtramp.c __sigtramp2.S
SRCS+=	__aeabi_uldivmod.S

CPPFLAGS += -DSOFTFLOAT

# for earm, use the 64-bit softfloat
.if ${MACHINE_ARCH} == "arm" || ${MACHINE_ARCH} == "armeb"
SOFTFLOAT_BITS=32
.endif

.include <softfloat/Makefile.inc>
