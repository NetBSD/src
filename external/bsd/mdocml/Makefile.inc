# $NetBSD: Makefile.inc,v 1.14 2011/10/12 15:33:38 njoly Exp $

.include <bsd.own.mk>

VERSION!=	cd ${.PARSEDIR}/dist && ${MAKE} -V VERSION

CPPFLAGS+=	-DVERSION=\"${VERSION}\" -DUGLY

.if (${HOSTPROG:U} == "")
CPPFLAGS+=	-DHAVE_STRLCAT -DHAVE_STRLCPY -DHAVE_STRPTIME
.endif

DISTDIR:=	${.PARSEDIR}/dist

.PATH:	${DISTDIR}

.for _LIB in mandoc
MDOCMLOBJDIR.${_LIB} !=	cd ${.PARSEDIR}/lib/lib${_LIB} && ${PRINTOBJDIR}
MDOCMLLIB.${_LIB}=	${MDOCMLOBJDIR.${_LIB}}/lib${_LIB}.a
.endfor

WARNS?=	4
