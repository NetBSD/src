#	$NetBSD: Makefile,v 1.15 2002/04/01 12:16:14 lukem Exp $

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.own.mk>

IMAGE=		ramdisk.fs
IMAGESIZE=	1536k
MAKEFS_FLAGS=	-f 15

WARNS=		1

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list  ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	le
MAKEDEVTARGETS=	std mt0 st0 rd0 rd1 dz0 dl0 dhu0 rx0 ra0 ra1 ra2 ra3 hp0 hp1 \
		sd0 sd1 sd2 md0 pty0 cd0 cd1
IMAGEDEPENDS=	${CRUNCHBIN} \
		disktab.preinstall dot.profile termcap.vt100 \
		${DESTDIR}/boot ${DESTDIR}/usr/mdec/xxboot \
		${_SRC_TOP_}/etc/group ${_SRC_TOP_}/etc/master.passwd \
		${_SRC_TOP_}/etc/netconfig ${_SRC_TOP_}/etc/protocols \
		${_SRC_TOP_}/etc/services

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
