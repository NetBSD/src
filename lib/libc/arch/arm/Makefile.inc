# $NetBSD: Makefile.inc,v 1.12 2012/08/05 04:54:38 matt Exp $

.include <bsd.own.mk>

SRCS+=	__aeabi_read_tp.S __sigaction14_sigtramp.c __sigtramp2.S
SRCS+=	__aeabi_uldivmod.S

CPPFLAGS += -DSOFTFLOAT
.if ${MACHINE_ARCH} == "earm" || ${MACHINE_ARCH} == "earmeb"
CPPFLAGS += -DSOFTFLOAT_NEED_FIXUNS
.endif

# for earm, use the 64-bit softfloat
.if ${MACHINE_ARCH} == "arm" || ${MACHINE_ARCH} == "armeb"
SOFTFLOAT_BITS=32
.endif

.include <softfloat/Makefile.inc>
