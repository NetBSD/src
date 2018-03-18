#	$NetBSD: Makefile,v 1.14.14.4 2018/03/18 01:17:29 pgoyette Exp $

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

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DCPU_UCODE
# pick up more for bsd.klinks.mk as needed.
MACHINE_EXTRA= x86
.else
MACHINE_EXTRA= none

.endif

.include "../../compat/common/Makefile.sysio"

SRCS+=	compat_mod.c compat_exec.c compat_util.c

.PATH:	${S}/arch/${MACHINE}/${MACHINE}
.PATH:	${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}
.PATH:	${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}
.PATH:	${S}	${S}/arch/${MACHINE_EXTRA}/${MACHINE_EXTRA}

.for i in compat_13_machdep.c compat_16_machdep.c compat_60_cpu_ucode.c
.   if exists(${S}/arch/${MACHINE}/${MACHINE}/${i}) || \
	exists(${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}/${i}) || \
	exists(${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}/${i}) || \
	exists(${S}/arch/${MACHINE_EXTRA}/${MACHINE_EXTRA}/${i})
SRCS+= ${i}
.   endif
.endfor

.include <bsd.kmodule.mk>
