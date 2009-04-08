#	$NetBSD: Makefile,v 1.1 2009/04/08 16:27:52 christos Exp $

SUBDIR=
.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/doc
.PATH: ${DIST}
TEXINFO = cvsclient.texi cvs.texinfo
INFOFLAGS = -I${DIST} -I${.CURDIR}

.include <bsd.info.mk>
