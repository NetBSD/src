#	$NetBSD: Makefile.inc,v 1.16 2021/10/31 22:24:35 thorpej Exp $

.include <bsd.own.mk>

SRCS+=	__sigtramp2.S

CPPFLAGS+= -I.
CPPFLAGS.assym.h+=-D__LIBC12_SOURCE__ -D_KERNTYPES

.if ${MKSOFTFLOAT} != "no"
.include <softfloat/Makefile.inc>
CPPFLAGS+= -DSOFTFLOAT


SRCS+=	fixunsgen_ieee754.c
SRCS+=	fixunsgen64_ieee754.c #fixunssfdi.c fixunsdfdi.c
.if ${MACHINE_MIPS64} && (!defined(CPUFLAGS) || empty(CPUFLAGS:M-mabi=32))
SRCS+=	fixunstfsi_ieee754.c fixunstfdi_ieee754.c
.endif
.endif

.if ${MACHINE_MIPS64} && (!defined(CPUFLAGS) || empty(CPUFLAGS:M-mabi=32))
SRCS+=	floatunditf_ieee754.c
.endif
