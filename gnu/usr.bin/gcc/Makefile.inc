#	$NetBSD: Makefile.inc,v 1.1 2001/06/18 16:18:31 tv Exp $

TOP=		${.CURDIR}/../../..
DIST=		${TOP}/dist/toolchain

GCCARCH=	${.CURDIR}/../arch/${MACHINE_ARCH}

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DNETBSD_NATIVE

# Don't auto-frob .y or .l files.
.l.c .y.c .y.h:

.include "${GCCARCH}/defs.mk"
