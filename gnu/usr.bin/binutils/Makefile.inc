#	$NetBSD: Makefile.inc,v 1.7 1999/02/09 19:21:53 tv Exp $

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

CPPFLAGS+=	-I${.OBJDIR} -I${.CURDIR} -I${.CURDIR}/../common \
		-I${BFDOBJ} -I${DIST}/bfd -I${DIST}/binutils -I${DIST}/include \
		-DHAVE_CONFIG_H
DPADD+=		${LIBCOMMONDIR}/libcommon.a ${BFDOBJ}/libbfd_pic.a
LDADD+=		-L${LIBCOMMONDIR} -lcommon -L${BFDOBJ} -lbfd

DIST=		${.CURDIR}/../../../dist
.if !make(print-objdir)
BFDOBJ!=	cd ${.CURDIR}/../../../lib/libbfd && ${MAKE} print-objdir
LIBCOMMONDIR!=	cd ${.CURDIR}/../common && ${MAKE} print-objdir
.endif

.PATH: ${DIST}/binutils

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # __MAKEFILE_INC_INCLUDED__
