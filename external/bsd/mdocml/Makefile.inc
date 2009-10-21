# $NetBSD: Makefile.inc,v 1.1 2009/10/21 18:04:52 joerg Exp $

.include <bsd.own.mk>

VERSION=	1.9.9

CPPFLAGS+=	-DVERSION=\"${VERSION}\"

DISTDIR:=	${.PARSEDIR}/dist

.PATH:	${DISTDIR}

.for _LIB in man mdoc
MDOCMLOBJDIR.${_LIB} !=	cd ${.PARSEDIR}/lib/lib${_LIB} && ${PRINTOBJDIR}
MDOCMLLIB.${_LIB}=	${MDOCMLOBJDIR.${_LIB}}/lib${_LIB}.a
.endfor

WARNS?=	4
