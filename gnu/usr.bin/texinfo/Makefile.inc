#	$NetBSD: Makefile.inc,v 1.3 2000/11/06 15:30:49 itojun Exp $

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

.include <bsd.own.mk>

CPPFLAGS+=	-I${.CURDIR}/../common -I${DIST}/texinfo/lib \
		-DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H

DPADD+=		${LIBCOMMONDIR}/libcommon.a
LDADD+=		-L${LIBCOMMONDIR} -lcommon

# libintl-less bulid should comment the following 3 lines out
DPADD+=		${LIBINTL}
LDADD+=		-lintl
CPPFLAGS+=	-DENABLE_NLS=1 -DHAVE_GETTEXT=1 -DHAVE_LIBINTL_H=1

DIST=		${.CURDIR}/../../../dist
.if !make(print-objdir)
LIBCOMMONDIR!=	cd ${.CURDIR}/../common && ${MAKE} print-objdir
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # __MAKEFILE_INC_INCLUDED__
