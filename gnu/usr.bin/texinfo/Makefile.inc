#	$NetBSD: Makefile.inc,v 1.5 2001/08/08 14:59:26 wiz Exp $

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

.include <bsd.own.mk>

CPPFLAGS+=	-I${.CURDIR}/../common -I${DIST}/texinfo/lib \
		-DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H

DPADD+=		${LIBCOMMONDIR}/libcommon.a
LDADD+=		-L${LIBCOMMONDIR} -lcommon

# libintl-less build should comment the following 3 lines out
DPADD+=		${LIBINTL}
LDADD+=		-lintl
CPPFLAGS+=	-DENABLE_NLS=1 -DHAVE_GETTEXT=1 -DHAVE_LIBINTL_H=1

DIST=		${.CURDIR}/../../../dist
LIBCOMMONDIR!=	cd ${.CURDIR}/../common && ${PRINTOBJDIR}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # __MAKEFILE_INC_INCLUDED__
