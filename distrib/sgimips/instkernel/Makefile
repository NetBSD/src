#	$NetBSD: Makefile,v 1.1 2002/05/21 20:54:17 rafal Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

IMAGE=		diskimage
IMAGESIZE=	3m
LISTS=		${.CURDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.dot
IMAGEENDIAN=	be
IMAGEDEPENDS=	netbsd-INSTALL.gz netbsd-INDY_INSTALL.gz 

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		RAMDISK ${RAMDISK} netbsd-INSTALL	\
			RAMDISK_INDY ${RAMDISK} netbsd-INDY_INSTALL

MDSET_RELEASEDIR=	binary/kernel

MDSET_SUFFIXES.netbsd-INDY_INSTALL=ecoff create-ecoff
create-ecoff=			${ELF2ECOFF} ${.TARGET:R} ${.TARGET}

IMAGE_RELEASEDIR=	installation/diskimage

.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
