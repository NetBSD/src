#	$NetBSD: Makefile,v 1.1.4.2 2002/09/19 21:18:44 thorpej Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/toolchain

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/gdbserver.mk"

PROG=		gdbreplay

SRCS=		gdbreplay.c
CPPFLAGS=	-I${DIST} -I${.CURDIR}/../arch/${MACHINE_ARCH} \
		${G_INTERNAL_CFLAGS:M-[ID]*:N*readline*:N*intl*:N-I.*} \
		-I${TOP}/lib/libbfd/arch/${MACHINE_ARCH}

.include "../../Makefile.inc"
.include <bsd.prog.mk>

.PATH: ${DIST}/gdb/gdbserver
