# $NetBSD: Makefile.inc,v 1.18 2013/01/26 07:09:41 matt Exp $

.include <bsd.own.mk>

SRCS+=	__aeabi_read_tp.S __sigaction14_sigtramp.c __sigtramp2.S

CPPFLAGS += -I.

.if ${MACHINE_ARCH} == "earm" || ${MACHINE_ARCH} == "earmeb"
SRCS+=	__aeabi_ldivmod.S __aeabi_uldivmod.S
SRCS+=	__aeabi_lcmp.c __aeabi_ulcmp.c
SRCS+=	fixunsgen_ieee754.c fixunssfsi_ieee754.c
SRCS+=	fixunsgen64_ieee754.c fixunsdfsi_ieee754.c
.endif

.if ${MKSOFTFLOAT} != "no"
CPPFLAGS += -DSOFTFLOAT

# for earm, use the 64-bit softfloat
.if ${MACHINE_ARCH} == "arm" || ${MACHINE_ARCH} == "armeb"
SOFTFLOAT_BITS=32
.endif

.include <softfloat/Makefile.inc>
.endif
