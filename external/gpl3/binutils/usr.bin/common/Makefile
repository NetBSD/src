#	$NetBSD: Makefile,v 1.1 2009/08/18 20:22:08 skrll Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/external/gpl3/binutils

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"
.include "${.CURDIR}/../common/Makefile.inc"

TEXINFO=	${G_TEXINFOS}
INFOFLAGS=	-I${DIST}/binutils/doc -I${DIST}/libiberty -I{.CURDIR}

.PATH: ${DIST} ${DIST}/binutils/doc

${TEXINFO}: bfdver.texi

bfdver.texi: ${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk
	${_MKTARGET_CREATE}
	rm -f config.texi
	(echo "@set VERSION ${VERSION}"; \
	if [ -n "$(PKGVERSION)" ]; then \
		echo "@set VERSION_PACKAGE $(PKGVERSION)"; \
	fi) > ${.TARGET}

.include <bsd.info.mk>
