#	$NetBSD: Makefile.inc,v 1.7 2004/03/18 19:48:15 skrll Exp $

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

.if ${MKPIC} != "no"
.if ${G_ENABLE_SHARED} == "yes" && ${G_SHLIB_LINK} != ""
CPPFLAGS+=	-DENABLE_SHARED_LIBGCC
.endif

.if empty(G_SHLIB_MULTILIB)
CPPFLAGS+=	-DNO_SHARED_LIBGCC_MULTILIB
.endif
.endif
