#	$NetBSD: Makefile.inc,v 1.3 2007/10/28 10:06:41 ragge Exp $

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

libexecdir = ${DESTDIR}/usr/libexec
includedir = ${DESTDIR}/usr/include

CPPFLAGS+= -DLIBEXECDIR=\"${libexecdir}\" -DINCLUDEDIR=\"${includedir}\"
CPPFLAGS+= -I${DIST}/os/${TARGOS} -I${MDIR} -Dmach_${TARGMACH} -Dos_${TARGOS}
CPPFLAGS+= -I${.CURDIR} -I${.CURDIR}/..

# Somewhat inelegant logic to handle config.h - obviously not crossbuildable
#
../../config.h: ${DIST}/config.h.in ${DIST}/configure
	mkdir -p configtmp
	cd configtmp && ${SH} ${DIST}/configure && mv config.h ../../..
	rm -rf configtmp

DPSRCS+=../../config.h
CLEANFILES+=../../config.h
