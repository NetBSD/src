#	$NetBSD: Makefile,v 1.4 2003/09/14 09:50:15 martin Exp $

.include <bsd.own.mk>

.if exists(${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk)
MKPIC=no
MKPROFILE=no  
MKLINT=no	# lint misses important ifdef magic on 64 bit archs

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

LIB=		opcodes
SRCS=		${G_OPCODES_OBJS:.o=.c}
CPPFLAGS=	-I${.CURDIR}  -I${.CURDIR}/../arch/${MACHINE_ARCH} \
		-I${DIST}/include -I${.CURDIR}/../bfd \
		-DHAVE_CONFIG_H

libinstall::

.endif

.include "../../Makefile.inc"
.include <bsd.lib.mk>

.PATH: ${DIST}/opcodes
