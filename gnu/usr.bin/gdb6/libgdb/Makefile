#	$NetBSD: Makefile,v 1.1 2006/05/15 16:31:37 nathanw Exp $

MKPRIVATELIB=yes

.include <bsd.own.mk>

.include "../Makefile.inc"

.if exists(${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk)
MKPIC=no
MKPROFILE=no  
LIB=		gdb

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb6

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"



SRCS=		${G_GDB_OBJS:.o=.c}
CPPFLAGS=	-I${.CURDIR} \
		-I${.CURDIR}/../arch/${MACHINE_ARCH} \
		-I${DIST}/gdb \
		-I${DIST}/gdb/config \
		-I${DIST}/include \
		-I${DIST}/include/opcode \
		-I${DIST}/bfd \
		-I${.CURDIR}/../bfd \
		-I${DIST}/gdb/tui \
		-I${DIST} \
		-DHAVE_CONFIG_H \
		-DLOCALEDIR='"/usr/share/locale"' \
		-DUSE_INCLUDED_REGEX \
		-DNETBSD_CORE

.if ${USE_TUI} == "yes"
SRCS+=		${G_TUI_OBJS:.o=.c}
CPPFLAGS+=	-DUSE_TUI
.endif

.endif

.include "../../Makefile.inc"
.include <bsd.lib.mk>
.include <bsd.info.mk>


.PATH: ${DIST}/gdb ${DIST}/gdb/mi ${DIST}/gdb/cli ${DIST}/gdb/tui \
    ${DIST}/gdb/signals ${DIST}/gdb/doc ${.CURDIR}/../arch/${MACHINE_ARCH}
