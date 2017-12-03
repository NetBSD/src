#	$NetBSD: Makefile,v 1.6.16.4 2017/12/03 11:38:50 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat

#
# We don't want to hook these in for the native emulation, but do want
# the symbols available for other emulations.  So include the source
# files but don't hook them in via compat_modcmd().
#
CPPFLAGS+=	-DCOMPAT_09
CPPFLAGS+=	-DCOMPAT_10
CPPFLAGS+=	-DCOMPAT_11
CPPFLAGS+=	-DCOMPAT_12
CPPFLAGS+=	-DCOMPAT_13
CPPFLAGS+=	-DCOMPAT_14
CPPFLAGS+=	-DCOMPAT_15
CPPFLAGS+=	-DCOMPAT_16

CPPFLAGS+=	-DCOMPAT_20
CPPFLAGS+=	-DCOMPAT_30 -DCOMPAT_40 -DCOMPAT_50
CPPFLAGS+=	-DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80
CPPFLAGS+=	-DCOMPAT_43

.include "../../compat/common/Makefile.sysio"

SRCS+=	compat_mod.c

.PATH:	${S}/arch/${MACHINE}/${MACHINE}
.PATH:	${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}
.PATH:	${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}

.if (exists(${S}/arch/${MACHINE}/${MACHINE}/compat_13_machdep.c)) || \
    (exists(${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}/compat_13_machdep.c)) || \
    (exists(${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}/compat_13_machdep.c))
SRCS+=	compat_13_machdep.c
.endif

.if (exists(${S}/arch/${MACHINE}/${MACHINE}/compat_16_machdep.c)) || \
    (exists(${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}/compat_16_machdep.c)) || \
    (exists(${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}/compat_16_machdep.c))
SRCS+=	compat_16_machdep.c
.endif

.include <bsd.kmodule.mk>
