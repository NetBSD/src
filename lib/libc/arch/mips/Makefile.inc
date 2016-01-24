#	$NetBSD: Makefile.inc,v 1.14 2016/01/24 15:55:34 christos Exp $

.include <bsd.own.mk>

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

CPPFLAGS+= -I.
CPPFLAGS.assym.h+=-D__LIBC12_SOURCE__ -D_KERNTYPES

.if ${MKSOFTFLOAT} != "no"
.include <softfloat/Makefile.inc>
CPPFLAGS+= -DSOFTFLOAT


SRCS+=	fixunsgen_ieee754.c
SRCS+=	fixunsgen64_ieee754.c #fixunssfdi.c fixunsdfdi.c
.if (${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el") \
    && (!defined(CPUFLAGS) || empty(CPUFLAGS:M-mabi=32))
SRCS+=	fixunstfsi_ieee754.c fixunstfdi_ieee754.c
.endif
.endif

.if (${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el") \
    && (!defined(CPUFLAGS) || empty(CPUFLAGS:M-mabi=32))
SRCS+=	floatunditf_ieee754.c
.endif
