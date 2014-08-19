#	$NetBSD: Makefile.inc,v 1.17.2.1 2014/08/19 23:52:08 tls Exp $

PCC_DIR:=${.PARSEDIR}
PCC_DIST=${PCC_DIR}/dist/pcc
PCC_LIBS=${PCC_DIR}/dist/pcc-libs

# (these strings will be updated by the prepare-import.sh script)
PCC_VERSION=1.1.0.DEVEL
PCC_DATESTAMP=20140706

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

VERSSTR="pcc ${PCC_VERSION} ${PCC_DATESTAMP} for ${TARGOS}-${TARGMACH}"

CPPFLAGS+=	-DGCC_COMPAT
CPPFLAGS+=	-DPCC_DEBUG
CPPFLAGS+=	-DVERSSTR=${VERSSTR:Q}
CPPFLAGS+=	-Dos_${TARGOS}
CPPFLAGS+=	-Dmach_${TARGMACH}
CPPFLAGS+=	-I${PCC_DIR}/include

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

# top-level Makefile.inc
.include "../../../Makefile.inc"
