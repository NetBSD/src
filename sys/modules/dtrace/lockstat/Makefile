#	$NetBSD: Makefile,v 1.1.2.3 2015/09/22 12:06:09 skrll Exp $

.include "../../Makefile.inc"

.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/lockstat

KMOD=		dtrace_lockstat
SRCS=		lockstat.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/sys
CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common
CPPFLAGS+=	-DKDTRACE_HOOKS
.if (!empty(MACHINE_ARCH:Mearm*) || !empty(MACHINE_ARCH:Marm*))
CPPFLAGS+=	-D__HAVE_CPU_COUNTER
.endif

.include <bsd.kmodule.mk>
