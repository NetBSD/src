#	$NetBSD: Makefile,v 1.1 2001/11/09 05:52:20 thorpej Exp $

PROG=	dbsym

.if defined(DBSYM_TARGET_PREFIX)
PROGNAME= ${DBSYM_TARGET_PREFIX}${PROG}
NAM=	${DBSYM_TARGET_PREFIX}dbsym.8
.else
MAN=	dbsym.8
.endif

.if defined(DBSYM_TARGET_PREFIX)
${DBSYM_TARGET_PREFIX}dbsym.8: ${.CURDIR}/dbsym.8
	cp ${.IMPSRC} ${.TARGET}
.endif

.ifdef HOSTPROG
# BFDDIR and IBERTYDIR must already be set
TOP=		${.CURDIR}/../..
DIST=		${TOP}/dist/toolchain

# Host tool Makefile MUST set HOST_CPPFLAGS!
#CPPFLAGS+=	-I${BFDDIR} -I${DIST}/include

DPADD+=	${BFDDIR}/libbfd.a
LDADD+=	-L${BFDDIR} -lbfd

DPADD+=	${IBERTYDIR}/libiberty.a
LDADD+=	-L${IBERTYDIR} -liberty
.else
# XXX in an ideal world...
# LDADD+= -lbfd
# DPADD+= ${LIBBFD}

.include <bsd.own.mk>
.ifdef USE_NEW_TOOLCHAIN
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
DIST=		${.CURDIR}/../../dist
BFDOBJ!=	cd ${.CURDIR}/../../lib/libbfd && ${PRINTOBJDIR}

CPPFLAGS+=	-I${BFDOBJ} -I${DIST}/bfd -I${DIST}/binutils -I${DIST}/include \
		-DHAVE_CONFIG_H 

DPADD+=		${BFDOBJ}/libbfd_pic.a
LDADD+=		-L${BFDOBJ} -lbfd
.endif # !USE_NEW_TOOLCHAIN

.include <bsd.prog.mk>
.endif # HOSTPROG
