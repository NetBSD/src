#	$NetBSD: Makefile,v 1.12 2002/09/19 03:09:33 lukem Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/toolchain

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

TEXINFO=	${G_TEXINFOS}
INFOFLAGS=	-I${DIST}/binutils

.PATH: ${DIST}/binutils

.include <bsd.info.mk>
