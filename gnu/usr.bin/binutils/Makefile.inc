#	$NetBSD: Makefile.inc,v 1.8 2001/01/09 08:52:58 enami Exp $

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

.include <bsd.own.mk>

CPPFLAGS+=	-I${.OBJDIR} -I${.CURDIR} -I${.CURDIR}/../common \
		-I${BFDOBJ} -I${DIST}/bfd -I${DIST}/binutils -I${DIST}/include \
		-DHAVE_CONFIG_H
DPADD+=		${LIBCOMMONDIR}/libcommon.a ${BFDOBJ}/libbfd_pic.a
LDADD+=		-L${LIBCOMMONDIR} -lcommon -L${BFDOBJ} -lbfd

DIST=		${.CURDIR}/../../../dist
BFDOBJ!=	cd ${.CURDIR}/../../../lib/libbfd && ${PRINTOBJDIR}
LIBCOMMONDIR!=	cd ${.CURDIR}/../common && ${PRINTOBJDIR}

.PATH: ${DIST}/binutils

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # __MAKEFILE_INC_INCLUDED__
