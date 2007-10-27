#	$NetBSD: Makefile.inc,v 1.2 2007/10/27 18:14:45 abs Exp $

MDIR=	${DIST}/arch/${TARGMACH}
MIPDIR=	${DIST}/mip

TARGOS = netbsd
# XXX Currently only handles x86 (i386)
.if ${MACHINE_ARCH} == "i386"
TARGMACH = x86
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

# Somewhat inelegant logic to handle config.h - obviously not crossbuildable
#
../config.h: ${DIST}/config.h.in ${DIST}/configure
	mkdir -p configtmp
	cd configtmp && ${SH} ${DIST}/configure && mv config.h ../..
	rm -rf configtmp

DPSRCS+=../config.h
CLEANFILES+=../config.h
