#	$NetBSD: Makefile,v 1.15 2003/10/21 10:01:20 lukem Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/toolchain

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

TEXINFO=	${G_TEXINFOS}
INFOFLAGS=	-I${DIST}/binutils/doc -I{.CURDIR}

.PATH: ${DIST}/binutils ${DIST}/binutils/doc

${TEXINFO}: config.texi

config.texi: ${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk
	${_MKTARGET_CREATE}
	rm -f config.texi
	echo "@set VERSION ${VERSION}" > config.texi

CLEANFILES+=	config.texi

.include <bsd.info.mk>
