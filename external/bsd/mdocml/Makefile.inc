# $NetBSD: Makefile.inc,v 1.14.2.1 2012/04/17 00:03:35 yamt Exp $

.include <bsd.own.mk>

VERSION!=	cd ${.PARSEDIR}/dist && ${MAKE} -V VERSION

CPPFLAGS+=	-DVERSION=\"${VERSION}\" -DUGLY

.if (${HOSTPROG:U} == "")
CPPFLAGS+=	-DHAVE_STRLCAT -DHAVE_STRLCPY -DHAVE_STRPTIME -DHAVE_MMAP
.endif

DISTDIR:=	${.PARSEDIR}/dist

.PATH:	${DISTDIR}

.for _LIB in mandoc
MDOCMLOBJDIR.${_LIB} !=	cd ${.PARSEDIR}/lib/lib${_LIB} && ${PRINTOBJDIR}
MDOCMLLIB.${_LIB}=	${MDOCMLOBJDIR.${_LIB}}/lib${_LIB}.a
.endfor

WARNS?=	4
