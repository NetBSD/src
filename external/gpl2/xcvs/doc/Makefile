#	$NetBSD: Makefile,v 1.1.2.2 2009/05/13 18:53:22 jym Exp $

SUBDIR=
.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/doc
.PATH: ${DIST}
TEXINFO = cvsclient.texi cvs.texinfo
INFOFLAGS = -I${DIST} -I${.CURDIR}

.include <bsd.info.mk>
