#	$NetBSD: Makefile.inc,v 1.1 2008/08/24 05:47:05 gmcgarry Exp $

MDIR=	${DIST}/arch/${TARGMACH}
MIPDIR=	${DIST}/mip

TARGOS = netbsd
# XXX Currently only handles i386
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

CPPFLAGS+= -DLIBEXECDIR=\"${libexecdir}\" -DINCLUDEDIR=\"${includedir}\"
CPPFLAGS+= -I${DIST}/os/${TARGOS} -I${MDIR} -Dmach_${TARGMACH} -Dos_${TARGOS}
CPPFLAGS+= -I${.CURDIR} -I${.CURDIR}/..

.if exists(${.CURDIR}/../../../Makefile.inc)
.include "${.CURDIR}/../../../Makefile.inc"
.endif
