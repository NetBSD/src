#	$NetBSD: Makefile,v 1.2 2003/08/18 00:25:24 martin Exp $

.include <bsd.own.mk>

.if exists(${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk)
MKPIC=no
MKPROFILE=no  

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

LIB=		opcodes
SRCS=		${G_OPCODES_OBJS:.o=.c}
CPPFLAGS=	-I${.CURDIR}  -I${.CURDIR}/../arch/${MACHINE_ARCH} \
		-I${DIST}/include -I${.CURDIR}/../bfd \
		-DHAVE_CONFIG_H \


.endif

.include "../../Makefile.inc"
.include <bsd.lib.mk>

.PATH: ${DIST}/opcodes
