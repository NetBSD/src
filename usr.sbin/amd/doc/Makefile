#	$NetBSD: Makefile,v 1.1.1.4 2000/11/20 00:16:18 wiz Exp $

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/doc
.PATH:	${DIST}

TEXINFO=	am-utils.texi
INFOFLAGS=	-I${DIST} -I${.CURDIR}

CLEANFILES+=	am-utils.info

.include <bsd.info.mk>
