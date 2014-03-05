# $FreeBSD: src/sys/modules/dtrace/sdt/Makefile,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $

.include "../Makefile.inc"

.include <bsd.own.mk>

OSNETDIR=${NETBSDSRCDIR}/external/cddl/osnet

.if exists(${OSNETDIR}/dev/cyclic/${MACHINE:S/amd64/i386/})
ARCH=${MACHINE:S/amd64/i386/}
.elif exists(${OSNETDIR}/dev/cyclic/${MACHINE_CPU})
ARCH=${MACHINE_CPU}
.else
.error ${MACHINE} or ${MACHINE_CPU} not supported.
.endif

.PATH: ${OSNETDIR}/dev/cyclic

KMOD=		cyclic
SRCS=		cyclic.c

CPPFLAGS+=	-I${OSNETDIR}/sys \
		-I${OSNETDIR}/dev/cyclic/${ARCH} \
		-I${OSNETDIR}/dist/uts/common

CPPFLAGS+=	-Wno-unknown-pragmas -Wno-cast-qual

.include <bsd.kmodule.mk>
