#	$NetBSD: Makefile.inc,v 1.5.10.1 2013/02/25 00:27:46 tls Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gcc4

GCCARCH=	${.CURDIR}/../arch/${MACHINE_ARCH}

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DNETBSD_NATIVE
HOST_CPPFLAGS+=	-I${.CURDIR}/..

.if ${USETOOLS} == "yes"
NBCOMPATLIB=   -L${TOOLDIR}/lib -lnbcompat
.endif

GNUHOSTDIST=	${DIST}
BUILD_PREFIX=	${C_BUILD_PREFIX}

LIBIBERTYOBJ!=		cd ${.CURDIR}/../libiberty && ${PRINTOBJDIR}
FRONTENDOBJ!=		cd ${.CURDIR}/../frontend && ${PRINTOBJDIR}
BACKENDOBJ!=		cd ${.CURDIR}/../backend && ${PRINTOBJDIR}
LIBCPPOBJ!=		cd ${.CURDIR}/../libcpp && ${PRINTOBJDIR}

HOSTLIBIBERTYOBJ!=	cd ${.CURDIR}/../host-libiberty && ${PRINTOBJDIR}
HOSTLIBIBERTY=		${HOSTLIBIBERTYOBJ}/libiberty/libiberty.a

BASEVER!=		cat ${GNUHOSTDIST}/gcc/BASE-VER
DEVPHASE!=		cat ${GNUHOSTDIST}/gcc/DEV-PHASE
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

VER_CPPFLAGS=		-DBASEVER=${BASEVERSTR} \
			-DDATESTAMP=${DATESTAMPSTR} \
			-DDEVPHASE=${DEVPHASESTR}
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
