#	$NetBSD: Makefile,v 1.1 2001/10/22 03:23:00 simonb Exp $

PROG=	mdsetimage
MAN=	mdsetimage.8

# XXX in an ideal world...
# LDADD+= -lbfd
# DPADD+= ${LIBBFD}

.ifdef USE_NEW_TOOLCHAIN
.include <bsd.own.mk>
TOP=		${.CURDIR}/../..
DIST=		${TOP}/dist/toolchain

.include	"${TOP}/lib/libbfd/arch/${MACHINE_ARCH}/defs.mk"

CPPFLAGS+=	-I${TOP}/lib/libbfd/arch/${MACHINE_ARCH} \
		-I${BFDOBJ} ${G_INCLUDES}

BFDOBJ!=	cd ${.CURDIR}/../../lib/libbfd && ${PRINTOBJDIR}
DPADD+=		${BFDOBJ}/libbfd_pic.a
LDADD+=		-L${BFDOBJ} -lbfd

IBERTYOBJ!=	cd ${TOP}/lib/libiberty && ${PRINTOBJDIR}
LDADD+=		-L${IBERTYOBJ} -liberty
DPADD+=		${IBERTYOBJ}/libiberty.a

LDADD+=		-lintl
DPADD+=		${LIBINTL}
.else # !USE_NEW_TOOLCHAIN
.include <bsd.own.mk>
DIST=		${.CURDIR}/../../dist
BFDOBJ!=	cd ${.CURDIR}/../../lib/libbfd && ${PRINTOBJDIR}

CPPFLAGS+=	-I${BFDOBJ} -I${DIST}/bfd -I${DIST}/binutils -I${DIST}/include \
		-DHAVE_CONFIG_H

DPADD+=		${BFDOBJ}/libbfd_pic.a
LDADD+=		-L${BFDOBJ} -lbfd
.endif # !USE_NEW_TOOLCHAIN

.include <bsd.prog.mk>
