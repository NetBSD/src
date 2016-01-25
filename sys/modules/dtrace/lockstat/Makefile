#	$NetBSD: Makefile,v 1.6 2016/01/25 12:44:52 martin Exp $

.include "../../Makefile.inc"

.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/lockstat

KMOD=		dtrace_lockstat
SRCS=		lockstat.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/sys
CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common
CPPFLAGS+=	-DKDTRACE_HOOKS

.if ${MACHINE_ARCH} == "earm"
ARMV:= 5
.elif !empty(MACHINE_ARCH:Mearm*)
ARMV:= ${MACHINE_ARCH:S/earmv//:S/eb//:S/hf//}
.elif !empty(MACHINE_ARCH:Marm*)
ARMV:= ${MACHINE_ARCH:S/armv//:S/eb//:S/hf//}
.else
ARMV:= 0
.endif

.if ${ARMV} > 4
CPPFLAGS+=	-D__HAVE_CPU_COUNTER
.endif

.include <bsd.kmodule.mk>
