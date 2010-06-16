# $NetBSD: Makefile.inc,v 1.9 2010/06/16 18:10:57 joerg Exp $

.include <bsd.own.mk>

VERSION=	1.10.1

CPPFLAGS+=	-DVERSION=\"${VERSION}\" -DUGLY

.if (${HOSTPROG:U} == "")
CPPFLAGE+=	-DHAVE_STRLCAT -DHAVE_STRLCPY
.endif


DISTDIR:=	${.PARSEDIR}/dist

.PATH:	${DISTDIR}

.for _LIB in man mdoc roff
MDOCMLOBJDIR.${_LIB} !=	cd ${.PARSEDIR}/lib/lib${_LIB} && ${PRINTOBJDIR}
MDOCMLLIB.${_LIB}=	${MDOCMLOBJDIR.${_LIB}}/lib${_LIB}.a
.endfor

WARNS?=	4
