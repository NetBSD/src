#	$NetBSD: Makefile,v 1.1 2003/08/14 18:20:47 christos Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/gdbserver.mk"

PROG=		gdbreplay

SRCS=		gdbreplay.c
CPPFLAGS=	-I${DIST} -I${.CURDIR}/../arch/${MACHINE_ARCH} \
		${G_INTERNAL_CFLAGS:M-[ID]*:N*readline*:N*intl*:N-I.*} \
		-I${TOP}/lib/libbfd/arch/${MACHINE_ARCH}

.include "../../Makefile.inc"
.include <bsd.prog.mk>

.PATH: ${DIST}/gdb/gdbserver
