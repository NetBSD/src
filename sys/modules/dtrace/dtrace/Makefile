# $FreeBSD: src/sys/modules/dtrace/dtrace/Makefile,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $

.include "../../Makefile.inc"
.include "../../Makefile.assym"

.include <bsd.own.mk>

OSNETDIR=${NETBSDSRCDIR}/external/cddl/osnet

.if exists(${OSNETDIR}/dev/dtrace/${MACHINE})
ARCH=${MACHINE}
.elif exists(${OSNETDIR}/dev/dtrace/${MACHINE_CPU})
ARCH=${MACHINE_CPU}
.else
.error ${MACHINE} or ${MACHINE_CPU} not supported.
.endif

.PATH: ${OSNETDIR}/dist/uts/common/dtrace
.PATH: ${OSNETDIR}/dev/dtrace
.PATH: ${OSNETDIR}/dev/dtrace/${ARCH}

KMOD=		dtrace
SRCS=		dtrace.c \
		dtrace_asm.S \
		dtrace_subr.c

.if ${MACHINE} == "amd64" || ${MACHINE} == "i386"
SRCS+=		dis_tables.c \
		instr_size.c
.endif

CPPFLAGS+=	-I${OSNETDIR}/sys \
		-I${OSNETDIR}/dev/dtrace \
		-I${OSNETDIR}/dev/dtrace/${ARCH} \
		-I${OSNETDIR}/dist/uts/common \
		-DDIS_MEM

CPPFLAGS+=	-DSMP -DDEBUG

CPPFLAGS+=	-Wno-unknown-pragmas

EXPORT_SYMS=	dtrace_register \
		dtrace_unregister \
		dtrace_probe_lookup

.include <bsd.kmodule.mk>
