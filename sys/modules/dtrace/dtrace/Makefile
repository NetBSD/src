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
		dtrace_xoroshiro128_plus.c \
		dtrace_asm.S \
		dtrace_subr.c

.if ${MACHINE} == "amd64" || ${MACHINE} == "i386"
.PATH: ${OSNETDIR}/dev/dtrace/x86
SRCS+=		dis_tables.c \
		instr_size.c
CPPFLAGS+=	-I${OSNETDIR}/dev/dtrace/x86
.endif

CPPFLAGS+=	-I${OSNETDIR}/sys \
		-I${OSNETDIR}/dev/dtrace \
		-I${OSNETDIR}/dev/dtrace/${ARCH} \
		-I${OSNETDIR}/dist/uts/common \
		-I${OSNETDIR}/dist/common/util \
		-DDIS_MEM

CPPFLAGS+=	-Wno-unknown-pragmas \
		-Wno-shadow \
		-Wno-unused-variable \
		-Wno-unused-function

CPPFLAGS+=	 ${${ACTIVE_CC} == "gcc" :? -Wno-discarded-qualifiers :}

COPTS.dis_tables.c+= \
		-Wno-missing-field-initializers \
		-Wno-cast-qual \
		-Wno-parentheses \
		-Wno-uninitialized

COPTS.dtrace.c+=	${${ACTIVE_CC} == "gcc" :? -Wno-unused-but-set-variable :}

COPTS.dtrace_subr.c+= \
		-Wno-unused-value

EXPORT_SYMS=	dtrace_register \
		dtrace_unregister \
		dtrace_probe_lookup

.include <bsd.kmodule.mk>
