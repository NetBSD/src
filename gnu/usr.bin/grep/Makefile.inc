#	$NetBSD: Makefile.inc,v 1.2 1999/02/11 17:26:22 tv Exp $

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/grep/src ${DIST}/grep/doc

.if !make(print-objdir)
LIBCOMMONDIR!=	cd ${.CURDIR}/../common && ${MAKE} -s print-objdir
.endif

CPPFLAGS+=	-I${.CURDIR}/../common -I${DIST}/grep/intl -DHAVE_CONFIG_H \
		-DLOCALEDIR=\"/usr/share/locale\"
DPADD+=		${LIBCOMMONDIR}/libcommon.a
LDADD+=		-L${LIBCOMMONDIR} -lcommon

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
