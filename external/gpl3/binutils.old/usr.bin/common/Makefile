#	$NetBSD: Makefile,v 1.5.8.1 2018/04/16 01:59:27 pgoyette Exp $

.include "${.CURDIR}/../common/Makefile.inc"
.include "${.CURDIR}/arch/${BINUTILS_MACHINE_ARCH}/defs.mk"

TEXINFO=	${G_TEXINFOS}
INFOFLAGS=	-I${DIST}/binutils/doc -I${DIST}/libiberty -I{.CURDIR}

.PATH: ${DIST} ${DIST}/binutils/doc

${TEXINFO}: bfdver.texi

bfdver.texi: ${.CURDIR}/arch/${BINUTILS_MACHINE_ARCH}/defs.mk
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	(echo "@set VERSION ${G_VERSION}"; \
	if [ -n "$(G_PKGVERSION)" ]; then \
		echo "@set VERSION_PACKAGE $(G_PKGVERSION)"; \
	fi; \
	if [ -n "$(G_REPORT_BUGS_TEXI)" ]; then \
		echo "@set BUGURL $(G_REPORT_BUGS_TEXI)"; \
	fi \
	) > ${.TARGET}

CLEANFILES+=	bfdver.texi

.include <bsd.info.mk>
