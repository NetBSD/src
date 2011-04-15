# $NetBSD: Makefile.inc,v 1.13 2011/04/15 14:36:30 joerg Exp $

.include <bsd.own.mk>

VERSION!=	cd ${.PARSEDIR}/dist && ${MAKE} -V VERSION

CPPFLAGS+=	-DVERSION=\"${VERSION}\" -DUGLY

.if (${HOSTPROG:U} == "")
CPPFLAGS+=	-DHAVE_STRLCAT -DHAVE_STRLCPY
.endif

DISTDIR:=	${.PARSEDIR}/dist

.PATH:	${DISTDIR}

.for _LIB in mandoc
MDOCMLOBJDIR.${_LIB} !=	cd ${.PARSEDIR}/lib/lib${_LIB} && ${PRINTOBJDIR}
MDOCMLLIB.${_LIB}=	${MDOCMLOBJDIR.${_LIB}}/lib${_LIB}.a
.endfor

WARNS?=	4
