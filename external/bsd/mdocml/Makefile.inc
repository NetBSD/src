# $NetBSD: Makefile.inc,v 1.3 2009/11/05 18:28:10 joerg Exp $

.include <bsd.own.mk>

VERSION=	1.9.13

CPPFLAGS+=	-DVERSION=\"${VERSION}\"

DISTDIR:=	${.PARSEDIR}/dist

.PATH:	${DISTDIR}

.for _LIB in man mdoc
MDOCMLOBJDIR.${_LIB} !=	cd ${.PARSEDIR}/lib/lib${_LIB} && ${PRINTOBJDIR}
MDOCMLLIB.${_LIB}=	${MDOCMLOBJDIR.${_LIB}}/lib${_LIB}.a
.endfor

WARNS?=	4
