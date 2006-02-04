#	$NetBSD: Makefile,v 1.1 2006/02/04 16:22:53 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/doc
.PATH: ${DIST}
TEXINFO = cvsclient.texi cvs.texinfo
INFOFLAGS = -I${DIST} -I${.CURDIR}

.include <bsd.info.mk>
