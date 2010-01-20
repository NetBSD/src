#	$NetBSD: Makefile.inc,v 1.2 2010/01/20 11:45:54 plunky Exp $

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

libexecdir = /usr/libexec
includedir = /usr/include

CPPFLAGS+=	-DLIBEXECDIR=\"${libexecdir}/\"
CPPFLAGS+=	-DINCLUDEDIR=\"${includedir}/\"
CPPFLAGS+=	-Dos_${TARGOS}
CPPFLAGS+=	-Dmach_${TARGMACH}
CPPFLAGS+=	-I${PCC_DIR}

.if exists(${.CURDIR}/../../../Makefile.inc)
.include "${.CURDIR}/../../../Makefile.inc"
.endif
