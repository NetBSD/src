#	$NetBSD: Makefile.inc,v 1.2 2003/07/25 17:19:22 mrg Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gcc

GCCARCH=	${.CURDIR}/../arch/${MACHINE_ARCH}

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DNETBSD_NATIVE
HOST_CPPFLAGS+=	-I${.CURDIR}/..

.if ${USETOOLS} == "yes"
COMPATOBJDIR!=	cd ${NETBSDSRCDIR}/tools/compat && ${PRINTOBJDIR}
NBCOMPATLIB=	-L${COMPATOBJDIR} -lnbcompat
.endif

GNUHOSTDIST=	${DIST}
BUILD_PREFIX=	${C_BUILD_PREFIX}

LIBIBERTYOBJ!=		cd ${.CURDIR}/../libiberty && ${PRINTOBJDIR}
FRONTENDOBJ!=		cd ${.CURDIR}/../frontend && ${PRINTOBJDIR}
BACKENDOBJ!=		cd ${.CURDIR}/../backend && ${PRINTOBJDIR}
LIBCPPOBJ!=		cd ${.CURDIR}/../libcpp && ${PRINTOBJDIR}

HOSTLIBIBERTYOBJ!=	cd ${.CURDIR}/../host-libiberty && ${PRINTOBJDIR}
HOSTLIBIBERTY=		${HOSTLIBIBERTYOBJ}/libiberty/libiberty.a

.include "${GCCARCH}/defs.mk"
