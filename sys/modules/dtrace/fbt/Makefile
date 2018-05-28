# $FreeBSD: src/sys/modules/dtrace/sdt/Makefile,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $

.include "../../Makefile.inc"

.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/fbt
.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/fbt/${MACHINE_CPU}

.if ${MACHINE} == "amd64" || ${MACHINE} == "i386"
.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/fbt/x86
CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/dev/fbt/x86
.endif

KMOD=		dtrace_fbt
SRCS=		fbt.c fbt_isa.c

.PATH:	${S}/kern

SRCS+=		kern_ctf.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/sys \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dev/fbt \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dev/fbt/${MACHINE_CPU}

CPPFLAGS+=	-Wno-unknown-pragmas

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
