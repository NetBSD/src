#	$NetBSD: Makefile.inc,v 1.10 2001/11/26 18:19:47 jmc Exp $

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN)

GCPPFLAGS=	${G_DEFS} ${G_INCLUDES}
CPPFLAGS+=	${GCPPFLAGS:M-D*:N-DLOCALEDIR*} ${GCPPFLAGS:M-I*:N-I.*} \
		-I${TOP}/lib/libbfd/arch/${MACHINE_ARCH} \
		-DLOCALEDIR=\"${LOCALEDIR}\"

.else
#
# XXX - BEGIN - Old toolchain build goop - delete when everything migrated - XXX
#
CPPFLAGS+=	-I${.OBJDIR} -I${.CURDIR} -I${.CURDIR}/../common \
		-I${BFDOBJ} -I${DIST}/bfd -I${DIST}/binutils -I${DIST}/include \
		-DHAVE_CONFIG_H
DPADD+=		${LIBCOMMONDIR}/libcommon.a ${BFDOBJ}/libbfd_pic.a
LDADD+=		-L${LIBCOMMONDIR} -lcommon -L${BFDOBJ} -lbfd

DIST=		${.CURDIR}/../../../dist
BFDOBJ!=	cd ${.CURDIR}/../../../lib/libbfd && ${PRINTOBJDIR}
LIBCOMMONDIR!=	cd ${.CURDIR}/../common && ${PRINTOBJDIR}

.PATH: ${DIST}/binutils
#
# XXX - END - Old toolchain build goop - delete when everything migrated - XXX
#
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # __MAKEFILE_INC_INCLUDED__
