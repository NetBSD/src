#	$NetBSD: Makefile,v 1.15 2002/03/24 18:08:19 scw Exp $

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
		${DESTDIR}/usr/mdec/bootxx \
		${_SRC_TOP_}/etc/group ${_SRC_TOP_}/etc/master.passwd \
		${_SRC_TOP_}/etc/protocols ${_SRC_TOP_}/etc/netconfig \
		${_SRC_TOP_}/etc/services


realall: ${IMAGE}

release:


# Use stubs to eliminate some large stuff from libc
HACKSRC=${DISTRIBDIR}/utils/libhack
.include "${HACKSRC}/Makefile.inc"

# Turn off libhack's gethostby*() as it only supports /etc/hosts
HACKOBJS:=	${HACKOBJS:Ngethost.o}

# This is listed in ramdiskbin.conf but is built here.
${CRUNCHBIN}: libhack.o


.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
