# $NetBSD: Makefile.inc,v 1.26 2014/01/17 22:19:46 joerg Exp $

.include <bsd.own.mk>

SRCS+=	__aeabi_read_tp.S __sigaction14_sigtramp.c __sigtramp2.S

.if empty(LIBC_MACHINE_ARCH:Mearmv7*)
AFLAGS+= -marm
.endif

CPPFLAGS += -I.

.if !empty(LIBC_MACHINE_ARCH:Mearm*)
SRCS+=	__aeabi_ldivmod.S __aeabi_uldivmod.S
SRCS+=	__aeabi_lcmp.c __aeabi_ulcmp.c
SRCS+=	fixunsgen_ieee754.c fixunssfsi_ieee754.c
SRCS+=	fixunsgen64_ieee754.c fixunsdfsi_ieee754.c
SRCS+=	arm_initfini.c
.endif

.if ${MKSOFTFLOAT} != "no"
CPPFLAGS += -DSOFTFLOAT

# for earm, use the 64-bit softfloat
.if ${LIBC_MACHINE_ARCH} == "arm" || ${LIBC_MACHINE_ARCH} == "armeb"
SOFTFLOAT_BITS=32
#SRCS+=	floatunsidf_ieee754.c floatunsisf_ieee754.c
.endif

.include <softfloat/Makefile.inc>
.else
.PATH : ${ARCHDIR}/hardfloat
SRCS.hardfloat+=	fpgetround.c fpgetsticky.S fpgetmask.S fabs_ieee754.S
SRCS.hardfloat+=	fpsetround.c fpsetmask.S fpsetsticky.S
.for f in ${SRCS.hardfloat}
CPPFLAGS.${f} += -mfpu=vfp
.endfor
SRCS+=	${SRCS.hardfloat}
.endif
