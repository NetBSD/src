#	$NetBSD: Makefile,v 1.8 2000/11/20 00:25:03 wiz Exp $

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/doc
.PATH:	${DIST}

TEXINFO=	am-utils.texi
INFOFLAGS=	-I${DIST} -I${.CURDIR}

CLEANFILES+=	am-utils.info

.include <bsd.info.mk>
