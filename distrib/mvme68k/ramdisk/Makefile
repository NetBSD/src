#	$NetBSD: Makefile,v 1.17 2002/03/25 07:43:43 lukem Exp $

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.own.mk>

IMAGE=		ramdisk.fs
IMAGESIZE=	1600b

WARNS=		1

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
MAKEDEVTARGETS=	std md0 sd0 sd1 sd2 st0 cd0
IMAGEDEPENDS=	${CRUNCHBIN} \
		dot.profile rd.fstab rd.passwd rd.welcome \
		${DESTDIR}/usr/mdec/bootsd ${DESTDIR}/usr/mdec/bootst \
		${DESTDIR}/usr/mdec/bootxx ${DESTDIR}/usr/mdec/stboot \
		${_SRC_TOP_}/etc/netconfig ${_SRC_TOP_}/etc/protocols \
		${_SRC_TOP_}/etc/services


realall: ${IMAGE}

release:


# Use stubs to eliminate some large stuff from libc
HACKSRC=${DISTRIBDIR}/utils/libhack
.include "${HACKSRC}/Makefile.inc"

# This is listed in ramdiskbin.conf but is built here.
${CRUNCHBIN}: libhack.o


.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
