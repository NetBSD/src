#	$NetBSD: Makefile,v 1.5 2001/11/12 23:16:19 tv Exp $

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN)
TOP=		${.CURDIR}/../..
DIST=		${TOP}/dist/toolchain

PROG=		mdsetimage
MAN=		mdsetimage.8

CPPFLAGS+=	-I${BFDDIR} -I${DIST}/bfd -I${DIST}/binutils -I${DIST}/include \
		-DHAVE_CONFIG_H

DPADD+=		${BFDDIR}/libbfd.a
LDADD+=		-L${BFDDIR} -lbfd

DPADD+=		${IBERTYDIR}/libiberty.a
LDADD+=		-L${IBERTYDIR} -liberty

.ifndef HOSTPROG
.include	"${TOP}/lib/libbfd/arch/${MACHINE_ARCH}/defs.mk"

BFDDIR!=	cd ${.CURDIR}/../../lib/libbfd && ${PRINTOBJDIR}
IBERTYDIR!=	cd ${TOP}/lib/libiberty && ${PRINTOBJDIR}

CPPFLAGS+=	-I${TOP}/lib/libbfd/arch/${MACHINE_ARCH} ${G_INCLUDES}
LDADD+=		-lintl
DPADD+=		${LIBINTL}
.endif # HOSTPROG
.endif # USE_NEW_TOOLCHAIN

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif # HOSTPROG
