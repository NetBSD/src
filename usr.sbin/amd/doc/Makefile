#	$NetBSD: Makefile,v 1.9 2004/11/27 01:39:23 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/doc
.PATH:	${DIST}

TEXINFO=	am-utils.texi
INFOFLAGS=	-I${DIST} -I${.CURDIR}

CLEANFILES+=	version.texi

am-utils.info: version.texi

version.texi:
	echo @set UPDATED $$(date +"%d %B %Y") > ${.TARGET}
	echo @set UPDATED-MONTH $$(date +"%B %Y") >> ${.TARGET}
	echo @set EDITION $$( (cat ${.CURDIR}/../include/config.h && \
	    echo VERSION) | cpp -P | tr -d \") >> ${.TARGET}
	echo @set VERSION $$( (cat ${.CURDIR}/../include/config.h && \
	    echo VERSION) | cpp -P | tr -d \") >> ${.TARGET}


.include <bsd.info.mk>
