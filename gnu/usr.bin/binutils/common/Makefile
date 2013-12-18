#	$NetBSD: Makefile,v 1.16.44.2 2013/12/18 18:43:29 matt Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu

.include "${.CURDIR}/../common/Makefile.inc"
.include "${.CURDIR}/arch/${BFD_MACHINE_ARCH}/defs.mk"

TEXINFO=	${G_TEXINFOS}
INFOFLAGS=	-I${DIST}/binutils/doc -I{.CURDIR}

.PATH: ${DIST}/binutils ${DIST}/binutils/doc

${TEXINFO}: config.texi

config.texi: ${.CURDIR}/arch/${BFD_MACHINE_ARCH}/defs.mk
	${_MKTARGET_CREATE}
	rm -f config.texi
	echo "@set VERSION ${G_VERSION}" > config.texi

CLEANFILES+=	config.texi

.include <bsd.info.mk>
