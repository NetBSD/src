#	$NetBSD: Makefile,v 1.3 2016/01/24 01:01:43 christos Exp $

.include "../../Makefile.inc"

.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/lockstat

KMOD=		dtrace_lockstat
SRCS=		lockstat.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/sys
CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common
CPPFLAGS+=	-DKDTRACE_HOOKS
.if exists(${DESTDIR}/usr/include/machine/cpu_counter.h)
CPPFLAGS+=	-D__HAVE_CPU_COUNTER
.endif

.include <bsd.kmodule.mk>
