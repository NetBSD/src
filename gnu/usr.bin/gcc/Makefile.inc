#	$NetBSD: Makefile.inc,v 1.2 2001/11/26 18:19:48 jmc Exp $

TOP=		${.CURDIR}/../../..
DIST=		${TOP}/dist/toolchain

GCCARCH=	${.CURDIR}/../arch/${MACHINE_ARCH}

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DNETBSD_NATIVE

.include "${GCCARCH}/defs.mk"
