# $NetBSD: Makefile.inc,v 1.4 2009/11/16 22:28:43 joerg Exp $

.include <bsd.own.mk>

VERSION=	1.9.14

CPPFLAGS+=	-DVERSION=\"${VERSION}\"

DISTDIR:=	${.PARSEDIR}/dist

.PATH:	${DISTDIR}

.for _LIB in man mdoc
MDOCMLOBJDIR.${_LIB} !=	cd ${.PARSEDIR}/lib/lib${_LIB} && ${PRINTOBJDIR}
MDOCMLLIB.${_LIB}=	${MDOCMLOBJDIR.${_LIB}}/lib${_LIB}.a
.endfor

WARNS?=	4
