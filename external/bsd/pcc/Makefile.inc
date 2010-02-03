#	$NetBSD: Makefile.inc,v 1.3 2010/02/03 22:13:13 plunky Exp $

PCC_DIR:=${.PARSEDIR}
PCC_DIST=${PCC_DIR}/dist/pcc
PCC_LIBS=${PCC_DIR}/dist/pcc-libs

TARGOS = netbsd

.if ${MACHINE_ARCH} == "i386"
TARGMACH = i386
.elif ${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb"
TARGMACH = mips
.elif ${MACHINE_ARCH} == "vax"
TARGMACH = vax
.else
ERROR!= echo "ERROR: ${MACHINE_ARCH} not yet supported - write code!" >&2;echo
.endif

VERSSTR = PACKAGE_STRING PACKAGE_CHECKOUT "for ${HOST_OSTYPE}"

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
