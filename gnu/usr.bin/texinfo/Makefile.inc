#	$NetBSD: Makefile.inc,v 1.1 1999/02/11 15:42:14 tv Exp $

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

CPPFLAGS+=	-I${.CURDIR}/../common -I${DIST}/texinfo/intl -I${DIST}/texinfo/lib \
		-DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H

DPADD+=		${LIBCOMMONDIR}/libcommon.a
LDADD+=		-L${LIBCOMMONDIR} -lcommon

DIST=		${.CURDIR}/../../../dist
.if !make(print-objdir)
LIBCOMMONDIR!=	cd ${.CURDIR}/../common && ${MAKE} print-objdir
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # __MAKEFILE_INC_INCLUDED__
