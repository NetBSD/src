#	$NetBSD: Makefile,v 1.12 2002/03/25 07:43:42 lukem Exp $

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.own.mk>

IMAGE=		ramdisk.fs
IMAGESIZE=	1200k

WARNS=		1

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
MAKEDEVTARGETS=	std md0
IMAGEDEPENDS=	${CRUNCHBIN} \
		dot.profile termcap.mini \
		${DESTDIR}/usr/mdec/installboot ${DESTDIR}/usr/mdec/uboot.lif \
		${_SRC_TOP_}/etc/group ${_SRC_TOP_}/etc/master.passwd \
		${_SRC_TOP_}/etc/protocols ${_SRC_TOP_}/etc/netconfig \
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
