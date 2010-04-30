# $FreeBSD: src/sys/modules/dtrace/dtrace/Makefile,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $

.include "../../Makefile.inc"
.include "../../Makefile.assym"

ARCHDIR=	${MACHINE}

.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common/dtrace
.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/dtrace
.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/dtrace/${ARCHDIR}

KMOD=		dtrace
SRCS=		dtrace.c \
		dtrace_asm.S \
		dtrace_subr.c

.if ${MACHINE} == "amd64" || ${MACHINE} == "i386"
SRCS+=		dis_tables.c \
		instr_size.c
.endif

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/sys \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dev/dtrace \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dev/dtrace/${ARCHDIR} \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common \
		-DDIS_MEM

CPPFLAGS+=	-DSMP -DDEBUG

EXPORT_SYMS=	dtrace_register \
		dtrace_unregister \
		dtrace_probe_lookup

.include <bsd.kmodule.mk>
