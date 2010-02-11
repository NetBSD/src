#	$NetBSD: Makefile.inc,v 1.8 2010/02/11 14:18:45 plunky Exp $

PCC_DIR:=${.PARSEDIR}
PCC_DIST=${PCC_DIR}/dist/pcc
PCC_LIBS=${PCC_DIR}/dist/pcc-libs

TARGOS = netbsd

.if ${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb"
TARGMACH = mips
.elif ${MACHINE_ARCH} == "x86_64"
TARGMACH = amd64
.else
TARGMACH = ${MACHINE_ARCH}
.endif

.if !exists(${PCC_DIST}/arch/${TARGMACH})
ERROR!= echo "ERROR: ${TARGMACH} not yet supported - write code!" >&2;echo
.endif

VERSSTR = PACKAGE_STRING " for ${TARGOS}-${TARGMACH}"

libexecdir = /usr/libexec
includedir = /usr/include

CPPFLAGS+=	-DVERSSTR=${VERSSTR:Q}
CPPFLAGS+=	-DLIBEXECDIR=\"${libexecdir}/\"
CPPFLAGS+=	-DINCLUDEDIR=\"${includedir}/\"
CPPFLAGS+=	-Dos_${TARGOS}
CPPFLAGS+=	-Dmach_${TARGMACH}
CPPFLAGS+=	-I${PCC_DIR}

.include <bsd.endian.mk>

# We only build binaries from here where the pcc host and target architectures
# will be the same. For a cross-compiler, use the tools/pcc build.
#
.if ${TARGET_ENDIANNESS} == "1234"
CPPFLAGS+=	-DHOST_LITTLE_ENDIAN
CPPFLAGS+=	-DTARGET_LITTLE_ENDIAN
.elif ${TARGET_ENDIANNESS} == "4321"
CPPFLAGS+=	-DHOST_BIG_ENDIAN
CPPFLAGS+=	-DTARGET_BIG_ENDIAN
.else
ERROR!= echo "ERROR: Unknown TARGET_ENDIANNESS" >&2;echo
.endif

.if exists(${.CURDIR}/../../../Makefile.inc)
.include "${.CURDIR}/../../../Makefile.inc"
.endif
