#	$NetBSD: Makefile,v 1.4 2002/03/31 03:04:49 thorpej Exp $

.include "${.CURDIR}/../../../Makefile.inc"
.include <bsd.own.mk>

IMAGE=		ramdisk.fs
IMAGESIZE=	3072k

WARNS=		1

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	le
MAKEDEVTARGETS=	ramdisk
IMAGEDEPENDS=	${CRUNCHBIN} \
		disktab.preinstall dot.hdprofile dot.profile \
		${DESTDIR}/usr/share/misc/termcap \
		${_SRC_TOP_}/etc/group ${_SRC_TOP_}/etc/master.passwd \
		${_SRC_TOP_}/etc/protocols ${_SRC_TOP_}/etc/netconfig \
		${_SRC_TOP_}/etc/services

MAKEFS_FLAGS=	-f 10

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o


realall: ${IMAGE}

release:

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
