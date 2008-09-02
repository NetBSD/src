#	$NetBSD: Makefile.inc,v 1.9 2008/09/02 14:46:53 christos Exp $

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
IDIST=		${DIST}/texinfo

CPPFLAGS+=	-I${.CURDIR}/../common -I${IDIST}/lib \
		-DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H

.if !defined(MAKEDOC)
DPADD+=		${LIBCOMMONDIR}/libcommon.a
LDADD+=		-L${LIBCOMMONDIR} -lcommon

DPADD+=		${LIBINTL}
LDADD+=		-lintl

LIBCOMMONDIR!=	cd ${.CURDIR}/../common && ${PRINTOBJDIR}
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # __MAKEFILE_INC_INCLUDED__
