#	$NetBSD: Makefile,v 1.11 2002/09/16 16:22:18 thorpej Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

TOP=		${.CURDIR}/../../..
DIST=		${TOP}/dist/toolchain

TEXINFO=	${G_TEXINFOS}
INFOFLAGS=	-I${DIST}/binutils

.PATH: ${DIST}/binutils

.include <bsd.info.mk>
