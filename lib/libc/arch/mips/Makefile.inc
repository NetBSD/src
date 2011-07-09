#	$NetBSD: Makefile.inc,v 1.11 2011/07/09 02:30:27 matt Exp $

.include <bsd.own.mk>

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

CPPFLAGS+= -I.
CPPFLAGS.assym.h+=-D__LIBC12_SOURCE__

.if ${MKSOFTFLOAT} != "no"
.include <softfloat/Makefile.inc>
CPPFLAGS+= -DSOFTFLOAT

SRCS+=	fixunsgen_ieee754.c fixunssfsi_ieee754.c fixunsdfsi_ieee754.c
SRCS+=	fixunsgen64_ieee754.c #fixunssfdi.c fixunsdfdi.c
.if ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
SRCS+=	fixunstfsi_ieee754.c fixunstfdi_ieee754.c
.endif
.endif

.if ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
SRCS+=	floatunditf_ieee754.c
.endif
