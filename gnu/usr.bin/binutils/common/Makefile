#	$NetBSD: Makefile,v 1.14 2003/10/20 01:15:03 lukem Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/toolchain

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

TEXINFO=	${G_TEXINFOS}
INFOFLAGS=	-I${DIST}/binutils/doc -I{.CURDIR}

.PATH: ${DIST}/binutils ${DIST}/binutils/doc

${TEXINFO}: config.texi

config.texi: ${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk
	${_MKMSGCREATE}
	${_MKCMD}\
	rm -f config.texi
	${_MKCMD}\
	echo "@set VERSION ${VERSION}" > config.texi

CLEANFILES+=	config.texi

.include <bsd.info.mk>
