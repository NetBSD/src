#	$NetBSD: Makefile.inc,v 1.1.4.2 2012/04/17 00:04:37 yamt Exp $

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
