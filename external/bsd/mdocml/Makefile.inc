# $NetBSD: Makefile.inc,v 1.17.16.1 2019/06/10 21:51:09 christos Exp $

.include <bsd.own.mk>


.if (${HOSTPROG:U} == "")
VERSION!=	cd ${.PARSEDIR}/dist && ${MAKE} -V VERSION
CPPFLAGS+=	-DVERSION=\"${VERSION}\" -I${DISTDIR}/../include
#CPPFLAGS+=	-D_OPENBSD_SOURCE
#CPPFLAGS+=	-DMANPATH_BASE=\"/usr/share/man:/usr/X11R7/man\"
.endif

DISTDIR:=	${.PARSEDIR}/dist

.PATH:	${DISTDIR}

.for _LIB in mandoc
MDOCMLOBJDIR.${_LIB} !=	cd ${.PARSEDIR}/lib/lib${_LIB} && ${PRINTOBJDIR}
MDOCMLLIB.${_LIB}=	${MDOCMLOBJDIR.${_LIB}}/lib${_LIB}.a
.endfor

WARNS?=	4
