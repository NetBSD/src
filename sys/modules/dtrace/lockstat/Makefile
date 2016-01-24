#	$NetBSD: Makefile,v 1.4 2016/01/24 01:12:06 christos Exp $

.include "../../Makefile.inc"

.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/lockstat

KMOD=		dtrace_lockstat
SRCS=		lockstat.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/sys
CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common
CPPFLAGS+=	-DKDTRACE_HOOKS

.if !empty(MACHINE_ARCH:Mearm*)
ARMV:= ${MACHINE_ARCH:S/earmv//}
.elif !empty(MACHINE_ARCH:Marm*)
ARMV:= ${MACHINE_ARCH:S/armv//}
.else
ARMV:= 0
.endif

.if ${ARMV} > 4
CPPFLAGS+=	-D__HAVE_CPU_COUNTER
.endif

.include <bsd.kmodule.mk>
