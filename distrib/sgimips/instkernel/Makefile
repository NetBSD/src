#	$NetBSD: Makefile,v 1.2 2003/11/13 02:33:39 sekiya Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

IMAGE=		diskimage
IMAGESIZE=	4m
LISTS=		${.CURDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.dot
IMAGEENDIAN=	be
IMAGEDEPENDS=	netbsd-INSTALL32_IP2x.gz netbsd-INSTALL32_IP3x.gz 

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL32_IP2x ${RAMDISK} netbsd-INSTALL32_IP2x	\
			INSTALL32_IP3x ${RAMDISK} netbsd-INSTALL32_IP3x

MDSET_RELEASEDIR=	binary/kernel

MDSET_SUFFIXES.netbsd-INSTALL32_IP2x=ecoff create-ecoff
create-ecoff=			${ELF2ECOFF} ${.TARGET:R} ${.TARGET}

IMAGE_RELEASEDIR=	installation/diskimage

.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
