#	$NetBSD: Makefile.inc,v 1.7 2011/06/29 02:15:13 mrg Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}
DIST=		${TOP}/external/gpl3/gcc/dist

GCCARCH=	${.CURDIR}/../gcc/arch/${MACHINE_ARCH}

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DNETBSD_NATIVE
HOST_CPPFLAGS+=	-I${.CURDIR}/..

.if ${USETOOLS} == "yes"
COMPATOBJDIR!=	cd ${NETBSDSRCDIR}/tools/compat && ${PRINTOBJDIR}
NBCOMPATLIB=	-L${COMPATOBJDIR} -lnbcompat
.endif

GNUHOSTDIST=	${DIST}
BUILD_PREFIX=	${C_BUILD_PREFIX}

LIBIBERTYOBJ!=		cd ${.CURDIR}/../../lib/libiberty && ${PRINTOBJDIR}
FRONTENDOBJ!=		cd ${.CURDIR}/../frontend && ${PRINTOBJDIR}
BACKENDOBJ!=		cd ${.CURDIR}/../backend && ${PRINTOBJDIR}
LIBCPPOBJ!=		cd ${.CURDIR}/../libcpp && ${PRINTOBJDIR}
LIBDECNUMBEROBJ!=	cd ${.CURDIR}/../libdecnumber && ${PRINTOBJDIR}

HOSTLIBIBERTYOBJ!=	cd ${.CURDIR}/../host-libiberty && ${PRINTOBJDIR}
HOSTLIBIBERTY=		${HOSTLIBIBERTYOBJ}/libiberty/libiberty.a

BASEVER!=		cat ${GNUHOSTDIST}/gcc/BASE-VER
# XXX
#DEVPHASE!=		cat ${GNUHOSTDIST}/gcc/DEV-PHASE
DEVPHASE=
DATESTAMP!=		cat ${GNUHOSTDIST}/gcc/DATESTAMP

BASEVERSTR=		"\"$(BASEVER)\""
.if !empty(DEVPHASE)
DEVPHASESTR=		"\" $(DEVPHASE)\""
.else
DEVPHASESTR=		"\"\""
.endif
.if !empty(DEVPHASE)
DATESTAMPSTR=		"\" $(DATESTAMP)\""
.else
DATESTAMPSTR=		"\"\""
.endif

# XXX pull this out of our configs
G_BUGURL=<http://www.NetBSD.org/Misc/send-pr.html>
G_BUGURL_s="\"${G_BUG_URL}\""
G_PKGVERSION_s=(NetBSD nb1 20110620) 
G_PKGVERSION_s="\"${G_PKGVERSION} \""

VER_CPPFLAGS=			-DBUGURL=${G_BUGURL_s} \
				-DPKGVERSION=${G_PKGVERSION_s} \
				-DBASEVER="\"${BASEVER}"\" \
				-DDATESTAMP="\"${DATESTAMP}"\" \
				-DDEVPHASE=${DEVPHASE} \
				-DREVISION=${REVISION}
CPPFLAGS.version.c=		${VER_CPPFLAGS}

.include "${GCCARCH}/defs.mk"

.if ${MKPIC} != "no"
.if ${G_ENABLE_SHARED} == "yes" && ${G_SHLIB_LINK} != ""
CPPFLAGS+=	-DENABLE_SHARED_LIBGCC
.endif

.if empty(G_SHLIB_MULTILIB)
CPPFLAGS+=	-DNO_SHARED_LIBGCC_MULTILIB
.endif
.endif

# This depends on the "extern inline" mess, so downgrade to something safe.
# CPPFLAGS is applied after CFLAGS, which gets the -std=gnu99
CPPFLAGS+=	-std=gnu89

NOCLANGERROR=	# defined
