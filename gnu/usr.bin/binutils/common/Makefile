#	$NetBSD: Makefile,v 1.16 2003/11/27 15:02:21 mrg Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"
.include "${.CURDIR}/../common/Makefile.inc"

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
