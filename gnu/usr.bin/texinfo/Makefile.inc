#	$NetBSD: Makefile.inc,v 1.7 2003/01/17 15:32:23 wiz Exp $

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
IDIST=		${DIST}/texinfo

CPPFLAGS+=	-I${.CURDIR}/../common -I${IDIST}/lib \
		-DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H

DPADD+=		${LIBCOMMONDIR}/libcommon.a
LDADD+=		-L${LIBCOMMONDIR} -lcommon

# libintl-less build should comment the following 3 lines out
DPADD+=		${LIBINTL}
LDADD+=		-lintl
CPPFLAGS+=	-DENABLE_NLS=1 -DHAVE_GETTEXT=1 -DHAVE_LIBINTL_H=1

LIBCOMMONDIR!=	cd ${.CURDIR}/../common && ${PRINTOBJDIR}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # __MAKEFILE_INC_INCLUDED__
