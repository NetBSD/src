# $NetBSD: Makefile.inc,v 1.6 2010/04/05 14:23:56 joerg Exp $

.include <bsd.own.mk>

VERSION=	1.9.22

CPPFLAGS+=	-DVERSION=\"${VERSION}\"

.if (${HOSTPROG:U} == "")
CPPFLAGE+=	-DHAVE_STRLCAT -DHAVE_STRLCPY
.endif


DISTDIR:=	${.PARSEDIR}/dist

.PATH:	${DISTDIR}

.for _LIB in man mdoc
MDOCMLOBJDIR.${_LIB} !=	cd ${.PARSEDIR}/lib/lib${_LIB} && ${PRINTOBJDIR}
MDOCMLLIB.${_LIB}=	${MDOCMLOBJDIR.${_LIB}}/lib${_LIB}.a
.endfor

WARNS?=	4
