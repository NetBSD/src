#	$NetBSD: Makefile,v 1.1 2001/12/19 23:54:32 tv Exp $

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/gdbserver.mk"

TOP=		${.CURDIR}/../../..
DIST=		${TOP}/dist/toolchain

PROG=		gdbreplay
NOMAN=		# defined

SRCS=		gdbreplay.c
CPPFLAGS=	-I${DIST} -I${.CURDIR}/../arch/${MACHINE_ARCH} \
		${G_INTERNAL_CFLAGS:M-[ID]*:N*readline*:N*intl*:N-I.*} \
		-I${TOP}/lib/libbfd/arch/${MACHINE_ARCH}

.include "../../Makefile.inc"
.include <bsd.prog.mk>

.PATH: ${DIST}/gdb/gdbserver
