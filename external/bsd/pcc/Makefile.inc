#	$NetBSD: Makefile.inc,v 1.5 2010/02/05 08:58:47 plunky Exp $

PCC_DIR:=${.PARSEDIR}
PCC_DIST=${PCC_DIR}/dist/pcc
PCC_LIBS=${PCC_DIR}/dist/pcc-libs

TARGOS = netbsd

.if ${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb"
TARGMACH = mips
.else
TARGMACH = ${MACHINE_ARCH}
.endif

.if !exists(${PCC_DIST}/arch/${TARGMACH})
ERROR!= echo "ERROR: ${TARGMACH} not yet supported - write code!" >&2;echo
.endif

VERSSTR = PACKAGE_STRING " [" PACKAGE_CHECKOUT "] for ${HOST_OSTYPE}"

libexecdir = /usr/libexec
includedir = /usr/include

CPPFLAGS+=	-DVERSSTR=${VERSSTR:Q}
CPPFLAGS+=	-DLIBEXECDIR=\"${libexecdir}/\"
CPPFLAGS+=	-DINCLUDEDIR=\"${includedir}/\"
CPPFLAGS+=	-Dos_${TARGOS}
CPPFLAGS+=	-Dmach_${TARGMACH}
CPPFLAGS+=	-I${PCC_DIR}

.if exists(${.CURDIR}/../../../Makefile.inc)
.include "${.CURDIR}/../../../Makefile.inc"
.endif
