#	$NetBSD: Makefile,v 1.3 2002/03/06 02:40:06 wiz Exp $

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/doc
.PATH: ${DIST}
TEXINFO = cvsclient.texi cvs.texinfo
INFOFLAGS = -I${DIST} -I${.CURDIR}

.include <bsd.info.mk>
