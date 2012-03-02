#	$NetBSD: Makefile.inc,v 1.1.2.2 2012/03/02 18:27:51 riz Exp $

.include <bsd.own.mk>

EXPATDIR=	${NETBSDSRCDIR}/external/mit/expat
EXPATSRCDIR=	${NETBSDSRCDIR}/external/mit/expat/dist

.for _LIB in expat
.if !defined(EXPATOBJDIR.${_LIB})
EXPATOBJDIR.${_LIB}!=	cd ${EXPATDIR}/lib/lib${_LIB} && ${PRINTOBJDIR}
.MAKEOVERRIDES+=	EXPATOBJDIR.${_LIB}
.endif
EXPATLIB.${_LIB}=	${EXPATOBJDIR.${_LIB}}/lib${_LIB}.a
.endfor
