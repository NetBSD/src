#	$NetBSD: Makefile,v 1.9.16.2 2017/12/03 11:38:52 jdolecek Exp $

.include "../../Makefile.inc"

.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/lockstat

KMOD=		dtrace_lockstat
SRCS=		lockstat.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/sys
CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common
CPPFLAGS+=	-DKDTRACE_HOOKS

.if !empty(MACHINE_ARCH:Mearmv*)
ARMV:= ${MACHINE_ARCH:S/earmv//:S/eb//:S/hf//}
.elif !empty(MACHINE_ARCH:Marmv*)
ARMV:= ${MACHINE_ARCH:S/armv//:S/eb//:S/hf//}
.else
ARMV:= 0
.endif

.if ${ARMV} > 4
CPPFLAGS+=	-D__HAVE_CPU_COUNTER
.endif

.include <bsd.kmodule.mk>
