#	$NetBSD: Makefile,v 1.1 2003/08/17 18:03:26 christos Exp $

.include <bsd.own.mk>

.if exists(${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk)
MKPIC=no
MKPROFILE=no

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

LIB=		bfd
SRCS=		${G_BFD_OBJS:.o=.c}
CPPFLAGS=	-I${.CURDIR} -I${.CURDIR}/../arch/${MACHINE_ARCH} \
		-I${DIST}/include \
		-DHAVE_CONFIG_H \
		-DNETBSD_CORE \
		${G_BFD_CPPFLAGS} 


.endif

.include "../../Makefile.inc"
.include <bsd.lib.mk>

.PATH: ${DIST}/bfd
