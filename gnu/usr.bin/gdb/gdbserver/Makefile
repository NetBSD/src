#	$NetBSD: Makefile,v 1.2 2002/09/19 03:09:37 lukem Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/toolchain

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/gdbserver.mk"

PROG=		gdbserver

SRCS=		${G_OBS:.o=.c}
CPPFLAGS=	-I${DIST} -I${.CURDIR}/../arch/${MACHINE_ARCH} \
		${G_INTERNAL_CFLAGS:M-[ID]*:N*readline*:N*intl*:N-I.*} \
		-I${TOP}/lib/libbfd/arch/${MACHINE_ARCH}

.include "../../Makefile.inc"
.include <bsd.prog.mk>

.PATH: ${DIST}/gdb/gdbserver
