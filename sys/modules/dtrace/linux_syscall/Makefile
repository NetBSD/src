# $FreeBSD: src/sys/modules/dtrace/sdt/Makefile,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $

.include "../../Makefile.inc"

.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/systrace

KMOD=		dtrace_syscall_linux
SRCS=		systrace.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/sys \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common 

CPPFLAGS+=	-Wno-unknown-pragmas

.include <bsd.kmodule.mk>
