#	$NetBSD: Makefile,v 1.4 2009/09/26 11:51:34 skrll Exp $

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
	rm -f ${.TARGET}
	(echo "@set VERSION ${G_VERSION}"; \
	if [ -n "$(PKGVERSION)" ]; then \
		echo "@set VERSION_PACKAGE $(PKGVERSION)"; \
	fi) > ${.TARGET}

CLEANFILES+=	bfdver.texi

.include <bsd.info.mk>
