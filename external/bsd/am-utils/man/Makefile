#	$NetBSD: Makefile,v 1.1 2008/09/19 21:38:41 christos Exp $

.include <bsd.own.mk>

LIBAMUOBJDIR!=cd ${.CURDIR}/../lib/libamu && ${PRINTOBJDIR}

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/doc
.PATH:	${DIST}

TEXINFO=	am-utils.texi
INFOFLAGS=	-I${DIST} -I${.CURDIR}

CLEANFILES+=	version.texi

am-utils.info: version.texi

version.texi:
	echo @set UPDATED $$(${HOST_SH} ${DIST}/mdate-sh \
	    ${DIST}/am-utils.texi) > ${.TARGET}
	echo @set UPDATED-MONTH $$(${HOST_SH} ${DIST}/mdate-sh \
	    ${DIST}/am-utils.texi) >> ${.TARGET}
	echo @set EDITION $$( (cat ${.CURDIR}/../include/config.h && \
	    echo VERSION) | ${CPP} -P ${CPPFLAGS} | tr -d \") >> ${.TARGET}
	echo @set VERSION $$( (cat ${.CURDIR}/../include/config.h && \
	    echo VERSION) | ${CPP} -P ${CPPFLAGS} | tr -d \") >> ${.TARGET}


.include <bsd.info.mk>
