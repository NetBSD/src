#	$NetBSD: Makefile,v 1.6 2024/05/24 13:26:35 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

IMAGE=		diskimage
IMAGESIZE=	2600k
LISTS=		${.CURDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.dot
IMAGEENDIAN=	be
IMAGEDEPENDS=	netbsd-INSTALL.gz ${DESTDIR}/usr/mdec/boot.emips

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		RAMDISK ${RAMDISK} netbsd-INSTALL
MDSET_RELEASEDIR=	binary/kernel

IMAGE_RELEASEDIR=	installation/diskimage

.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
