# $NetBSD: Makefile.inc,v 1.2 2009/10/26 14:55:12 joerg Exp $

.include <bsd.own.mk>

VERSION=	1.9.10

CPPFLAGS+=	-DVERSION=\"${VERSION}\"

DISTDIR:=	${.PARSEDIR}/dist

.PATH:	${DISTDIR}

.for _LIB in man mdoc
MDOCMLOBJDIR.${_LIB} !=	cd ${.PARSEDIR}/lib/lib${_LIB} && ${PRINTOBJDIR}
MDOCMLLIB.${_LIB}=	${MDOCMLOBJDIR.${_LIB}}/lib${_LIB}.a
.endfor

WARNS?=	4
